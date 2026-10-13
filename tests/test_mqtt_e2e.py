"""Integration test: real local Mosquitto + USB serial simulated with a PTY.

Usage: python3 tests/test_mqtt_e2e.py --bridge PATH --mosquitto PATH
       --pub PATH --sub PATH
No Arduino is needed; this does not test physical sensors or hardware resets.
"""
import argparse
import json
import os
from pathlib import Path
import pty
import select
import socket
import subprocess
import tempfile
import time


def stop(process):
    if process is not None and process.poll() is None:
        process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("bridge", "mosquitto", "pub", "sub"):
        parser.add_argument("--" + name, required=True)
    args = parser.parse_args()
    with socket.socket() as probe:
        probe.bind(("127.0.0.1", 0))
        port = probe.getsockname()[1]
    prefix = "/TEF/integration_" + str(os.getpid())
    master, slave = pty.openpty()
    processes = []
    with tempfile.TemporaryDirectory(prefix="edge-mqtt-e2e-") as directory:
        config = Path(directory) / "mosquitto.conf"
        config.write_text(f"listener {port} 127.0.0.1\nallow_anonymous true\npersistence false\n")
        with (Path(directory) / "processes.log").open("w+") as log:
            def start_broker():
                broker = subprocess.Popen([args.mosquitto, "-c", str(config)], stdout=log, stderr=log)
                processes.append(broker)
                deadline = time.monotonic() + 5
                while time.monotonic() < deadline:
                    if broker.poll() is not None:
                        raise AssertionError("Broker exited before accepting connections")
                    try:
                        with socket.create_connection(("127.0.0.1", port), timeout=.1):
                            return broker
                    except OSError:
                        time.sleep(.05)
                raise AssertionError("Broker did not start")

            def publish(payload, retained=False):
                command = [args.pub, "-h", "127.0.0.1", "-p", str(port), "-t", prefix + "/cmd", "-m", payload]
                if retained:
                    command.append("-r")
                subprocess.run(command, check=True, timeout=5)

            def subscribe():
                process = subprocess.Popen([args.sub, "-h", "127.0.0.1", "-p", str(port), "-t", prefix + "/attrs/telemetria", "-C", "1", "-W", "15"], stdout=subprocess.PIPE, stderr=log)
                processes.append(process)
                time.sleep(.3)
                return process

            def receive_command(timeout):
                data = b""
                deadline = time.monotonic() + timeout
                while time.monotonic() < deadline:
                    if select.select([master], [], [], max(0, deadline - time.monotonic()))[0]:
                        data += os.read(master, 1024)
                        if b"\n" in data:
                            break
                return data

            def telemetry(subscriber, status="ok", uptime=1234):
                sample = {"distancia_cm": 25.0 if status == "ok" else None,
                          "referencia_cm": 100, "brilho": 191 if status == "ok" else 0,
                          "status": status, "uptime_ms": uptime}
                frame = json.dumps(sample, separators=(",", ":")).encode() + b"\r\n"
                # Logs and broken/oversized lines must never reach the MQTT topic.
                os.write(master, b"Sistema de Controle de Brilho\n{broken}\n" + b"x" * 600 + b"\n")
                os.write(master, frame[:19])
                time.sleep(.05)
                os.write(master, frame[19:])
                output, _ = subscriber.communicate(timeout=18)
                assert subscriber.returncode == 0, "Telemetry subscriber timed out"
                assert json.loads(output) == sample, output

            try:
                broker = start_broker()
                publish("2000", retained=True)
                subscriber = subscribe()
                bridge = subprocess.Popen([args.bridge, "--serial", os.ttyname(slave), "--broker", "127.0.0.1", "--port", str(port), "--prefix", prefix, "--client-id", "edge-test-" + str(os.getpid())], stdout=log, stderr=log)
                processes.append(bridge)
                assert receive_command(2) == b"", "Retained reset command was replayed"
                assert bridge.poll() is None, "Bridge exited"
                telemetry(subscriber)
                print("PASS: fragmented telemetry, log filtering and retained command rejection")
                publish("99")
                assert receive_command(.5) == b"", "Invalid command reached serial"
                publish("250")
                assert receive_command(3) == b"250\n", "Valid command not forwarded"
                print("PASS: MQTT commands validated and forwarded to serial")
                telemetry(subscribe(), "sem_eco", 2000)
                print("PASS: missing echo published as null with LED off")
                stop(broker)
                time.sleep(.5)
                assert bridge.poll() is None, "Broker outage killed bridge"
                broker = start_broker()
                telemetry(subscribe(), "fora_faixa", 3000)
                publish("100")
                assert receive_command(3) == b"100\n", "Command subscription not restored"
                print("PASS: broker restart restores publishing and command subscription")
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


if __name__ == "__main__":
    main()
