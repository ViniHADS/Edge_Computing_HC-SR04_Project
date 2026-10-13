#include "ControleBrilho.h"
#include "Configuracao.h"

namespace
{
bool presencaConfirmada = false;
bool controleHabilitado = true;
int potenciometroBruto = 0;
unsigned long ultimaPresencaMs = 0;
}

namespace ControleBrilho
{
void iniciar()
{
  pinMode(Configuracao::PINO_LED, OUTPUT);
  presencaConfirmada = false;
  controleHabilitado = true;
  potenciometroBruto = 0;
  ultimaPresencaMs = 0;
  aplicar(0);
}

int lerReferencia()
{
  // O potenciometro ajusta o limite de deteccao de presenca.
  int valor = analogRead(Configuracao::PINO_POTENCIOMETRO);
  potenciometroBruto = valor;
  int referencia = map(valor, 0, Configuracao::ADC_MAX,
                       Configuracao::REFERENCIA_MIN_CM,
                       Configuracao::REFERENCIA_MAX_CM);
  return constrain(referencia, Configuracao::REFERENCIA_MIN_CM,
                   Configuracao::REFERENCIA_MAX_CM);
}

int calcular(const LeituraDistancia &leitura, int referenciaCm)
{
  unsigned long agora = millis();
  if (leitura.estado == EstadoDistancia::Ok)
  {
    // Presenca valida acende com brilho constante; fora do limite apaga.
    presencaConfirmada = leitura.centimetros < referenciaCm;
    if (presencaConfirmada)
      ultimaPresencaMs = agora;
  }
  else if (leitura.estado == EstadoDistancia::ForaFaixa ||
           static_cast<unsigned long>(agora - ultimaPresencaMs) >=
             Configuracao::TOLERANCIA_SEM_ECO_MS)
  {
    // Somente sem eco temporario preserva a presenca anterior.
    // A diferenca de tempos continua correta quando millis() volta a zero.
    presencaConfirmada = false;
  }
  return controleHabilitado && presencaConfirmada ? Configuracao::PWM_MAX : 0;
}

int lerPotenciometroBruto() { return potenciometroBruto; }
void definirHabilitado(bool valor) { controleHabilitado = valor; }
bool habilitado() { return controleHabilitado; }

void aplicar(int brilho)
{
  analogWrite(Configuracao::PINO_LED, brilho);
}
}
