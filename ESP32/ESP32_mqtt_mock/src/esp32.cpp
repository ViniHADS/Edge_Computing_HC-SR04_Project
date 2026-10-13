#include "esp32.h"
#include "utils/utils.h"
#include "ultralight.h"

// Adaptacao do esp32.cpp da disciplina; copia original em ../reference/.
// A API HTTP publica comandos no mesmo broker local e assina nossa telemetria.

#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace
{
constexpr unsigned long RECONNECT_INTERVAL_MS = 5000;
constexpr unsigned long PUBLISH_INTERVAL_MS = 1000;
constexpr int SERIAL_CHUNK_SIZE = 256;
constexpr int LOOP_PAUSE_MS = 5;
}

Bridge::Bridge(const BridgeConfig &config) : config_(config)
{
}

void Bridge::setup()
{
    // Monta os topicos a partir de um unico prefixo configuravel.
    cmdTopic_ = config_.prefix + "/cmd";
    attrsTopic_ = config_.prefix + "/attrs";
    telemetryTopic_ = attrsTopic_ + "/telemetria";
    if (config_.fiware)
    {
        const auto prefix = "/" + config_.apiKey + "/" + config_.deviceId;
        fiwareAttrsTopic_ = prefix + "/attrs";
        fiwareCmdTopic_ = prefix + "/cmd";
        fiwareResultTopic_ = "/ul" + prefix + "/cmdexe";
        std::cout << "FIWARE UltraLight: " << fiwareAttrsTopic_ << std::endl;
    }

    if (config_.mock && !config_.manualMock)
    {
        simulator_.start(millis());
        std::cout << "Mock automatico: leituras a cada 100 ms; comandos MQTT alteram o intervalo."
                  << std::endl;
    }
    else if (config_.mock)
        mock_.begin('s');
    else
    {
        serial_.setPortName(config_.serialPort.c_str());
        serial_.begin(9600);
    }

    mqtt_.setServer(config_.broker.c_str(), config_.port);
    mqtt_.setCallback([this](const char *topic, uint8_t *payload, unsigned int length) {
        forwardCommand(topic, payload, length);
    });
    std::cout << "Telemetria: " << telemetryTopic_
              << " | Comandos: " << cmdTopic_ << std::endl;
}

void Bridge::loop()
{
    // A serial continua sendo drenada entre as tentativas de reconexao MQTT.
    try { receiveTelemetry(); }
    catch (...) {
        if (config_.fiware && mqtt_.connected())
            mqtt_.publish(fiwareAttrsTopic_.c_str(), "c|usb_desconectada");
        if (!pendingCommand_.empty()) reportCommand("ERROR");
        throw;
    }
    auto now = millis();
    supervise(now);
    reconnect(now);

    if (mqtt_.connected())
    {
        mqtt_.loop(); // Executa os comandos recebidos na thread principal.
        publishTelemetry(now);
    }
    delay(LOOP_PAUSE_MS);
}

void Bridge::receiveTelemetry()
{
    if (config_.mock && !config_.manualMock)
    {
        // Nenhuma entrada no teclado e necessaria no dispositivo virtual.
        simulator_.sample(millis(), latest_);
        return;
    }
    char rx[SERIAL_CHUNK_SIZE];
    int available = config_.mock ? mock_.available() : serial_.available();
    if (available <= 0)
        return;

    int length = std::min(available, SERIAL_CHUNK_SIZE);
    int count = config_.mock ? mock_.readBytes(rx, length) : serial_.readBytes(rx, length);
    for (int i = 0; i < count; ++i)
    {
        // Reconstroi linhas fragmentadas e guarda apenas a ultima amostra valida.
        lines_.feed(rx[i], [this](const std::string &line) {
            protocol::Telemetry sample;
            if (protocol::parseTelemetry(line, sample)) {
                latest_ = line;
                hasTelemetry_ = true; lastTelemetryAt_ = millis(); staleReported_ = false;
                extendedFirmware_ = sample.extended;
            } else {
                std::string name; unsigned long id;
                if (protocol::parseAcknowledgement(line, name, id) &&
                    id == pendingId_ && name == pendingCommand_) {
                    reportCommand("OK");
                    std::cout << "Arduino confirmou " << name << " (id " << id << ")" << std::endl;
                }
            }
        });
    }
}

void Bridge::reconnect(unsigned long now)
{
    if (mqtt_.connected() || now < retryAt_)
        return;

    retryAt_ = now + RECONNECT_INTERVAL_MS;
    if (mqtt_.connect(config_.clientId.c_str()))
    {
        // Cada nova sessao precisa renovar a assinatura de comandos.
        if (!mqtt_.subscribe(cmdTopic_.c_str()) ||
            (config_.fiware && !mqtt_.subscribe(fiwareCmdTopic_.c_str())))
            mqtt_.disconnect();
        else
            mqtt_.publish(attrsTopic_.c_str(), "s|true");
    }
}

