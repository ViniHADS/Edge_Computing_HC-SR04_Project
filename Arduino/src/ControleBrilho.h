#pragma once

#include "SensorDistancia.h"

namespace ControleBrilho
{
void iniciar();
int lerReferencia();
// Retorna 255 com presenca, tolerando sem eco por menos de 300 ms, ou zero.
int calcular(const LeituraDistancia &leitura, int referenciaCm);
void aplicar(int brilho);
int lerPotenciometroBruto();
void definirHabilitado(bool valor);
bool habilitado();
}
