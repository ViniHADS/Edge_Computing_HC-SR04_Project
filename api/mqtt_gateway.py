"""Transporte MQTT: reconexão, última amostra e envio sem retenção."""
from datetime import datetime, timezone
import json
import os
import threading
import time

import paho.mqtt.client as mqtt

from .protocol import valid_telemetry


class MqttGateway:
    def __init__(self, port=1883, prefix="/TEF/exemplo001"):
        self.port = port
        self.topic = prefix + "/attrs/telemetria"
        self.command_topic = prefix + "/cmd"
        self._lock = threading.Lock()
        self._ready = threading.Event()
        self._sample = None
        self._received_at = None
        self._received_clock = None
        self.client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2,
                                  client_id=f"mega-http-{os.getpid()}")
        self.client.on_connect = self._on_connect
        self.client.on_subscribe = self._on_subscribe
        self.client.on_disconnect = self._on_disconnect
        self.client.on_message = self._on_message
        self.client.reconnect_delay_set(min_delay=1, max_delay=5)

    def start(self):
        # Não bloqueia o HTTP caso o broker ainda não tenha sido iniciado.
        self.client.connect_async("127.0.0.1", self.port, keepalive=10)
        self.client.loop_start()

    def close(self):
        self._ready.clear()
        self.client.disconnect()
        self.client.loop_stop()

    def _on_connect(self, client, userdata, flags, reason_code, properties):
        self._ready.clear()
        if not reason_code.is_failure:
            client.subscribe(self.topic, qos=0)

    def _on_subscribe(self, client, userdata, mid, reasons, properties):
        if reasons and all(not code.is_failure for code in reasons):
            self._ready.set()

    def _on_disconnect(self, client, userdata, flags, reason_code, properties):
        self._ready.clear()

    def _on_message(self, client, userdata, message):
        # Somente amostras válidas e novas entram no cache; retidas são ignoradas.
        if message.topic != self.topic or message.retain or len(message.payload) > 512:
            return
        try:
            sample = json.loads(message.payload)
        except (ValueError, UnicodeError):
            return
        if not valid_telemetry(sample):
            return
        with self._lock:
            self._sample = sample
            self._received_at = datetime.now(timezone.utc).isoformat()
            self._received_clock = time.monotonic()

    def ready(self):
        return self._ready.is_set() and self.client.is_connected()

    def snapshot(self):
        with self._lock:
            if self._sample is None:
                return None
            age = time.monotonic() - self._received_clock
            return {"dados": dict(self._sample), "recebido_em": self._received_at,
                    "idade_segundos": round(age, 3), "mqtt_conectado": self.ready(),
                    "desatualizada": age > 5 or not self.ready(),
                    "origem": "arduino" if "potenciometro" in self._sample else "mock_ou_legado",
                    "comunicacao_dispositivo": self._sample.get("comunicacao", "nao_informada") if age <= 5 else "sem_telemetria"}

    def publish_delay(self, delay_ms):
        if not self.ready():
            raise ConnectionError("Broker MQTT indisponivel")
        # QoS 0, não retido: não acumula comandos offline para repetir resets.
        info = self.client.publish(self.command_topic, str(delay_ms), qos=0, retain=False)
        if info.rc != mqtt.MQTT_ERR_SUCCESS:
            raise ConnectionError("Falha ao publicar comando MQTT")
        try:
            info.wait_for_publish(timeout=2)
            if not info.is_published():
                raise ConnectionError("Tempo de envio MQTT esgotado; entrega incerta")
        except (RuntimeError, ValueError) as error:
            raise ConnectionError("Envio MQTT interrompido; entrega incerta") from error
