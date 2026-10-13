# Camada de comunicacao

[ESP32_mqtt_mock](ESP32_mqtt_mock/) e a ponte C++ executada no computador.
Ela conecta a USB do Arduino ao MQTT e ao FIWARE. A pasta segue a separacao
prevista pelo template da disciplina, mas nao contem firmware para ESP32 fisico.
Mac/Linux executam a ponte POSIX; Windows utiliza WSL2 com USB encaminhada.
Consulte o [README principal](../README.md) para a arquitetura real e execucao.
