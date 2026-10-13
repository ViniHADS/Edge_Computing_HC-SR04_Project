# API auxiliar de telemetria

Veja o [passo a passo principal](../README.md). A API Python conecta HTTP ao
MQTT local; o FIWARE continua sendo atendido pelo IoT Agent/Orion.

```sh
.venv/bin/python -m api.server --port 8001 --mqtt-port 1883
```

| Rota | Resultado |
| --- | --- |
| GET /health | 200 com MQTT conectado/assinatura ativa, 503 sem broker |
| GET /telemetria | 200 com última amostra e idade, 404 antes da primeira |
| POST /comandos/atraso | JSON `{"atraso_ms":100}`; 202 significa envio MQTT |

O POST exige application/json, apenas atraso_ms inteiro 100–60000. Valores
inválidos retornam 400; sem JSON, 415; MQTT indisponível, 503.
`execucao_no_mega_confirmada: false` é intencional para o comando de atraso.
On/off físico é enviado ao IoT Agent em 4041 e confirmado por ACK do Arduino.

O JSON v2 inclui ADC bruto, habilitação e comunicação. `origem: arduino` indica
formato v2 do firmware; não é autenticação criptográfica da origem.
`desatualizada` torna-se true após 5 s sem amostra ou queda do broker.
Sem eco usa distância null; brilho pode ser 255 durante tolerância de 300 ms.
O cache é apenas em memória, sem histórico e sem mensagens retidas.

A [coleção local](../postman/Mega2560-localhost.postman_collection.json) foi
preservada com baseUrl padrão 8000; ajuste a URL de uma requisição temporária
para 8001 ou use curl sem alterar a coleção. Testes:

```sh
.venv/bin/python -B tests/test_http_e2e.py
```
