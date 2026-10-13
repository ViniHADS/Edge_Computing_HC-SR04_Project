#include "SensorDistancia.h"
#include "Configuracao.h"

namespace SensorDistancia
{
void iniciar()
{
  pinMode(Configuracao::PINO_TRIGGER, OUTPUT);
  pinMode(Configuracao::PINO_ECHO, INPUT);
  digitalWrite(Configuracao::PINO_TRIGGER, LOW);
}

LeituraDistancia ler()
{
  // Dispara o HC-SR04 com um pulso de 10 us, partindo de nivel baixo.
  digitalWrite(Configuracao::PINO_TRIGGER, LOW);
  delayMicroseconds(2);
  digitalWrite(Configuracao::PINO_TRIGGER, HIGH);
  delayMicroseconds(10);
  digitalWrite(Configuracao::PINO_TRIGGER, LOW);

  // Sem eco completo dentro do prazo, informa falha ao controle e telemetria.
  unsigned long duracao = pulseIn(Configuracao::PINO_ECHO, HIGH,
                                 Configuracao::TIMEOUT_ECHO_US);
  if (duracao == 0)
  {
    return {-1.0, EstadoDistancia::SemEco};
  }

  // O som percorre ida e volta: converte us em cm e divide por dois.
  float distancia = duracao * 0.0343 / 2.0;
  if (distancia < Configuracao::DISTANCIA_MIN_CM ||
      distancia > Configuracao::DISTANCIA_MAX_CM)
  {
    return {distancia, EstadoDistancia::ForaFaixa};
  }
  return {distancia, EstadoDistancia::Ok};
}
}
