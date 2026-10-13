"""Validação do mesmo contrato de telemetria e atraso usado pelo Mega."""


def valid_delay(value):
    # bool e float não são comandos inteiros, mesmo que o JSON os aceite.
    return type(value) is int and 100 <= value <= 60000


def valid_telemetry(data):
    fields = {"distancia_cm", "referencia_cm", "brilho", "status", "uptime_ms"}
    if not isinstance(data, dict) or set(data) not in (fields, fields | {"potenciometro", "habilitado", "comunicacao"}):
        return False
    for key, lower, upper in (("referencia_cm", 10, 100), ("brilho", 0, 255),
                              ("uptime_ms", 0, 4294967295)):
        if type(data[key]) is not int or not lower <= data[key] <= upper:
            return False
    if "potenciometro" in data:
        if (type(data["potenciometro"]) is not int or not 0 <= data["potenciometro"] <= 1023
                or type(data["habilitado"]) is not bool
                or data["comunicacao"] not in ("online", "ponte_offline", "mqtt_offline")
                or data["brilho"] not in (0, 255)
                or (not data["habilitado"] and data["brilho"] != 0)):
            return False
    if data["status"] == "sem_eco":
        # Presenca anterior pode manter o LED aceso na tolerancia de 300 ms.
        return data["distancia_cm"] is None and data["brilho"] in (0, 255)
    if data["status"] == "fora_faixa":
        return data["distancia_cm"] is None and data["brilho"] == 0
    distance = data["distancia_cm"]
    return (data["status"] == "ok" and type(distance) in (int, float)
            and 2 <= distance <= 400)
