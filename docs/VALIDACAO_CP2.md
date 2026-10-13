# Validação CP1/CP2 — 13/10/2026

Este relatório registra testes observados nesta montagem e nesta máquina.
Os testes com mock, PTY e hardware real estão identificados separadamente.
Não afirma aprovação acadêmica, publicação ou garantia de nota.

## Resultado final

A integração física Arduino Mega → USB → ponte C++ → MQTT → FIWARE/API funciona
com telemetria real e comandos ON/OFF confirmados pelo Arduino. A recuperação
após watchdog foi confirmada em **dois ensaios completos consecutivos** após o
ajuste autorizado do heartbeat da ponte. Não houve gravação de bootloader/fuses.

| Verificação | Resultado |
| --- | --- |
| Firmware Mega2560 | Compilação aprovada: 8224 bytes de programa, 771 bytes de RAM estática |
| Ponte C++ | Compilação aprovada após reorganização e ajuste final |
| Testes de lógica/protocolo | 26 aprovados, também repetidos após o ajuste final |
| `test_mqtt_e2e.py` | Fragmentação, logs ignorados, comando inválido/retido, UART e reconexão aprovados |
| `test_http_e2e.py` | HTTP/MQTT/UART simulada, erros, tolerância sem eco, cache e reconexão aprovados |
| `test_physical_bridge_e2e.py` | UART via PTY/MQTT real: ADC bruto, ACK com ID, timeout ERROR, heartbeat offline/online e USB perdida aprovados |
| Regressão nova de heartbeat | Silêncio serial após perda de telemetria e retomada com nova leitura aprovados |
| `test_fiware_e2e.py` | Mock no Docker existente: atributos e ON/OFF com confirmação nova aprovados na etapa anterior; não equivale a hardware |
| Hardware real | ADC bruto, referência, estado do sensor, comunicação e ON/OFF com ACK correlacionado aprovados |
| Watchdog físico, ensaio 1 | Uptime 20112 → 417 ms; telemetria retomada e atraso 100 ms restaurado |
| Watchdog físico, ensaio 2 | Uptime 123590 → 417 ms; executado no terminal integrado do VSCode, aprovado |
| Postman, hardware | GET telemetria 200; POST ON 204 seguido de novo ACK OK e state ON |
| VSCode, terminal integrado | Compilação Arduino aprovada; GET e POST OFF executados; estado OFF/led 0 e novo ACK OK conferidos |
| Compose de referência | `docker compose config --quiet` aprovado; não houve implantação de outro ambiente |

## Investigação e correção do watchdog

Os primeiros ensaios interromperam a telemetria e não confirmaram retomada em
20 segundos. A rotina AVR `.init3` autorizada limpou WDRF/desativou WDT antes do
setup, mas essa alteração isolada não resolveu a integração. Essas falhas foram
observações intermediárias, superadas pelos ensaios finais acima.

Na retomada, a memória flash foi **lida sem escrita** com avrdude. Os bytes do
sketch corresponderam ao firmware atual compilado; os bytes do bootloader
corresponderam a `stk500boot_v2_mega2560.hex` instalado pelo Arduino AVR Core1.8.8.
Foi encerrado também um processo antigo de gravação avrdude. Não foi comprovado
que esse processo causava a falha.

No ensaio direto pela serial, sem heartbeat da ponte, reapareceram o banner de
inicialização e as leituras após enviar 2000. Na integração, os heartbeats
continuavam mesmo quando não havia novas leituras. Suspender a ponte por 12 s,
mantendo a porta aberta, permitiu recuperar telemetria com uptime reiniciado.
Isso direcionou o ajuste autorizado ao tráfego UART durante inicialização.

A ponte agora envia heartbeat somente com telemetria serial recente (até 5 s).
Quando a placa para de transmitir, deixa de enviar heartbeat, permitindo um
intervalo sem tráfego para o bootloader iniciar o firmware. A primeira leitura
válida retoma o heartbeat. Nenhum delay, pino, watchdog1s ou regra do LED foi
alterado nessa correção. Os dois ensaios finais completos confirmaram recuperação
automática, seguida de aumento do uptime e dados recentes.

A [documentação oficial Arduino](https://support.arduino.cc/hc/en-us/articles/5779192727068-Reset-your-board)
descreve o papel do watchdog nos resets AVR e problemas possíveis com bootloaders.
A hipótese inicial de bootloader incompatível não foi comprovada; a solução
validada nesta montagem foi ajustar o envio de heartbeat na ponte.

## Verificação no Postman e VSCode

Foram usadas as requisições que já estavam abertas, sem editar nem salvar a
coleção. O GET do item7 retornou 200. O POST ON existente em4041 retornou204;
o resultado foi confirmado por ACK físico novo, não somente pelo código HTTP.
Após a correção, o ACK ON observado teve TimeInstant21:35:21.583Z e state ON.
O POST OFF no terminal do VSCode teve ACK21:34:48.391Z, state OFF e led0.

Após o segundo watchdog, o GET da API no Postman mostrou200, origem arduino,
mqtt_conectado true, desatualizada false e idade0.656s. O terminal do VSCode
mostrou os quatro PASS do teste físico, incluindo uptime123590 → 417ms.

O editor C/C++ do VSCode ainda apresenta diagnóstico de includePath; usa a
configuração genérica clang e não foi reconfigurado para o SDK AVR. Isso é uma
limitação do IntelliSense, registrada separadamente da compilação real aprovada
pelo Arduino CLI. O editor antigo do sketch movido também permanece aberto como
arquivo excluído. Para compilar, usar a tarefa Arduino ou `scripts/arduino.sh`.

## Cadastro e dependências

- Docker existente ativo nas portas1883,4041 e1026, usado nas validações.
- dist001 ampliado com reference, sensor_status e communication, mantendo os
  atributos anteriores, comandos e protocolo. PUT204 seguido de GET conferido.
- A implantação nova do Compose, setup em máquina limpa e Windows/WSL2 não foram
  reproduzidos; os passos estão documentados, com esse limite explícito.
- 32 links locais da documentação principal conferidos, sem destinos ausentes;
  coleções locais com JSON válido. ZIP verificado com coleções idênticas às locais.

## Reprodução

Os comandos completos e as dependências estão no [README](../README.md).
Com a ponte física e a API ativas, aguarde alguns segundos após inicialização:

```sh
.venv/bin/python -B scripts/verify_hardware.py
.venv/bin/python -B scripts/verify_hardware.py --watchdog
```

O segundo comando provoca reset intencional; aguarde sem enviar outros comandos.
O teste permite até20s para confirmar o retorno e restaura atraso100ms ao passar.
Na conclusão das validações, ponte/API foram encerradas; o Docker foi preservado
ativo e o firmware permaneceu com controle habilitado e atraso normal100ms.

## Limites da entrega acadêmica

- Display/botão dispensados segundo orientação do grupo.
- Não há ESP32 físico. A pasta ESP32 contém a ponte do computador. A aceitação
  dessa configuração frente à exigência original de duas placas deve ser
  alinhada com o professor, sem apresentar a aplicação como uma placa ESP32.
- Dashboard reservado ao CP3. Wokwi original não atualizado nesta atividade.
- Coleções preservadas. Os testes deste relatório foram concluídos antes da
  publicação da entrega conjunta por PR para a main.
- Acesso do professor, regras de revisão e envio no Teams são etapas externas.
- O ZIP contém os fontes preparados para revisão/transporte; não significa
  entrega já enviada ou aprovação integral pelo professor.
