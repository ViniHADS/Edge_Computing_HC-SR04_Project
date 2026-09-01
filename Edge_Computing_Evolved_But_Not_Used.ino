// ============================================================
// DISTANCE CONTROL SYSTEM
// Arduino Mega 2560 + HC-SR04 + KY-040 + RGB LED + Buzzer
// ============================================================

// ------------------------- Pinos -----------------------------
constexpr uint8_t PIN_TRIG = 4;
constexpr uint8_t PIN_ECHO = 5;

constexpr uint8_t PIN_RGB_R = 6;   // PWM
constexpr uint8_t PIN_RGB_G = 7;   // PWM
constexpr uint8_t PIN_RGB_B = 8;   // PWM

constexpr uint8_t PIN_BUZZER = 10;

constexpr uint8_t PIN_ENCODER_CLK = 2; // INT
constexpr uint8_t PIN_ENCODER_DT  = 3; // INT
constexpr uint8_t PIN_ENCODER_SW  = 11;

// ---------------------- Configurações ------------------------
constexpr unsigned long ECHO_TIMEOUT_US = 25000UL;
constexpr unsigned long SENSOR_INTERVAL_MS = 60UL;
constexpr unsigned long SERIAL_INTERVAL_MS = 500UL;
constexpr unsigned long BUTTON_DEBOUNCE_MS = 50UL;

constexpr int REFERENCIA_MIN_CM = 10;
constexpr int REFERENCIA_MAX_CM = 100;

// Abaixo de 50% da referência = estado crítico
constexpr float LIMIAR_CRITICO = 0.50f;

// Buzzer - atenção
constexpr unsigned long BUZZER_ATENCAO_INTERVAL_MS = 1000UL;
constexpr unsigned long BUZZER_ATENCAO_DURATION_MS = 120UL;
constexpr unsigned int FREQ_BUZZER_ATENCAO = 1000U;

// Buzzer - crítico
constexpr unsigned long BUZZER_CRITICO_INTERVAL_MS = 250UL;
constexpr unsigned long BUZZER_CRITICO_DURATION_MS = 100UL;
constexpr unsigned int FREQ_BUZZER_CRITICO = 2000U;

// -------------------------- Estados ---------------------------
enum class EstadoSistema {
  SEGURO,
  ATENCAO,
  CRITICO
};

// ---------------- Variáveis do sistema -----------------------
volatile int referenciaCm = 70;
volatile bool referenciaAlterada = false;

float distanciaCm = 0.0f;
bool sensorValido = false;

int brilhoAtual = 0;
EstadoSistema estadoAtual = EstadoSistema::SEGURO;

// ---------------- Encoder / botão ----------------------------
bool estadoBotaoAtual = HIGH;
bool ultimaLeituraBotao = HIGH;
unsigned long ultimaMudancaBotaoMs = 0;

// ----------------------- Buzzer -------------------------------
bool buzzerAtivo = false;
unsigned long inicioBuzzerMs = 0;
unsigned long ultimaAcaoBuzzerMs = 0;
EstadoSistema estadoAnteriorBuzzer = EstadoSistema::SEGURO;

// ------------------- Temporização -----------------------------
unsigned long ultimaLeituraSensorMs = 0;
unsigned long ultimoSerialMs = 0;

// ============================================================
// UTILIDADES
// ============================================================

int obterReferenciaComSeguranca() {
  noInterrupts();
  int referencia = referenciaCm;
  interrupts();

  return referencia;
}

const char* nomeEstado(EstadoSistema estado) {

  switch (estado) {

    case EstadoSistema::SEGURO:
      return "SEGURO";

    case EstadoSistema::ATENCAO:
      return "ATENCAO";

    case EstadoSistema::CRITICO:
      return "CRITICO";
  }

  return "SEGURO";
}

const char* nomeCor(EstadoSistema estado) {

  switch (estado) {

    case EstadoSistema::SEGURO:
      return "VERDE";

    case EstadoSistema::ATENCAO:
      return "AMARELO";

    case EstadoSistema::CRITICO:
      return "VERMELHO";
  }

  return "VERDE";
}

// ============================================================
// CONFIGURAÇÃO
// ============================================================

void configurarPinos() {

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);

  pinMode(PIN_RGB_R, OUTPUT);
  pinMode(PIN_RGB_G, OUTPUT);
  pinMode(PIN_RGB_B, OUTPUT);

  pinMode(PIN_BUZZER, OUTPUT);

  pinMode(PIN_ENCODER_CLK, INPUT);
  pinMode(PIN_ENCODER_DT, INPUT);
  pinMode(PIN_ENCODER_SW, INPUT_PULLUP);

  digitalWrite(PIN_TRIG, LOW);

  analogWrite(PIN_RGB_R, 0);
  analogWrite(PIN_RGB_G, 0);
  analogWrite(PIN_RGB_B, 0);

  noTone(PIN_BUZZER);
}

void inicializarSerial() {

  Serial.begin(9600);
}

