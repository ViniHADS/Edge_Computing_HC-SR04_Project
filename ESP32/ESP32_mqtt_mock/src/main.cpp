#include "config.h"
#include "esp32.h"

#include <csignal>
#include <exception>
#include <iostream>

namespace
{
// O handler apenas sinaliza: a liberacao dos recursos ocorre fora do sinal.
volatile std::sig_atomic_t running = 1;
void stop(int) { running = 0; }
}

int main(int argc, char **argv)
{
    try
    {
        BridgeConfig config;
        if (!parseArguments(argc, argv, config))
            return 0;

        std::signal(SIGINT, stop);
        std::signal(SIGTERM, stop);

        // Recursos locais sao liberados ao sair normalmente ou por excecao.
        Bridge bridge(config);
        bridge.setup();
        while (running)
            bridge.loop();
    }
    catch (const std::exception &error)
    {
        std::cerr << "Erro: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
