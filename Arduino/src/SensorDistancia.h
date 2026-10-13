#pragma once

// O estado separa uma medida valida de falhas; nenhuma falha vira distancia zero.
enum class EstadoDistancia { Ok, SemEco, ForaFaixa };

struct LeituraDistancia
{
  float centimetros;
  EstadoDistancia estado;
};

namespace SensorDistancia
{
void iniciar();
LeituraDistancia ler();
}
