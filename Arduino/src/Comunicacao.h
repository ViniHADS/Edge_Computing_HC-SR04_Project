#pragma once
namespace Comunicacao {
void iniciar();
void receberEstado(bool conectado);
// Heartbeat distingue falha da ponte/MQTT de falha do sensor.
const char *estado();
}
