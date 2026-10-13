#pragma once

#include "config.h"
#include "mqtt/mqtt.h"
#include "protocol.h"
#include "serial/serial.h"
#include "serial/serialMock.h"
#include "simulator.h"

// Coordena os transportes. Cada instancia possui seus recursos e sua amostra.
class Bridge
{
public:
    explicit Bridge(const BridgeConfig &config);
    void setup();
    void loop();

private:
    void receiveTelemetry();
    void reconnect(unsigned long now);
    void publishTelemetry(unsigned long now);
    void supervise(unsigned long now);
    void reportCommand(const char *result);
    void forwardCommand(const char *topic, uint8_t *payload, unsigned int length);

    BridgeConfig config_;
    SerialCom serial_;
    SerialMock mock_;
    DeviceSimulator simulator_;
    protocol::LineBuffer lines_;
    std::string latest_;
    std::string pendingCommand_;
    unsigned long pendingId_ = 0, nextId_ = 1, pendingUntil_ = 0;
    unsigned long heartbeatAt_ = 0, lastTelemetryAt_ = 0;
    bool hasTelemetry_ = false, extendedFirmware_ = false, staleReported_ = false;
    std::string cmdTopic_, attrsTopic_, telemetryTopic_;
    std::string fiwareAttrsTopic_, fiwareCmdTopic_, fiwareResultTopic_;
    unsigned long fiwarePublishAt_ = 0;
    unsigned long retryAt_ = 0;
    unsigned long publishAt_ = 0;
    // Destruido primeiro: encerra callbacks antes dos objetos que eles acessam.
    ArduinoMQTT mqtt_;
};
