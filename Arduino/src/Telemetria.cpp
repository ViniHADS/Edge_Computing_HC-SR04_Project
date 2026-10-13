#include "Telemetria.h"
#include "Configuracao.h"
#include "ControleBrilho.h"
#include "Comunicacao.h"

namespace
{
// Os nomes e a ordem dos campos JSON fazem parte do protocolo da ponte.
const char *nomeEstado(EstadoDistancia estado)
{
  switch (estado)
  {
    case EstadoDistancia::Ok: return "ok";
    case EstadoDistancia::SemEco: return "sem_eco";
    case EstadoDistancia::ForaFaixa: return "fora_faixa";
  }
  return "fora_faixa";
}

void imprimirLog(const LeituraDistancia &leitura, int referenciaCm, int brilho)
{
  if (leitura.estado == EstadoDistancia::SemEco)
  {
    Serial.print("Sem eco | Sensor sem leitura valida | Brilho: ");
    Serial.println(brilho);
  }
  else if (leitura.estado == EstadoDistancia::ForaFaixa)
  {
    Serial.print("Leitura invalida: ");
    Serial.print(leitura.centimetros);
    Serial.println(" cm | Brilho: 0");
  }
  else
  {
    Serial.print("Distancia: ");
    Serial.print(leitura.centimetros, 2);
    Serial.print(" cm | Referencia: ");
    Serial.print(referenciaCm);
    Serial.print(" cm | Brilho: ");
    Serial.println(brilho);
  }
}
}

namespace Telemetria
{
void iniciar()
{
  Serial.begin(Configuracao::BAUD_SERIAL);
  Serial.println("======================================");
  Serial.println("Sistema de Indicacao de Presenca");
  Serial.println("HC-SR04 + Potenciometro + LED PWM");
  Serial.println("======================================");
  Serial.println("Watchdog: 1 segundo | Atraso inicial: 100 ms");
  Serial.println("Envie um inteiro de 100 a 60000 ms seguido de Enter.");
  Serial.println("Para testar o reset, envie 2000. Apos reiniciar, volta a 100 ms.");
  Serial.flush(); // Mensagens iniciais terminam antes de ativar o watchdog.
}

void publicar(const LeituraDistancia &leitura, int referenciaCm, int brilho)
{
  imprimirLog(leitura, referenciaCm, brilho);

  // Escreve diretamente na serial, sem montar String dinamica na RAM do AVR.
  Serial.print(F("{\"distancia_cm\":"));
  if (leitura.estado != EstadoDistancia::Ok)
  {
    Serial.print(F("null"));
  }
  else
  {
    Serial.print(leitura.centimetros, 2);
  }
  Serial.print(F(",\"referencia_cm\":"));
  Serial.print(referenciaCm);
  Serial.print(F(",\"brilho\":"));
  Serial.print(brilho);
  Serial.print(F(",\"status\":\""));
  Serial.print(nomeEstado(leitura.estado));
  Serial.print(F("\",\"uptime_ms\":"));
  Serial.print(millis());
  Serial.print(F(",\"potenciometro\":"));
  Serial.print(ControleBrilho::lerPotenciometroBruto());
  Serial.print(F(",\"habilitado\":"));
  Serial.print(ControleBrilho::habilitado() ? F("true") : F("false"));
  Serial.print(F(",\"comunicacao\":\""));
  Serial.print(Comunicacao::estado());
  Serial.println(F("\"}"));
}
}
