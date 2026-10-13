#include "mqtt.h"

#include <iostream>

ArduinoMQTT::~ArduinoMQTT()
{
    disconnect();
}

// Configuracao feita antes da primeira conexao, pela inicializacao da ponte.
void ArduinoMQTT::setServer(const char *host, uint16_t port)
{
    domain_ = host;
    port_ = port;
}

void ArduinoMQTT::setCallback(ArduinoCallback callback)
{
    callback_.set_callback_arduino(std::move(callback));
}

bool ArduinoMQTT::connected()
{
    return client_ && client_->is_connected();
}

bool ArduinoMQTT::connect(const char *id)
{
    try
    {
        if (connected())
            return true;
        if (!client_)
        {
            client_ = std::make_unique<mqtt::async_client>(
                "tcp://" + domain_ + ":" + std::to_string(port_), id);
            client_->set_callback(callback_);
        }

        // Sessao limpa: a aplicacao renova as assinaturas apos cada conexao.
        mqtt::connect_options options;
        options.set_clean_session(true);
        options.set_connect_timeout(2);
        options.set_keep_alive_interval(10);
        client_->connect(options)->wait();
        return true;
    }
    catch (const mqtt::exception &error)
    {
        std::cerr << "MQTT: " << error.what() << '\n';
        return false;
    }
}

void ArduinoMQTT::disconnect()
{
    if (!connected())
        return;
    try
    {
        client_->disconnect()->wait();
    }
    catch (const mqtt::exception &)
    {
        // O broker pode ja ter caido; o destrutor ainda libera o cliente local.
    }
}

bool ArduinoMQTT::subscribe(const char *topic)
{
    if (!connected())
        return false;
    try
    {
        client_->subscribe(topic, 0)->wait();
        return true;
    }
    catch (const mqtt::exception &error)
    {
        std::cerr << "Subscribe: " << error.what() << '\n';
        return false;
    }
}

bool ArduinoMQTT::publish(const char *topic, const char *payload)
{
    if (!connected())
        return false;
    try
    {
        // QoS 0 sem retencao: telemetria representa amostras, nao historico.
        client_->publish(mqtt::make_message(topic, payload, 0, false))->wait();
        return true;
    }
    catch (const mqtt::exception &error)
    {
        std::cerr << "Publish: " << error.what() << '\n';
        return false;
    }
}

void ArduinoMQTT::loop()
{
    callback_.dispatch_msgs();
}

void MqttCallback::set_callback_arduino(ArduinoCallback callback)
{
    callback_ = std::move(callback);
}

void MqttCallback::message_arrived(mqtt::const_message_ptr message)
{
    // Nao repetir resets retidos pelo broker. Limita a fila para conter memoria.
    if (message->is_retained())
        return;
    std::lock_guard<std::mutex> lock(mutex_);
    if (messages_.size() < 32)
        messages_.push({message->get_topic(), message->to_string()});
}

void MqttCallback::dispatch_msgs()
{
    for (;;)
    {
        mqttQueueMsg message;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (messages_.empty())
                return;
            message = std::move(messages_.front());
            messages_.pop();
        }
        // Libera o mutex antes da escrita serial para nao bloquear o Paho.
        if (callback_)
            callback_(message.topic.c_str(),
                      reinterpret_cast<uint8_t *>(message.payload.data()),
                      static_cast<unsigned int>(message.payload.size()));
    }
}
