#pragma once

#include <Arduino.h>

// Parametros do Mega: mantenha pinos e limites de hardware neste arquivo.
namespace Configuracao
{
constexpr int PINO_TRIGGER = 4;
constexpr int PINO_ECHO = 5;
constexpr int PINO_POTENCIOMETRO = A0;
constexpr int PINO_LED = 9;
constexpr unsigned long BAUD_SERIAL = 9600;

// O timeout limita o bloqueio de pulseIn quando nao ha eco.
constexpr unsigned long TIMEOUT_ECHO_US = 30000UL;
constexpr float DISTANCIA_MIN_CM = 2.0;
constexpr float DISTANCIA_MAX_CM = 400.0;
constexpr int REFERENCIA_MIN_CM = 10;
constexpr int REFERENCIA_MAX_CM = 100;
constexpr int ADC_MAX = 1023;
constexpr int PWM_MAX = 255;

// Evita apagar o LED por uma falha breve de eco; falhas persistentes apagam.
constexpr unsigned long TOLERANCIA_SEM_ECO_MS = 300;
constexpr unsigned long TIMEOUT_COMUNICACAO_MS = 5000;

// Atrasos longos continuam permitidos para a demonstracao do watchdog.
constexpr unsigned long ATRASO_INICIAL_MS = 100;
constexpr unsigned long ATRASO_MIN_MS = 100;
constexpr unsigned long ATRASO_MAX_MS = 60000;
constexpr int BYTES_COMANDO_POR_CICLO = 32;
}
