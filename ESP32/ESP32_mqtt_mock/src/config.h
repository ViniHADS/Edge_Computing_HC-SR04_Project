#pragma once

#include <string>

// Opcoes de execucao da ponte. Nao contem estado de conexao ou de sensores.
struct BridgeConfig
{
    std::string serialPort;
    std::string broker = "localhost";
    std::string prefix = "/TEF/exemplo001";
    std::string clientId;
    unsigned short port = 1883;
    bool mock = false;
    bool manualMock = false;
    bool fiware = false;
    std::string apiKey = "TEF";
    std::string deviceId = "dist001";
};

// Retorna false para --help; erros de uso geram excecao tratada pelo main.
bool parseArguments(int argc, char **argv, BridgeConfig &config);
