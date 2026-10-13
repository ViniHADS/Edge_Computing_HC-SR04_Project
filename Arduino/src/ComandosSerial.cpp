#include "ComandosSerial.h"
#include "Configuracao.h"
#include "ControleBrilho.h"
#include "Comunicacao.h"
#include <string.h>

namespace {
char linha[48];
unsigned int tamanho = 0;
bool excedeu = false;
bool confirmar = false;
bool comandoLiga = false;
unsigned long comandoId = 0;

bool inteiro(const char *texto, unsigned long limite, unsigned long &numero) {
  numero = 0;
  if (!*texto) return false;
  for (; *texto; ++texto) {
    if (*texto < '0' || *texto > '9') return false;
    unsigned int digito = *texto - '0';
    if (numero > (limite - digito) / 10) return false;
    numero = numero * 10 + digito;
  }
  return numero <= limite;
}
void interpretar(unsigned long &atrasoMs) {
  if (!strcmp(linha, "hb|online") || !strcmp(linha, "hb|offline")) {
    Comunicacao::receberEstado(!strcmp(linha, "hb|online"));
    return;
  }
  const char *id = nullptr;
  if (!strncmp(linha, "on|", 3)) { comandoLiga = true; id = linha + 3; }
  else if (!strncmp(linha, "off|", 4)) { comandoLiga = false; id = linha + 4; }
  if (id && inteiro(id, 4294967295UL, comandoId) && comandoId != 0) {
    ControleBrilho::definirHabilitado(comandoLiga);
    confirmar = true;
    return;
  }
  unsigned long numero;
  if (!id && inteiro(linha, Configuracao::ATRASO_MAX_MS, numero) && numero >= Configuracao::ATRASO_MIN_MS) {
    atrasoMs = numero;
    Serial.print(F("Atraso em ms: ")); Serial.println(atrasoMs);
  } else Serial.println(F("Comando serial invalido"));
}
}
namespace ComandosSerial {
void processar(unsigned long &atrasoMs) {
  // Uma linha por ciclo; buffer limitado, sem esperar dados nem usar String.
  for (int i = 0; i < Configuracao::BYTES_COMANDO_POR_CICLO && Serial.available() > 0; ++i) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (tamanho || excedeu) {
        linha[tamanho] = '\0';
        if (!excedeu) interpretar(atrasoMs);
        else Serial.println(F("Comando serial muito longo"));
      }
      tamanho = 0; excedeu = false;
      return;
    }
    if (tamanho < sizeof(linha) - 1 && !excedeu) linha[tamanho++] = c;
    else excedeu = true;
  }
}
void confirmarAplicacao() {
  // O .ino chama somente depois de aplicar PWM, evitando ACK antes da atuacao.
  if (!confirmar) return;
  Serial.print(F("{\"comando\":\"")); Serial.print(comandoLiga ? "on" : "off");
  Serial.print(F("\",\"id\":")); Serial.print(comandoId);
  Serial.println(F(",\"resultado\":\"OK\"}"));
  confirmar = false;
}
}
