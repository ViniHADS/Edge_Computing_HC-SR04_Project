#include "Comunicacao.h"
#include "Configuracao.h"
#include <Arduino.h>
namespace {
bool recebeuHeartbeat = false;
bool mqttConectado = false;
unsigned long ultimoHeartbeat = 0;
}
namespace Comunicacao {
void iniciar() { recebeuHeartbeat = false; mqttConectado = false; ultimoHeartbeat = 0; }
void receberEstado(bool conectado) {
  recebeuHeartbeat = true;
  mqttConectado = conectado;
  ultimoHeartbeat = millis();
}
const char *estado() {
  if (!recebeuHeartbeat || millis() - ultimoHeartbeat >= Configuracao::TIMEOUT_COMUNICACAO_MS)
    return "ponte_offline";
  return mqttConectado ? "online" : "mqtt_offline";
}
}
