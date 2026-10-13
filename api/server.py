"""Servidor HTTP apenas em loopback; execução: python -m api.server."""
import argparse
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
from urllib.parse import urlsplit

from .mqtt_gateway import MqttGateway
from .protocol import valid_delay


def handler_for(gateway):
    class Handler(BaseHTTPRequestHandler):
        def setup(self):
            super().setup()
            self.connection.settimeout(5)

        def reply(self, status, data):
            body = json.dumps(data, ensure_ascii=False, allow_nan=False).encode("utf-8")
            self.send_response(status)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.send_header("Content-Length", str(len(body)))
            self.send_header("Cache-Control", "no-store")
            self.end_headers()
            self.wfile.write(body)

        def do_GET(self):
            path = urlsplit(self.path).path
            if path == "/health":
                ready = gateway.ready()
                self.reply(200 if ready else 503, {
                    "status": "ok" if ready else "mqtt_indisponivel",
                    "mqtt_conectado": ready, "broker": f"localhost:{gateway.port}",
                    "topico_telemetria": gateway.topic,
                    "observacao": "Conexao MQTT nao confirma presenca do Mega"})
            elif path == "/telemetria":
                sample = gateway.snapshot()
                self.reply(200 if sample else 404, sample or {
                    "erro": "Nenhuma telemetria recebida desde o inicio da API"})
            else:
                self.reply(404, {"erro": "Rota inexistente"})

        def do_POST(self):
            if urlsplit(self.path).path != "/comandos/atraso":
                self.reply(404, {"erro": "Rota inexistente"})
                return
            if self.headers.get_content_type() != "application/json":
                self.reply(415, {"erro": "Use Content-Type: application/json"})
                return
            try:
                if self.headers.get("Transfer-Encoding"):
                    raise ValueError("Use Content-Length, sem Transfer-Encoding")
                length = int(self.headers.get("Content-Length", "0"))
                if not 0 < length <= 4096:
                    raise ValueError("Corpo JSON deve ter entre 1 e 4096 bytes")
                data = json.loads(self.rfile.read(length))
                if (not isinstance(data, dict) or set(data) != {"atraso_ms"}
                        or not valid_delay(data["atraso_ms"])):
                    raise ValueError("Informe apenas atraso_ms inteiro entre 100 e 60000")
            except (ValueError, UnicodeError, TimeoutError) as error:
                self.reply(400, {"erro": str(error)})
                return
            try:
                gateway.publish_delay(data["atraso_ms"])
            except ConnectionError as error:
                self.reply(503, {"erro": str(error)})
                return
            # Confirma o envio MQTT, nunca a execução física pela placa.
            self.reply(202, {"status": "enviado_ao_mqtt", "atraso_ms": data["atraso_ms"],
                             "topico": gateway.command_topic,
                             "execucao_no_mega_confirmada": False})

    return Handler


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", type=int, default=8000)
    parser.add_argument("--mqtt-port", type=int, default=1883)
    parser.add_argument("--prefix", default="/TEF/exemplo001")
    args = parser.parse_args()
    if not all(1 <= port <= 65535 for port in (args.port, args.mqtt_port)):
        parser.error("Portas devem estar entre 1 e 65535")
    if not args.prefix or any(char in args.prefix for char in "+#\0"):
        parser.error("Prefixo MQTT invalido")
    gateway = MqttGateway(args.mqtt_port, args.prefix.rstrip("/") or "/")
    server = ThreadingHTTPServer(("127.0.0.1", args.port), handler_for(gateway))
    gateway.start()
    print(f"API: http://localhost:{args.port} | MQTT: localhost:{args.mqtt_port}", flush=True)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()
        gateway.close()


if __name__ == "__main__":
    main()
