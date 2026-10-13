#pragma once

// Substitui a placa por stdin/stdout; usa o mesmo contrato de bytes da serial.
class SerialMock
{
public:
    void begin(char mode);
    int available();
    int readBytes(char *buffer, int length);
    int writeBytes(const char *data, int length);
};
