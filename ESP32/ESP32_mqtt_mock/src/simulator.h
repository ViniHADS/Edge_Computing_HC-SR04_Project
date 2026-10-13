#pragma once

#include <string>

// Modelo virtual de distancia/potenciometro/LED; nao acessa hardware.
// O tempo e fornecido pela ponte, permitindo testes deterministas sem sleeps.
class DeviceSimulator
{
public:
    void start(unsigned long now);
    void setDelay(unsigned long delayMs, unsigned long now);
    void setPower(bool enabled, unsigned long now);
    bool powerEnabled() const { return powerEnabled_; }
    bool sample(unsigned long now, std::string &json);

private:
    unsigned long startedAt_ = 0;
    unsigned long nextSampleAt_ = 0;
    unsigned long delayMs_ = 100;
    unsigned long sampleIndex_ = 0;
    bool powerEnabled_ = true;
};
