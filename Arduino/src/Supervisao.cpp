#include "Supervisao.h"
#include <Arduino.h>
#include <Watchdog.h>

#if defined(__AVR__)
#include <avr/wdt.h>
// Apos reset por watchdog, AVR pode voltar com WDT ativo em prazo muito curto.
// Limpa WDRF e desativa WDT antes de setup/Serial; iniciar() reativa em 1 s.
// Nao modifica o bootloader, a temporizacao normal ou o teste com 2000 ms.
extern "C" void prepararWatchdogNoBoot(void)
  __attribute__((naked, section(".init3"), used));
extern "C" void prepararWatchdogNoBoot(void)
{
  MCUSR = 0;
  wdt_disable();
}
#endif

namespace
{
Watchdog watchdog; // Somente este modulo acessa o watchdog de hardware.
}

namespace Supervisao
{
void iniciar()
{
  watchdog.enable(Watchdog::TIMEOUT_1S);
}

void finalizarCiclo(unsigned long atrasoMs)
{
  // Alimentar depois do delay preserva o teste de reset com o comando 2000.
  // Este caminho e executado tambem quando o sensor nao tem leitura valida.
  delay(atrasoMs);
  watchdog.reset();
}
}
