#pragma once

#include <string>

// Transporte USB/serial POSIX (Linux/macOS); o descritor pertence ao objeto.
class SerialCom
{
public:
    SerialCom() = default;
    ~SerialCom();
    SerialCom(const SerialCom &) = delete;
    SerialCom &operator=(const SerialCom &) = delete;

    void setPortName(const char *port);
    void begin(int baudRate);
    // Indica um tamanho de leitura possivel, nao uma contagem exata de bytes.
    int available();
    int readBytes(char *buffer, int length);
    int writeBytes(const char *data, int length);

private:
    int fd_ = -1;
    std::string portName_ = "/dev/ttyACM0";
};
