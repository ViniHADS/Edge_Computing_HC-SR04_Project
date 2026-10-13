# Referência original

`esp32.cpp` é uma cópia do arquivo indicado em
https://github.com/gsalati/Edge-Computing-Computer-Systems/blob/main/ESP32_mqtt_mock/src/esp32.cpp.

Essa cópia não participa do build. A implementação ativa está em `../src/esp32.cpp`:
preserva broker localhost, tópicos `/TEF/exemplo001`, SerialCom/SerialMock e o
adaptador ArduinoMQTT, usando o JSON do Mega no lugar dos bytes arbitrários do
exemplo. O exemplo original não expõe HTTP; a API para Postman está em `../../api/`.
