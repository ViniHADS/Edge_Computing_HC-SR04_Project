#include "serialMock.h"

#include <iostream>
#include <poll.h>
#include <stdexcept>
#include <unistd.h>

void SerialMock::begin(char mode)
{
    // A ponte recebe o JSON textual emitido pelo Mega, sem modo hexadecimal.
    if (mode != 's')
        throw std::runtime_error("Esta ponte recebe linhas JSON em modo texto ('s').");
}

int SerialMock::available()
{
    pollfd descriptor{STDIN_FILENO, POLLIN, 0};
    poll(&descriptor, 1, 0);
    return descriptor.revents & POLLIN ? 256 : 0;
}

int SerialMock::readBytes(char *buffer, int length)
{
    auto count = read(STDIN_FILENO, buffer, length);
    return count > 0 ? static_cast<int>(count) : 0;
}

int SerialMock::writeBytes(const char *data, int length)
{
    // Comandos recebidos aparecem no console; nenhum hardware e acionado.
    std::cout << "TX Arduino: ";
    std::cout.write(data, length);
    std::cout.flush();
    return length;
}
