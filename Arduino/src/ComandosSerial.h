#pragma once

namespace ComandosSerial
{
// Consome somente os bytes disponiveis; uma linha completa altera atrasoMs.
void processar(unsigned long &atrasoMs);
void confirmarAplicacao();
}
