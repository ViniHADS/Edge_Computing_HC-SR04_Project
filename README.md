# Sistema de Controle de Brilho por Distância

## Integrantes

- Gabriel Donato - RM  572320
- Jorge Meert - RM  572179
- Vinicius Araujo - RM  572666

## Descrição

Este projeto foi desenvolvido para o Checkpoint 1 do projeto semestral
da disciplina de Edge Computing.

O protótipo utiliza um Arduino Mega 2560 para realizar a leitura de
distância através de um sensor ultrassônico HC-SR04 e controlar a
intensidade de um LED utilizando PWM.

Um potenciômetro permite ao usuário configurar uma distância de
referência entre 10 cm e 100 cm.

A intensidade do LED é calculada de acordo com a diferença entre a
distância medida pelo sensor e a distância de referência escolhida
pelo usuário.

## Objetivo

O objetivo do projeto é demonstrar a integração entre aquisição de
dados, interação do usuário, processamento e atuação utilizando um
sistema embarcado baseado em Arduino.

O sistema possui:

- aquisição de distância pelo HC-SR04;
- configuração da referência através de potenciômetro;
- cálculo proporcional da intensidade;
- controle PWM do LED;
- validação das leituras;
- tratamento de ausência de eco;
- acompanhamento dos valores através do Monitor Serial.

## Componentes utilizados

| Componente | Quantidade | Função |
|---|---:|---|
| Arduino Mega 2560 | 1 | Processamento e controle |
| HC-SR04 | 1 | Medição de distância |
| Potenciômetro | 1 | Configuração da referência |
| LED | 1 | Feedback visual |
| Resistor | 1 | Limitação da corrente do LED |
| Jumpers | Vários | Conexões elétricas |

## Funcionamento

O funcionamento do sistema pode ser representado pelo seguinte fluxo:

HC-SR04
↓
Distância medida

Potenciômetro
↓
Distância de referência

Arduino Mega
↓
Comparação + cálculo PWM

LED
↓
Feedback visual

O HC-SR04 mede continuamente a distância de um objeto.

O potenciômetro fornece uma leitura analógica entre 0 e 1023.
O firmware converte esse valor para uma distância de referência
entre 10 cm e 100 cm.

O Arduino compara a distância medida com essa referência.

Quando o objeto está dentro da distância de referência, o brilho
do LED aumenta proporcionalmente à aproximação.

Quando o objeto está na referência ou além dela, o LED permanece
apagado.

## Sensor HC-SR04

O HC-SR04 utiliza dois sinais principais:

- TRIG: recebe um pulso do Arduino para iniciar a medição;
- ECHO: retorna um pulso cuja duração representa o tempo de
  percurso do sinal ultrassônico.

O firmware gera um pulso de aproximadamente 10 microssegundos no
TRIG e utiliza `pulseIn()` para medir a duração do ECHO.

A distância é calculada através da expressão:

distância = duração × 0.0343 / 2

A divisão por dois ocorre porque o tempo medido representa o
percurso de ida e volta do sinal ultrassônico.

## Potenciômetro

O potenciômetro está conectado à entrada analógica A0.

A função `analogRead()` fornece valores entre:

0 e 1023

O firmware utiliza `map()` para converter essa faixa para:

10 cm a 100 cm

Essa distância funciona como referência para o controle do LED.

## LED e PWM

O LED está conectado ao pino digital D9, utilizado como saída PWM.

O brilho é calculado através da relação:

brilho =
(referência - distância)
× 255
÷ referência

O resultado é limitado entre 0 e 255 utilizando `constrain()`.

Exemplo:

Referência = 100 cm
Distância = 20 cm

PWM ≈ 204

Referência = 100 cm
Distância = 50 cm

PWM ≈ 127

Referência = 100 cm
Distância = 100 cm

PWM = 0

## Tratamento das leituras

O projeto possui tratamento para falhas do HC-SR04.

### Timeout

A leitura do ECHO utiliza um timeout de:

30000 microssegundos

Se nenhum eco for detectado dentro desse período, `pulseIn()`
retorna zero.

O sistema então:

- considera a leitura inválida;
- desliga o LED;
- informa o erro no Monitor Serial.

### Faixa válida

O firmware considera válidas distâncias entre:

2 cm e 400 cm

Leituras fora dessa faixa são tratadas como inválidas.

Nesse caso o LED também permanece desligado.

## Monitor Serial

A comunicação Serial utiliza:

9600 baud

Durante a operação, o sistema exibe:

- distância medida;
- distância de referência;
- valor do PWM enviado ao LED.

Exemplo:

Distancia: 42.35 cm | Referencia: 70 cm | Brilho: 100

Em caso de ausência de eco:

Sem eco | Sensor sem leitura valida | Brilho: 0

## Pinagem

| Arduino Mega | Componente | Função |
|---|---|---|
| D4 | HC-SR04 TRIG | Pulso de disparo |
| D5 | HC-SR04 ECHO | Recepção do eco |
| A0 | Potenciômetro | Entrada analógica |
| D9 | LED | Saída PWM |
| 5V | HC-SR04/Potenciômetro | Alimentação |
| GND | Circuito | Referência elétrica |

## Como utilizar

1. Monte e alimente o circuito.
2. Execute o firmware no Arduino Mega 2560.
3. Abra o Monitor Serial configurado para 9600 baud.
4. Ajuste o potenciômetro para definir a distância de referência.
5. Posicione um objeto diante do HC-SR04.
6. Observe a distância medida no Monitor Serial.
7. Observe a distância de referência.
8. Observe o valor PWM calculado.
9. Observe a mudança de intensidade do LED conforme o objeto se aproxima.

## Simulação

O projeto também possui uma versão simulada utilizando a plataforma
Wokwi.

[Projeto no Wokwi](https://wokwi.com/projects/473274436180247553)

## Código-fonte

O firmware do Arduino está disponível em:

`COLOCAR_CAMINHO_DO_CODIGO_AQUI`