void inicializarSistema() {

  attachInterrupt(
    digitalPinToInterrupt(PIN_ENCODER_CLK),
    processarEncoderISR,
    FALLING
  );

  ultimaLeituraBotao = digitalRead(PIN_ENCODER_SW);
  estadoBotaoAtual = ultimaLeituraBotao;

  ultimaLeituraSensorMs =
    millis() - SENSOR_INTERVAL_MS;

  ultimoSerialMs =
    millis() - SERIAL_INTERVAL_MS;

  ultimaAcaoBuzzerMs = millis();

  Serial.println();
  Serial.println("========================================");
  Serial.println("       DISTANCE CONTROL SYSTEM");
  Serial.println("========================================");
  Serial.println("Sistema iniciado.");
  Serial.println("Encoder: ajuste a distancia de referencia.");
  Serial.println("Botao do encoder: confirma a referencia.");
  Serial.println();
}

// ============================================================
// ENCODER
// ============================================================

void processarEncoderISR() {

  int dt = digitalRead(PIN_ENCODER_DT);

  if (dt == HIGH) {
    referenciaCm++;
  } else {
    referenciaCm--;
  }

  if (referenciaCm > REFERENCIA_MAX_CM) {
    referenciaCm = REFERENCIA_MAX_CM;
  }

  if (referenciaCm < REFERENCIA_MIN_CM) {
    referenciaCm = REFERENCIA_MIN_CM;
  }

  referenciaAlterada = true;
}

void processarEncoder() {

  bool houveAlteracao;

  noInterrupts();

  houveAlteracao = referenciaAlterada;
  referenciaAlterada = false;

  int referencia = referenciaCm;

  interrupts();

  if (houveAlteracao) {

    Serial.print("[ENCODER] Referencia ajustada para ");
    Serial.print(referencia);
    Serial.println(" cm");
  }
}

// ============================================================
// BOTÃO DO ENCODER
// ============================================================

void processarBotaoEncoder() {

  bool leitura = digitalRead(PIN_ENCODER_SW);

  unsigned long agora = millis();

  if (leitura != ultimaLeituraBotao) {

    ultimaMudancaBotaoMs = agora;
    ultimaLeituraBotao = leitura;
  }

  if (
    (agora - ultimaMudancaBotaoMs) >=
      BUTTON_DEBOUNCE_MS
    &&
    leitura != estadoBotaoAtual
  ) {

    estadoBotaoAtual = leitura;

    if (estadoBotaoAtual == LOW) {

      int referencia =
        obterReferenciaComSeguranca();

      Serial.println();
      Serial.println(
        "[ENCODER] Referencia confirmada"
      );

      Serial.print(
        "Referencia atual: "
      );

      Serial.print(referencia);
      Serial.println(" cm");

      Serial.println();
    }
  }
}

// ============================================================
// HC-SR04
// ============================================================

bool lerDistancia(float& distancia) {

  unsigned long agora = millis();

  if (
    (agora - ultimaLeituraSensorMs)
    < SENSOR_INTERVAL_MS
  ) {
    return false;
  }

  ultimaLeituraSensorMs = agora;

  digitalWrite(PIN_TRIG, LOW);

  delayMicroseconds(2);

  digitalWrite(PIN_TRIG, HIGH);

  delayMicroseconds(10);

  digitalWrite(PIN_TRIG, LOW);

  unsigned long duracao =
    pulseIn(
      PIN_ECHO,
      HIGH,
      ECHO_TIMEOUT_US
    );

  if (duracao == 0) {

    sensorValido = false;
    distancia = 0.0f;

    return true;
  }

  float distanciaCalculada =
    duracao * 0.0343f / 2.0f;

  if (
    distanciaCalculada < 2.0f ||
    distanciaCalculada > 400.0f
  ) {

    sensorValido = false;
    distancia = 0.0f;

    return true;
  }

  distancia = distanciaCalculada;
  sensorValido = true;

  return true;
}

// ============================================================
// CÁLCULO
// ============================================================

int calcularBrilho(
  float distancia,
  int referencia
) {

  if (
    !sensorValido ||
    referencia <= 0 ||
    distancia >= referencia
  ) {
    return 0;
  }

  float brilhoFloat =
    (referencia - distancia)
    * 255.0f
    / referencia;

  brilhoFloat =
    constrain(
      brilhoFloat,
      0.0f,
      255.0f
    );

  return static_cast<int>(
    brilhoFloat
  );
}

EstadoSistema determinarEstado(
  float distancia,
  int referencia
) {

  if (
    !sensorValido ||
    distancia >= referencia
  ) {
    return EstadoSistema::SEGURO;
  }

  if (
    distancia >
    (referencia * LIMIAR_CRITICO)
  ) {
    return EstadoSistema::ATENCAO;
  }

  return EstadoSistema::CRITICO;
}

void calcularEstado() {

  int referencia =
    obterReferenciaComSeguranca();

  if (!sensorValido) {

    estadoAtual =
      EstadoSistema::SEGURO;

    brilhoAtual = 0;

    return;
  }

  brilhoAtual =
    calcularBrilho(
      distanciaCm,
      referencia
    );

  estadoAtual =
    determinarEstado(
      distanciaCm,
      referencia
    );
}

