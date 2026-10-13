"""Teste real HTTP -> MQTT -> ponte -> PTY, sem placa nem servicos externos.

Execute apos setup_local.sh: .venv/bin/python tests/test_http_e2e.py
"""
import json
import os
from pathlib import Path
import pty
import select
import socket
import subprocess
import sys
import tempfile
import time
import tty
import urllib.error
import urllib.request

ROOT = Path(__file__).resolve().parents[1]


def free_port():
    with socket.socket() as probe:
        probe.bind(("127.0.0.1", 0))
        return probe.getsockname()[1]


def stop(process):
    if process.poll() is None:
        process.terminate()
        try:
            process.wait(timeout=6)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()


def wait_for(check, timeout=15):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        try:
            result = check()
            if result:
                return result
        except (OSError, urllib.error.URLError):
            pass
        time.sleep(.1)
    raise AssertionError("Condicao nao satisfeita antes do timeout")


def request(port, path, body=None, content_type="application/json"):
    encoded = body.encode() if isinstance(body, str) else json.dumps(body).encode() if body is not None else None
    req = urllib.request.Request(f"http://127.0.0.1:{port}{path}", data=encoded,
                                 headers={"Content-Type": content_type})
    try:
        response = urllib.request.urlopen(req, timeout=4)
    except urllib.error.HTTPError as error:
        response = error
    with response:
        return response.status, json.load(response)


def main():
    port, mqtt_port = free_port(), free_port()
    master, slave = pty.openpty()
    tty.setraw(slave)  # Nao ecoar fixtures enquanto a ponte ainda esta iniciando.
    processes = []
    with tempfile.TemporaryDirectory(prefix="edge-http-test-") as directory:
        config = Path(directory) / "mosquitto.conf"
        config.write_text(f"listener {mqtt_port} 127.0.0.1\nallow_anonymous true\npersistence false\n")
        with (Path(directory) / "services.log").open("w+") as log:
            def start(command):
                process = subprocess.Popen(command, cwd=ROOT, stdout=log, stderr=log)
                processes.append(process)
                return process

            def broker():
                return start([str(ROOT / ".local/bin/mosquitto"), "-c", str(config)])

            def command_received(expected):
                received = b""
                deadline = time.monotonic() + 4
                while time.monotonic() < deadline and b"\n" not in received:
                    if select.select([master], [], [], max(0, deadline - time.monotonic()))[0]:
                        received += os.read(master, 1024)
                assert received == expected, (received, expected)

            def sample(uptime, status="ok", brightness=None):
                data = {"distancia_cm": 50.0 if status == "ok" else None,
                        "referencia_cm": 100, "brilho": 127 if status == "ok" else 0,
                        "status": status, "uptime_ms": uptime}
                if brightness is not None:
                    data["brilho"] = brightness
                os.write(master, (json.dumps(data, separators=(",", ":")) + "\n").encode())
                wait_for(lambda: request(port, "/telemetria")[1].get("dados") == data)
                return data

            try:
                # API inicia mesmo sem broker: relata indisponibilidade e nao aceita comando.
                start([sys.executable, "-m", "api.server", "--port", str(port), "--mqtt-port", str(mqtt_port)])
                wait_for(lambda: request(port, "/health")[0] == 503)
                assert request(port, "/comandos/atraso", {"atraso_ms": 250})[0] == 503
                assert request(port, "/telemetria")[0] == 404
                server = broker()
                wait_for(lambda: request(port, "/health")[0] == 200)
                start([str(ROOT / "ESP32/ESP32_mqtt_mock/build/ESP32Mock"), "--serial", os.ttyname(slave),
                       "--broker", "127.0.0.1", "--port", str(mqtt_port)])
                time.sleep(.5)
                sample(1234)
                print("PASS: API inicia offline, reconecta e recebe telemetria da ponte", flush=True)

                code, result = request(port, "/comandos/atraso", {"atraso_ms": 250})
                assert code == 202 and result["execucao_no_mega_confirmada"] is False
                command_received(b"250\n")
                for value in (99, 60001, True, 100.5, "100", None):
                    assert request(port, "/comandos/atraso", {"atraso_ms": value})[0] == 400
                assert request(port, "/comandos/atraso", "{broken}")[0] == 400
                assert request(port, "/comandos/atraso", {"atraso_ms": 100, "extra": 1})[0] == 400
                assert request(port, "/comandos/atraso", "100", "text/plain")[0] == 415
                assert not select.select([master], [], [], .2)[0]
                sample(2000, "sem_eco")
                sample(2200, "sem_eco", 255)
                sample(2500, "sem_eco", 0)
                print("PASS: LED mantido em 255 durante sem eco chega pela ponte e API", flush=True)
                print("PASS: POST chega a serial, erros sao rejeitados, sem eco retorna null", flush=True)

                stop(server)
                wait_for(lambda: request(port, "/health")[0] == 503)
                assert request(port, "/telemetria")[1]["desatualizada"] is True
                assert request(port, "/comandos/atraso", {"atraso_ms": 2000})[0] == 503
                broker()
                wait_for(lambda: request(port, "/health")[0] == 200)
                sample(3000)
                assert not select.select([master], [], [], .2)[0], "Comando offline foi repetido"
                assert request(port, "/comandos/atraso", {"atraso_ms": 100})[0] == 202
                command_received(b"100\n")
                print("PASS: queda do broker sinaliza cache antigo; retorno restaura operacao", flush=True)
            except Exception:
                log.flush()
                log.seek(0)
                print(log.read())
                raise
            finally:
                for process in reversed(processes):
                    stop(process)
                os.close(master)
                os.close(slave)

    # Valida tambem o comando unico de inicializacao entregue ao usuario.
    api_port, mqtt_port = free_port(), free_port()
    launcher = subprocess.Popen([sys.executable, "scripts/run_local.py", "--mock",
                                 "--api-port", str(api_port), "--mqtt-port", str(mqtt_port)], cwd=ROOT)
    try:
        wait_for(lambda: request(api_port, "/telemetria")[0] == 200)
        # O POST altera a cadencia produzida no C++, sem escrita em stdin.
        assert request(api_port, "/comandos/atraso", {"atraso_ms": 500})[0] == 202
        samples = []
        deadline = time.monotonic() + 8
        while time.monotonic() < deadline and len(samples) < 5:
            data = request(api_port, "/telemetria")[1]["dados"]
            if not samples or samples[-1]["uptime_ms"] != data["uptime_ms"]:
                samples.append(data)
            time.sleep(.03)
        assert len(samples) == 5, "Mock nao gerou leituras automaticamente"
        intervals = [b["uptime_ms"] - a["uptime_ms"] for a, b in zip(samples[1:], samples[2:])]
        assert all(450 <= interval <= 800 for interval in intervals), intervals
        assert len({sample["distancia_cm"] for sample in samples}) > 1
        assert request(api_port, "/comandos/atraso", {"atraso_ms": 100})[0] == 202
        time.sleep(.4)
        first = request(api_port, "/telemetria")[1]["dados"]["uptime_ms"]
        time.sleep(.25)
        second = request(api_port, "/telemetria")[1]["dados"]["uptime_ms"]
        assert second > first, "Mock nao voltou a gerar com atraso menor"
        print("PASS: mock C++ gera dados variaveis e aplica POST de 500 ms e 100 ms", flush=True)
    finally:
        stop(launcher)
    for value in (api_port, mqtt_port):
        with socket.socket() as probe:
            assert probe.connect_ex(("127.0.0.1", value)) != 0, "Servico ficou aberto apos encerrar launcher"


if __name__ == "__main__":
    main()
