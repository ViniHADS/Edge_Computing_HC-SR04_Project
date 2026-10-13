# Apresentação conjunta CP1/CP2

## Explicação para falar em sala

> Nosso projeto usa Arduino Mega, HC-SR04, potenciômetro e LED. O sensor mede
> distância e o potenciômetro ajusta uma referência entre 10 e 100 centímetros.
> Abaixo dessa referência, o LED fica aceso com brilho constante; acima dela,
> apaga. Falhas breves de eco são toleradas por menos de 300 ms, mas continuam
> informadas como erro. Falhas persistentes e leituras fora da faixa apagam a saída.
>
> O Arduino realiza esse controle localmente, perto do sensor, demonstrando o
> processamento na borda. Dividimos o firmware em módulos de aquisição, atuação,
> comunicação, telemetria e supervisão por watchdog.
>
> O computador executa uma ponte C++ entre USB e MQTT. O sistema operacional
> não define o projeto: a implementação POSIX funciona em Mac/Linux e pode ser
> executada em Windows através do WSL2 com USB encaminhada. Não estamos usando
> uma placa ESP32 física nem apresentando a ponte como firmware dessa placa.
>
> A ponte publica os dados reais em JSON e UltraLight. O Mosquitto distribui as
> mensagens, o IoT Agent traduz os atributos e o Orion mantém o estado atual.
> O Postman é o cliente usado para consultar esses dados e enviar comandos.
>
> O comando off chega ao Arduino e desabilita o LED. O comando on reabilita o
> controle automático, não força o LED a acender sem presença. Só publicamos
> confirmação OK após o Arduino aplicar a saída e responder com o mesmo ID.
>
> Também monitoramos a comunicação por heartbeat. O controle local continua
> funcionando se a rede cair, enquanto a falha é mostrada na telemetria e nas APIs.
> Após reset por watchdog, a ponte interrompe heartbeats sem telemetria recente
> para permitir que o firmware reinicie e retome as leituras automaticamente.
> O teste físico de recuperação passou duas vezes. Temos testes automatizados e uma
> demonstração física; o dashboard será desenvolvido na etapa CP3.

## Demonstração, em ordem

1. Mostrar a montagem e pinagem; colocar objeto a 10–30 cm e ajustar referência
   maior. LED constante dentro do limite, apagado fora.
2. Mostrar ADC bruto do potenciômetro e a referência derivada no GET do Orion.
3. Enviar off e observar LED zero/state OFF; enviar on e observar o retorno do
   controle. Conferir os horários de on_status/off_status para evitar OK antigo.
4. Mostrar no terminal da ponte a confirmação do Arduino com ID.
5. Mostrar sensor_status em ausência de eco e communication em condição normal.
6. Demonstrar o watchdog de modo separado: executar `.venv/bin/python -B scripts/verify_hardware.py --watchdog`, observar reinicialização, uptime menor e retorno a 100 ms. Aguarde
   a recuperação sem enviar comandos simultâneos; o teste permite até 20 segundos.
7. Mostrar organização dos arquivos e evidências dos testes, sem explicar cada
   linha de código. O README contém os comandos de reprodução.

## Respostas precisas

| Pergunta | Resposta |
| --- | --- |
| Postman recebe publicação diretamente do Arduino? | Não. Consulta os serviços que recebem as mensagens MQTT. |
| ON sempre significa LED aceso? | Não. Significa controle habilitado; a presença determina a saída. |
| Potentiometer é real? | Sim no modo físico v2, ADC 0–1023 transmitido pelo Arduino. |
| Sem eco e brilho 255 é erro no JSON? | Não. É tolerância curta; sensor_status continua indicando falha. |
| Distance no Orion pode ser antigo? | Sim. Durante leitura inválida, permanece a última distância válida. |
| HTTP 204 comprova atuação? | Só aceitação. O ACK correlacionado e estado novo comprovam o resultado. |
| O mock valida o circuito? | Não. A montagem é validada com o Mega físico. |
| Há dashboard final? | Ainda não; é entrega do CP3. |
| O ESP32 está sendo usado? | Não há placa física; a ponte MQTT é uma aplicação no computador. |

[Passo a passo](../README.md) · [Relatório de validação](VALIDACAO_CP2.md)