// ============================================================
// LED RGB
// ============================================================

void atualizarLED() {

  int brilho = brilhoAtual;

  switch (estadoAtual) {

    case EstadoSistema::SEGURO:

      // Verde
      analogWrite(
        PIN_RGB_R,
        0
      );

      analogWrite(
        PIN_RGB_G,
        brilho
      );

      analogWrite(
        PIN_RGB_B,
        0
      );

      break;

    case EstadoSistema::ATENCAO:

      // Amarelo = vermelho + verde
      analogWrite(
        PIN_RGB_R,
        brilho
      );

      analogWrite(
        PIN_RGB_G,
        brilho
      );

      analogWrite(
        PIN_RGB_B,
        0
      );

      break;

    case EstadoSistema::CRITICO:

      // Vermelho
      analogWrite(
        PIN_RGB_R,
        brilho
      );

      analogWrite(
        PIN_RGB_G,
        0
      );

      analogWrite(
        PIN_RGB_B,
        0
      );

      break;
  }
}

// ============================================================
// BUZZER
// ============================================================

void desligarBuzzer() {

  if (buzzerAtivo) {

    noTone(PIN_BUZZER);

    buzzerAtivo = false;
  }
}

void atualizarBuzzer() {

  unsigned long agora = millis();

  if (
    estadoAtual ==
    EstadoSistema::SEGURO
  ) {

    desligarBuzzer();

    estadoAnteriorBuzzer =
      estadoAtual;

    return;
  }

  unsigned long intervalo;
  unsigned long duracao;
  unsigned int frequencia;

  if (
    estadoAtual ==
    EstadoSistema::ATENCAO
  ) {

    intervalo =
      BUZZER_ATENCAO_INTERVAL_MS;

    duracao =
      BUZZER_ATENCAO_DURATION_MS;

    frequencia =
      FREQ_BUZZER_ATENCAO;

  } else {

    intervalo =
      BUZZER_CRITICO_INTERVAL_MS;

    duracao =
      BUZZER_CRITICO_DURATION_MS;

    frequencia =
      FREQ_BUZZER_CRITICO;
  }

  if (
    estadoAtual !=
    estadoAnteriorBuzzer
  ) {

    desligarBuzzer();

    tone(
      PIN_BUZZER,
      frequencia
    );

    buzzerAtivo = true;

    inicioBuzzerMs = agora;
    ultimaAcaoBuzzerMs = agora;

    estadoAnteriorBuzzer =
      estadoAtual;

    return;
  }

  if (buzzerAtivo) {

    if (
      (agora - inicioBuzzerMs)
      >= duracao
    ) {

      noTone(PIN_BUZZER);

      buzzerAtivo = false;
    }

  } else if (
    (agora - ultimaAcaoBuzzerMs)
    >= intervalo
  ) {

    tone(
      PIN_BUZZER,
      frequencia
    );

    buzzerAtivo = true;

    inicioBuzzerMs = agora;
    ultimaAcaoBuzzerMs = agora;
  }
}

// ============================================================
// MONITOR SERIAL
// ============================================================

void imprimirCabecalho() {

  Serial.println(
    "========================================"
  );

  Serial.println(
    "       DISTANCE CONTROL SYSTEM"
  );

  Serial.println(
    "========================================"
  );
}

void atualizarMonitorSerial() {

  unsigned long agora = millis();

  if (
    (agora - ultimoSerialMs)
    < SERIAL_INTERVAL_MS
  ) {
    return;
  }

  ultimoSerialMs = agora;

  int referencia =
    obterReferenciaComSeguranca();

  imprimirCabecalho();

  if (sensorValido) {

    Serial.print(
      "Distancia : "
    );

    Serial.print(
      distanciaCm,
      2
    );

    Serial.println(
      " cm"
    );

  } else {

    Serial.println(
      "Distancia : SEM LEITURA VALIDA"
    );
  }

  Serial.print(
    "Referencia: "
  );

  Serial.print(
    referencia
  );

  Serial.println(
    " cm"
  );

  Serial.print(
    "Brilho    : "
  );

  Serial.print(
    brilhoAtual
  );

  Serial.println(
    " / 255"
  );

  Serial.print(
    "Cor       : "
  );

  Serial.println(
    nomeCor(
      estadoAtual
    )
  );

  Serial.print(
    "Estado    : "
  );

  Serial.println(
    nomeEstado(
      estadoAtual
    )
  );

  Serial.print(
    "Buzzer    : "
  );

  Serial.println(
    buzzerAtivo
      ? "ATIVO"
      : "DESLIGADO"
  );

  Serial.println(
    "========================================"
  );

  Serial.println();
}

// ============================================================
// SETUP
// ============================================================

void setup() {

  configurarPinos();

  inicializarSerial();

  inicializarSistema();
}

// ============================================================
// LOOP
// ============================================================

void loop() {

  lerDistancia(
    distanciaCm
  );

  processarEncoder();

  processarBotaoEncoder();

  calcularEstado();

  atualizarLED();

  atualizarBuzzer();

  atualizarMonitorSerial();
}