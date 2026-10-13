#include "../ESP32/ESP32_mqtt_mock/src/simulator.h"
#include "../ESP32/ESP32_mqtt_mock/src/protocol.h"

#include <cassert>
#include <iostream>

int main()
{
    DeviceSimulator device;
    std::string json;
    device.start(500);
    assert(device.sample(500, json));
    assert(json == "{\"distancia_cm\":20,\"referencia_cm\":100,\"brilho\":204,\"status\":\"ok\",\"uptime_ms\":0}");
    assert(!device.sample(599, json));
    assert(device.sample(600, json));
    assert(json.find("\"distancia_cm\":25") != std::string::npos);

    // Um comando reagenda a proxima amostra; nao publica antes do novo prazo.
    device.setDelay(250, 650);
    assert(!device.sample(899, json));
    assert(device.sample(900, json));
    assert(!device.sample(1149, json));
    assert(device.sample(1150, json));

    // Nao emula watchdog: mesmo 2000 ms e um intervalo valido do simulador.
    device.setDelay(2000, 1200);
    assert(!device.sample(3199, json));
    assert(device.sample(3200, json));
    assert(json.find("\"uptime_ms\":2700") != std::string::npos);

    device.setPower(false, 3201);
    assert(device.sample(3201, json));
    assert(!device.powerEnabled());
    assert(json.find("\"brilho\":0") != std::string::npos);
    device.setPower(true, 3202);
    assert(device.sample(3202, json));
    assert(device.powerEnabled());

    // Todo o ciclo de aproximacao/afastamento respeita o protocolo da ponte.
    device.start(0);
    for (unsigned long now = 0; now <= 8000; now += 100)
    {
        assert(device.sample(now, json));
        assert(protocol::validTelemetry(json));
        if (now == 2000)
            assert(json.find("\"distancia_cm\":120,\"referencia_cm\":100,\"brilho\":0") != std::string::npos);
    }
    std::cout << "Simulator timing, command application and telemetry checks passed\n";
}
