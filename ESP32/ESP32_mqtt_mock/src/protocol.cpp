#include "protocol.h"

#include <regex>

namespace protocol
{
bool parseDelayCommand(const std::string &text, unsigned long &value)
{
    // Limite de tamanho evita overflow antes da verificacao de faixa.
    if (text.empty() || text.size() > 5)
        return false;

    unsigned long number = 0;
    for (unsigned char character : text)
    {
        if (character < '0' || character > '9')
            return false;
        number = number * 10 + character - '0';
    }
    if (number < 100 || number > 60000)
        return false;
    value = number;
    return true;
}

bool parseTelemetry(const std::string &text, Telemetry &sample)
{
    // Versao 1 preservada para mocks; versao 2 acrescenta valores reais e saude.
    static const std::regex pattern(R"json(^\{"distancia_cm":(null|[0-9]+(?:\.[0-9]+)?),"referencia_cm":([0-9]+),"brilho":([0-9]+),"status":"(ok|sem_eco|fora_faixa)","uptime_ms":([0-9]+)(?:,"potenciometro":([0-9]+),"habilitado":(true|false),"comunicacao":"(online|ponte_offline|mqtt_offline)")?\}$)json");
    std::smatch fields;
    if (!std::regex_match(text, fields, pattern)) return false;
    try {
        if (std::stod(fields[2]) < 10 || std::stod(fields[2]) > 100 ||
            std::stod(fields[3]) > 255 || std::stod(fields[5]) > 4294967295.0) return false;
        if (fields[4] == "ok") {
            if (fields[1] == "null" || std::stod(fields[1]) < 2 || std::stod(fields[1]) > 400) return false;
        } else if (fields[1] != "null" ||
                   (fields[4] == "fora_faixa" && fields[3] != "0") ||
                   (fields[4] == "sem_eco" && fields[3] != "0" && fields[3] != "255")) return false;
        sample.distance = fields[1]; sample.status = fields[4];
        sample.reference = std::stoi(fields[2]); sample.brightness = std::stoi(fields[3]);
        sample.extended = fields[6].matched;
        sample.enabled = !sample.extended || fields[7] == "true";
        sample.communication = sample.extended ? fields[8].str() : "";
        sample.potentiometer = sample.extended ? std::stoi(fields[6]) : (sample.reference - 10) * 1023 / 90;
        if (sample.potentiometer > 1023) return false;
        if (sample.extended && ((!sample.enabled && sample.brightness != 0) ||
            (sample.brightness != 0 && sample.brightness != 255))) return false;
        return true;
    } catch (...) { return false; }
}
bool validTelemetry(const std::string &text) { Telemetry sample; return parseTelemetry(text, sample); }
bool parseAcknowledgement(const std::string &text, std::string &command, unsigned long &id)
{
    static const std::regex pattern(R"ack(^\{"comando":"(on|off)","id":([0-9]+),"resultado":"OK"\}$)ack");
    std::smatch fields;
    if (!std::regex_match(text, fields, pattern)) return false;
    try {
        if (std::stod(fields[2]) < 1 || std::stod(fields[2]) > 4294967295.0) return false;
        command = fields[1]; id = std::stoul(fields[2]); return true;
    } catch (...) { return false; }
}

void LineBuffer::feed(char character,
                      const std::function<void(const std::string &)> &callback)
{
    if (character == '\n')
    {
        if (!overflow_ && !buffer_.empty())
            callback(buffer_);
        buffer_.clear();
        overflow_ = false;
    }
    else if (character != '\r' && !overflow_)
    {
        // CR e ignorado para aceitar tanto LF quanto CRLF do Arduino.
        if (buffer_.size() >= 512)
        {
            buffer_.clear();
            overflow_ = true;
        }
        else
            buffer_ += character;
    }
}
}
