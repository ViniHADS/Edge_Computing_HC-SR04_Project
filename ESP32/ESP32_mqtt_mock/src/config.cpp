#include "config.h"

#include <iostream>
#include <stdexcept>
#include <unistd.h>

bool parseArguments(int argc, char **argv, BridgeConfig &config)
{
    // IDs distintos permitem executar varias pontes no mesmo computador.
    config.clientId = "mega2560-" + std::to_string(getpid());

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--help")
        {
            std::cout << "ESP32Mock (--serial PORTA | --mock | --mock-manual) [--broker HOST] "
                         "[--port 1883] [--prefix /TEF/exemplo001] [--client-id ID] "
                         "[--fiware --api-key TEF --device-id dist001]\n";
            return false;
        }
        if (arg == "--mock" || arg == "--mock-manual")
        {
            if (config.mock)
                throw std::runtime_error("Escolha somente um modo mock");
            config.mock = true;
            config.manualMock = arg == "--mock-manual";
            continue;
        }
        if (arg == "--fiware")
        {
            config.fiware = true;
            continue;
        }
        if (i + 1 >= argc)
        {
            throw std::runtime_error("Falta valor para " + arg);
        }

        std::string value = argv[++i];
        if (arg == "--serial")
            config.serialPort = value;
        else if (arg == "--broker")
            config.broker = value;
        else if (arg == "--prefix")
            config.prefix = value;
        else if (arg == "--client-id")
            config.clientId = value;
        else if (arg == "--api-key")
            config.apiKey = value;
        else if (arg == "--device-id")
            config.deviceId = value;
        else if (arg == "--port")
        {
            // Exige que toda a string represente uma porta TCP valida.
            std::size_t used;
            int port = std::stoi(value, &used);
            if (used != value.size() || port < 1 || port > 65535)
                throw std::runtime_error("Porta MQTT inválida");
            config.port = static_cast<unsigned short>(port);
        }
        else
            throw std::runtime_error("Opção desconhecida: " + arg);
    }

    // Serial real e mock sao mutuamente exclusivos; topicos nao usam wildcards.
    if (config.mock == !config.serialPort.empty())
        throw std::runtime_error("Escolha exatamente --serial PORTA, --mock ou --mock-manual");
    if (config.prefix.empty() || config.prefix.find_first_of("+#") != std::string::npos ||
        config.broker.empty() || config.clientId.empty())
        throw std::runtime_error("Configuração MQTT inválida");
    while (config.prefix.size() > 1 && config.prefix.back() == '/')
        config.prefix.pop_back();
    if (config.fiware && config.manualMock)
        throw std::runtime_error("--fiware suporta --serial ou --mock automatico");
    for (const auto &part : {config.apiKey, config.deviceId})
        if (part.empty() || part.find_first_of("/+#@| \r\n") != std::string::npos)
            throw std::runtime_error("API key/device ID invalido");
    return true;
}
