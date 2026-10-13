#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <utility>
#include <mqtt/async_client.h>

using ArduinoCallback = std::function<void(const char *, uint8_t *, unsigned int)>;

struct mqttQueueMsg
{
    std::string topic;
    std::string payload;
};

// A thread do Paho enfileira; a thread principal executa os comandos seriais.
class MqttCallback : public virtual mqtt::callback
{
public:
    void message_arrived(mqtt::const_message_ptr message) override;
    void dispatch_msgs();
    void set_callback_arduino(ArduinoCallback callback);

private:
    std::mutex mutex_;
    std::queue<mqttQueueMsg> messages_;
    ArduinoCallback callback_;
};

// Adaptador com API semelhante a PubSubClient, implementado com Eclipse Paho.
class ArduinoMQTT
{
public:
    ~ArduinoMQTT();
    void setServer(const char *host, uint16_t port);
    void setCallback(ArduinoCallback callback);
    bool connected();
    bool connect(const char *id);
    void disconnect();
    bool subscribe(const char *topic);
    bool publish(const char *topic, const char *payload);
    void loop();

private:
    std::string domain_ = "localhost";
    uint16_t port_ = 1883;
    MqttCallback callback_;
    // Cliente destruido antes do callback utilizado por suas threads internas.
    std::unique_ptr<mqtt::async_client> client_;
};
