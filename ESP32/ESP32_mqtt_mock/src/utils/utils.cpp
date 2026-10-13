#include "utils.h"

#include <chrono>
#include <thread>

void delay(int ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

unsigned long millis()
{
    // Relogio monotono: ajustes no horario do sistema nao afetam os intervalos.
    static const auto start = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::steady_clock::now() - start;
    return static_cast<unsigned long>(
        std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
}
