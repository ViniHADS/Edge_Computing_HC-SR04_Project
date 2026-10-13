"""Inicia broker, ponte e API locais; Ctrl+C encerra os tres processos."""
import argparse
from pathlib import Path
import signal
import socket
import subprocess
import time
import urllib.error
import urllib.request

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--serial", help="Porta real do Mega, por exemplo /dev/cu.usbmodemXXXX")
    mode.add_argument("--mock", action="store_true", help="Gera dados artificiais sem placa")
    parser.add_argument("--api-port", type=int, default=8000)
    parser.add_argument("--mqtt-port", type=int, default=1883)
    args = parser.parse_args()
    ports = (args.api_port, args.mqtt_port)
    if len(set(ports)) != 2 or not all(1 <= port <= 65535 for port in ports):
        parser.error("Informe duas portas validas e diferentes")

    broker = ROOT / ".local/bin/mosquitto"
    bridge = ROOT / "ESP32/ESP32_mqtt_mock/build/ESP32Mock"
    python = ROOT / ".venv/bin/python"
    for path in (broker, bridge, python):
        if not path.exists():
            parser.error(f"Falta {path}. Execute sh scripts/setup_local.sh")
    # Nao reutiliza nem encerra servicos de outros projetos por engano.
    for port in ports:
        try:
            with socket.socket() as probe:
                probe.bind(("127.0.0.1", port))
        except OSError:
            parser.error(f"Porta {port} ocupada; encerre o outro servico ou escolha outra porta")

    runtime = ROOT / ".local/run"
    runtime.mkdir(parents=True, exist_ok=True)
    config = runtime / "mosquitto.conf"
    config.write_text(f"listener {args.mqtt_port} 127.0.0.1\nallow_anonymous true\npersistence false\n")
    children, logs = [], []

    def start(name, command, stdin=None):
        log = (runtime / f"{name}.log").open("w")
        logs.append(log)
        child = subprocess.Popen(command, cwd=ROOT, stdin=stdin, stdout=log,
                                 stderr=log, text=True, start_new_session=True)
        children.append((name, child))
        return child

    def stop_signal(signum, frame):
        raise KeyboardInterrupt

    signal.signal(signal.SIGTERM, stop_signal)
    try:
        start("broker", [str(broker), "-c", str(config)])
        command = [str(bridge), "--broker", "127.0.0.1", "--port", str(args.mqtt_port)]
        command += ["--mock"] if args.mock else ["--serial", args.serial]
        start("ponte", command, subprocess.DEVNULL)
        start("api", [str(python), "-m", "api.server", "--port", str(args.api_port),
                      "--mqtt-port", str(args.mqtt_port)])

        # Aguarda a assinatura MQTT da API antes de anunciar disponibilidade.
        deadline = time.monotonic() + 15
        while True:
            for name, child in children:
                if child.poll() is not None:
                    raise RuntimeError(f"{name} encerrou. Veja {runtime / (name + '.log')}")
            try:
                with urllib.request.urlopen(f"http://127.0.0.1:{args.api_port}/health", timeout=1) as response:
                    if response.status == 200:
                        break
            except (OSError, urllib.error.URLError):
                pass
            if time.monotonic() > deadline:
                raise RuntimeError(f"API nao ficou pronta. Veja os logs em {runtime}")
            time.sleep(.2)

        print(f"Postman: http://localhost:{args.api_port} | MQTT: localhost:{args.mqtt_port}", flush=True)
        print("MODO DEMONSTRACAO: dados artificiais, sem reset fisico." if args.mock else "Modo Mega conectado por USB.", flush=True)
        print(f"Logs: {runtime}\nCtrl+C encerra broker, ponte e API.", flush=True)
        while True:
            for name, child in children:
                if child.poll() is not None:
                    raise RuntimeError(f"{name} encerrou. Veja {runtime / (name + '.log')}")
            time.sleep(.5)
    except KeyboardInterrupt:
        print("Encerrando servicos locais...", flush=True)
    finally:
        for _, child in reversed(children):
            if child.poll() is None:
                child.terminate()
                try:
                    child.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    child.kill()
                    child.wait()
        for log in logs:
            log.close()


if __name__ == "__main__":
    main()
