#include "serial.h"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <stdexcept>
#include <termios.h>
#include <unistd.h>

SerialCom::~SerialCom()
{
    if (fd_ >= 0)
        close(fd_);
}

void SerialCom::setPortName(const char *port)
{
    portName_ = port;
}

void SerialCom::begin(int baudRate)
{
    // A mesma taxa deve ser usada pelo sketch; leitura da porta nao bloqueia.
    if (baudRate != 9600)
        throw std::runtime_error("A ponte usa 9600 baud.");
    fd_ = open(portName_.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd_ < 0)
        throw std::runtime_error("Abrir serial " + portName_ + ": " + strerror(errno));

    // Modo raw 8N1: preserva os bytes JSON, sem eco ou controle de fluxo.
    termios tty{};
    if (tcgetattr(fd_, &tty))
        throw std::runtime_error("tcgetattr falhou");
    cfmakeraw(&tty);
    cfsetispeed(&tty, B9600);
    cfsetospeed(&tty, B9600);
    tty.c_cflag |= CLOCAL | CREAD;
    tty.c_cflag &= ~(PARENB | CSTOPB | CSIZE);
    tty.c_cflag |= CS8;
#ifdef CRTSCTS
    tty.c_cflag &= ~CRTSCTS;
#endif
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 0;
    if (tcsetattr(fd_, TCSANOW, &tty))
        throw std::runtime_error("tcsetattr falhou");
}

int SerialCom::available()
{
    // Consulta imediata permite alternar leitura serial e processamento MQTT.
    pollfd descriptor{fd_, POLLIN, 0};
    int result = poll(&descriptor, 1, 0);
    if (result < 0 && errno != EINTR)
        throw std::runtime_error("poll serial falhou");
    if (descriptor.revents & (POLLERR | POLLHUP | POLLNVAL))
        throw std::runtime_error("Serial desconectada; reconecte e reinicie a ponte.");
    return descriptor.revents & POLLIN ? 256 : 0;
}

int SerialCom::readBytes(char *buffer, int length)
{
    if (length <= 0)
        return 0;
    auto count = read(fd_, buffer, length);
    if (count < 0 && (errno == EAGAIN || errno == EINTR))
        return 0;
    if (count < 0)
        throw std::runtime_error("read serial falhou");
    return static_cast<int>(count);
}

int SerialCom::writeBytes(const char *data, int length)
{
    // Uma chamada de write pode enviar so parte do comando; completa o restante.
    int sent = 0;
    while (sent < length)
    {
        auto count = write(fd_, data + sent, length - sent);
        if (count > 0)
            sent += static_cast<int>(count);
        else if (count < 0 && errno == EINTR)
            continue;
        else if (count < 0 && errno == EAGAIN)
        {
            pollfd descriptor{fd_, POLLOUT, 0};
            if (poll(&descriptor, 1, 1000) <= 0)
                break;
        }
        else
            break;
    }
    return sent;
}
