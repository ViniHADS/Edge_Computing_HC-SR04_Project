#pragma once

#include "SensorDistancia.h"

namespace Telemetria
{
void iniciar();
// Emite log legivel e JSON compacto no contrato esperado pela ponte MQTT.
void publicar(const LeituraDistancia &leitura, int referenciaCm, int brilho);
}