void Bridge::publishTelemetry(unsigned long now)
{
    const bool automaticMock = config_.mock && !config_.manualMock;
    if (latest_.empty() || (!automaticMock && now < publishAt_))
        return;

    // A API Python recebe JSON; o IoT Agent recebe separadamente UltraLight.
    if (config_.fiware && now >= fiwarePublishAt_)
    {
        auto payload = ultralight::attributes(latest_, simulator_.powerEnabled());
        if (!payload.empty() && mqtt_.publish(fiwareAttrsTopic_.c_str(), payload.c_str()))
            fiwarePublishAt_ = now + PUBLISH_INTERVAL_MS;
    }
    // Se falhar, mantem a amostra para nova tentativa (ou uma mais recente).
    if (mqtt_.publish(telemetryTopic_.c_str(), latest_.c_str()))
        latest_.clear();
    publishAt_ = now + PUBLISH_INTERVAL_MS;
}

void Bridge::forwardCommand(const char *topic, uint8_t *payload, unsigned int length)
{
    if (config_.fiware && fiwareCmdTopic_ == topic)
    {
        std::string name;
        bool enabled;
        if (!ultralight::powerCommand(std::string(reinterpret_cast<char *>(payload), length),
                                     config_.deviceId, name, enabled))
        {
            std::cerr << "Comando FIWARE invalido" << std::endl;
            return;
        }
        if (!config_.mock) {
            if (!extendedFirmware_ || !hasTelemetry_ || millis() - lastTelemetryAt_ > 5000 ||
                !pendingCommand_.empty()) {
                auto result = config_.deviceId + "@" + name + "|ERROR";
                mqtt_.publish(fiwareResultTopic_.c_str(), result.c_str());
                std::cerr << "Comando fisico rejeitado: firmware sem telemetria recente ou ocupado" << std::endl;
                return;
            }
            pendingCommand_ = name;
            pendingId_ = nextId_++;
            if (nextId_ > 4294967295UL) nextId_ = 1;
            pendingUntil_ = millis() + 5000;
            auto frame = name + "|" + std::to_string(pendingId_) + "\n";
            if (serial_.writeBytes(frame.data(), frame.size()) != static_cast<int>(frame.size())) {
                reportCommand("ERROR");
                throw std::runtime_error("Falha ao enviar comando fisico");
            }
            return;
        }
        simulator_.setPower(enabled, millis());
        latest_.clear();
        fiwarePublishAt_ = 0;
        auto result = config_.deviceId + "@" + name + "|OK";
        bool reported = mqtt_.publish(fiwareResultTopic_.c_str(), result.c_str());
        std::cout << "FIWARE: " << name << " aplicado; resposta "
                  << (reported ? "enviada" : "nao enviada") << std::endl;
        return;
    }
    if (cmdTopic_ != topic)
        return;

    // Payload MQTT tem comprimento explicito; nao precisa terminar em zero.
    std::string text(reinterpret_cast<char *>(payload), length);
    unsigned long value;
    if (!protocol::parseDelayCommand(text, value))
    {
        std::cerr << "Comando rejeitado: esperado inteiro 100..60000.\n";
        return;
    }

    if (config_.mock && !config_.manualMock)
    {
        simulator_.setDelay(value, millis());
        latest_.clear(); // Descarta amostra pendente do intervalo anterior.
        std::cout << "Mock automatico: atraso aplicado = " << value << " ms" << std::endl;
        return;
    }

    text = std::to_string(value) + "\n";
    int count = config_.mock ? mock_.writeBytes(text.data(), text.size())
                             : serial_.writeBytes(text.data(), text.size());
    if (count != static_cast<int>(text.size()))
        throw std::runtime_error("Falha ao enviar comando serial");
    std::cout << "Comando encaminhado: " << value << " ms" << std::endl;
}

void Bridge::reportCommand(const char *result)
{
    if (pendingCommand_.empty()) return;
    auto frame = config_.deviceId + "@" + pendingCommand_ + "|" + result;
    mqtt_.publish(fiwareResultTopic_.c_str(), frame.c_str());
    pendingCommand_.clear(); pendingId_ = 0;
}
void Bridge::supervise(unsigned long now)
{
    if (config_.mock || !extendedFirmware_) return;
    // Se o firmware parar, deixa a UART em silencio para o bootloader concluir
    // a inicializacao apos watchdog. Retoma heartbeat com a primeira leitura.
    // O prazo nao altera o delay nem o watchdog do Mega.
    const bool serialRecente = hasTelemetry_ && now - lastTelemetryAt_ <= 5000;
    if (serialRecente && now >= heartbeatAt_) {
        std::string frame = mqtt_.connected() ? "hb|online\n" : "hb|offline\n";
        if (serial_.writeBytes(frame.data(), frame.size()) != static_cast<int>(frame.size()))
            throw std::runtime_error("Falha ao enviar heartbeat serial");
        heartbeatAt_ = now + 1000;
    }
    if (!pendingCommand_.empty() && now >= pendingUntil_) {
        reportCommand("ERROR");
        std::cerr << "Timeout: Arduino nao confirmou comando" << std::endl;
    }
    if (config_.fiware && mqtt_.connected() && hasTelemetry_ &&
        now - lastTelemetryAt_ > 5000 && !staleReported_) {
        if (mqtt_.publish(fiwareAttrsTopic_.c_str(), "c|sem_telemetria")) staleReported_ = true;
    }
}
