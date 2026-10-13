# Ponte serial–MQTT / CP2

A aplicação C++ roda no computador. Consulte o [README principal](../../README.md)
para preparação, protocolo completo, erros e comandos.

```sh
cmake --build ESP32/ESP32_mqtt_mock/build
./ESP32/ESP32_mqtt_mock/build/ESP32Mock --serial /dev/cu.usbserial-120 --fiware --broker localhost --port 1883
```

O firmware está em Arduino/Arduino.ino e Arduino/src. Serial 9600 8N1, LF/CRLF,
JSON compacto v2 (ADC, habilitado e comunicacao). O JSON v1 permanece aceito
para mocks antigos. A ponte lê continuamente e publica no máximo a cada segundo.

On/off envia `on|ID`/`off|ID` ao Mega e espera ACK após PWM aplicado. Só confirma
nome/ID correspondente; timeout 5 s gera ERROR. Heartbeat hb|online/offline
informa a saúde MQTT ao firmware enquanto a telemetria serial é recente. Após
mais de 5 s sem telemetria, a ponte suspende heartbeat para permitir recuperação
do bootloader após watchdog; a primeira leitura válida retoma o envio. USB
perdida gera comunicação de erro e saída.

Paho C 1.3.15/C++ 1.5.3 é preparado por bootstrap_paho.sh. Modo --mock gera
leituras artificiais e não emula o watchdog físico. --mock-manual não aceita
FIWARE. O material original da disciplina permanece em README.md/reference.
