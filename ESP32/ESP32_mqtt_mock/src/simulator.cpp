#include "simulator.h"

#include <algorithm>
#include <cstdint>
#include <sstream>

void DeviceSimulator::start(unsigned long now)
{
    startedAt_ = now;
    nextSampleAt_ = now;
    delayMs_ = 100;
    sampleIndex_ = 0;
    powerEnabled_ = true;
}

void DeviceSimulator::setPower(bool enabled, unsigned long now)
{
    powerEnabled_ = enabled;
    nextSampleAt_ = now; // Publica o novo estado sem esperar um atraso longo.
}

void DeviceSimulator::setDelay(unsigned long delayMs, unsigned long now)
{
    delayMs_ = delayMs;
    // O novo intervalo conta a partir do comando, sem bloquear a rede.
    nextSampleAt_ = now + delayMs_;
}

bool DeviceSimulator::sample(unsigned long now, std::string &json)
{
    if (now < nextSampleAt_)
        return false;

    // Distancia triangular entre 20 e 120 cm, em passos de 5 cm por amostra.
    // Referencia fixa de 100 cm representa o potenciometro virtual.
    unsigned long step = sampleIndex_++ % 40;
    int distance = 20 + static_cast<int>(step <= 20 ? step : 40 - step) * 5;
    int brightness = std::clamp(static_cast<int>((100 - distance) * 255.0 / 100), 0, 255);
    if (!powerEnabled_)
        brightness = 0;
    std::ostringstream output;
    output << "{\"distancia_cm\":" << distance
           << ",\"referencia_cm\":100,\"brilho\":" << brightness
           << ",\"status\":\"ok\",\"uptime_ms\":"
           << static_cast<std::uint32_t>(now - startedAt_) << '}';
    json = output.str();
    nextSampleAt_ = now + delayMs_;
    return true;
}
