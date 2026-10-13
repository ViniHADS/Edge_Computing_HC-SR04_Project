# FIWARE com dist001 — CP2

Veja o [README principal](../../README.md) para um roteiro completo.

O modo físico agora aceita --serial junto de --fiware. Use apenas uma ponte
para dist001; mock e físico não devem publicar simultaneamente nessa entidade.

```sh
.venv/bin/python scripts/configure_fiware.py --apply
./ESP32/ESP32_mqtt_mock/build/ESP32Mock --serial /dev/cu.usbserial-120 --fiware --broker localhost --port 1883
```

Serviço smart, caminho /, API key TEF. GET no Orion1026 exibe state, led,
distance, potentiometer, reference, sensor_status e communication. state ON/OFF
indica habilitação do controle; led é a saída física. Potentiometer é ADC real
no firmware v2. Distance permanece última medida válida quando sem eco;
consulte sensor_status e horários para não confundir com uma leitura atual.

POST http://localhost:4041/v2/op/update envia on/off; confirmação OK é publicada
somente após resposta correlacionada do Arduino. A coleção local foi preservada.
Não usar o antigo PATCH para Lamp:001 como comando do dispositivo dist001.

O Docker existente foi reutilizado. Scripts de cadastro acrescentam atributos,
sem remover dados e sem refazer dispositivos existentes. Não alteramos suas
inscrições duplicadas no Orion; os comandos do roteiro vão direto ao IoT Agent.
