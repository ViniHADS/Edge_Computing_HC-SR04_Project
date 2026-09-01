#define pino_trigger 4
#define pino_echo 5
#define potenciometro A0
#define led 9

// Timeout máximo da leitura do HC-SR04.
// 30000 us corresponde a aproximadamente 5 metros,
// portanto é mais que suficiente para este projeto.
#define TIMEOUT_ECHO 30000UL

void setup()
{
  Serial.begin(9600);

  // HC-SR04
  pinMode(pino_trigger, OUTPUT);
  pinMode(pino_echo, INPUT);

  // LED
  pinMode(led, OUTPUT);

  // Garante que o trigger comece desligado
  digitalWrite(pino_trigger, LOW);

  // LED começa desligado
  analogWrite(led, 0);

  Serial.println("======================================");
  Serial.println("Sistema de Controle de Brilho");
  Serial.println("HC-SR04 + Potenciometro + LED PWM");
  Serial.println("======================================");
}

void loop()
{
  // =================================
  // LEITURA DO SENSOR ULTRASSÔNICO
  // =================================

  digitalWrite(pino_trigger, LOW);
  delayMicroseconds(2);

  digitalWrite(pino_trigger, HIGH);
  delayMicroseconds(10);

  digitalWrite(pino_trigger, LOW);

  // Aguarda o retorno do eco, mas com timeout.
  unsigned long duracao =
    pulseIn(pino_echo, HIGH, TIMEOUT_ECHO);

  // =================================
  // VERIFICAÇÃO DA LEITURA
  // =================================

  if (duracao == 0)
  {
    // Nenhum eco recebido.
    // Não existe uma distância válida.
    analogWrite(led, 0);

    Serial.println(
      "Sem eco | Sensor sem leitura valida | Brilho: 0"
    );

    delay(100);

    return;
  }

  // =================================
  // CONVERSÃO PARA CENTÍMETROS
  // =================================

  // Velocidade aproximada do som:
  // 0.0343 cm/us

  float distanciaCm =
    duracao * 0.0343 / 2.0;

  // Verifica se a distância está dentro
  // de uma faixa razoável para o HC-SR04.
  if (distanciaCm < 2.0 || distanciaCm > 400.0)
  {
    analogWrite(led, 0);

    Serial.print("Leitura invalida: ");
    Serial.print(distanciaCm);
    Serial.println(" cm | Brilho: 0");

    delay(100);

    return;
  }

  // =================================
  // LEITURA DO POTENCIÔMETRO
  // =================================

  int valorPot = analogRead(potenciometro);

  // Converte 0-1023 para 10-100 cm.
  int distanciaReferencia =
    map(
      valorPot,
      0,
      1023,
      10,
      100
    );

  // Proteção adicional.
  distanciaReferencia =
    constrain(
      distanciaReferencia,
      10,
      100
    );

  // =================================
  // CÁLCULO DO BRILHO
  // =================================

  float brilhoFloat =
    (distanciaReferencia - distanciaCm)
    * 255.0
    / distanciaReferencia;

  // Limita entre 0 e 255.
  int brilho =
    constrain(
      (int)brilhoFloat,
      0,
      255
    );

  // =================================
  // CONTROLE DO LED
  // =================================

  analogWrite(led, brilho);

  // =================================
  // MONITOR SERIAL
  // =================================

  Serial.print("Distancia: ");
  Serial.print(distanciaCm, 2);

  Serial.print(" cm | Referencia: ");
  Serial.print(distanciaReferencia);

  Serial.print(" cm | Brilho: ");
  Serial.println(brilho);

  delay(100);
}