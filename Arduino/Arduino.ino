#include "src/Configuracao.h"
#include "src/SensorDistancia.h"
#include "src/ControleBrilho.h"
#include "src/ComandosSerial.h"
#include "src/Telemetria.h"
#include "src/Supervisao.h"
#include "src/Comunicacao.h"

namespace
{
// Unico estado da aplicacao: o atraso pode ser alterado pela serial/MQTT.
unsigned long atrasoMs = Configuracao::ATRASO_INICIAL_MS;
}

void setup()
{
  // Prepara as saidas em estado seguro e conclui os logs antes do watchdog.
  SensorDistancia::iniciar();
  ControleBrilho::iniciar();
  Telemetria::iniciar();
  Comunicacao::iniciar();
  Supervisao::iniciar();
}

void loop()
{
  // Recebe comandos sem esperar uma linha completa.
  ComandosSerial::processar(atrasoMs);

  // Atualiza a referencia mesmo sem eco e aplica o resultado da medicao.
  int referenciaCm = ControleBrilho::lerReferencia();
  LeituraDistancia leitura = SensorDistancia::ler();
  int brilho = ControleBrilho::calcular(leitura, referenciaCm);
  ControleBrilho::aplicar(brilho);
  ComandosSerial::confirmarAplicacao();

  // Publicacao e supervisao usam o mesmo fluxo em leituras validas e invalidas.
  Telemetria::publicar(leitura, referenciaCm, brilho);
  Supervisao::finalizarCiclo(atrasoMs);
}
