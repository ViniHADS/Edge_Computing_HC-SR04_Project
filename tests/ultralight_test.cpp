#include "../ESP32/ESP32_mqtt_mock/src/ultralight.h"
#include <cassert>
#include <iostream>
int main() {
 const std::string json = R"({"distancia_cm":25,"referencia_cm":100,"brilho":191,"status":"ok","uptime_ms":1200})";
 assert(ultralight::attributes(json,true)=="s|ON|l|191|d|25|p|1023");
 assert(ultralight::attributes(json,false)=="s|OFF|l|191|d|25|p|1023");
 assert(ultralight::attributes("{broken}",true).empty());
 const std::string physical = R"({"distancia_cm":25,"referencia_cm":54,"brilho":255,"status":"ok","uptime_ms":10,"potenciometro":501,"habilitado":true,"comunicacao":"online"})";
 assert(ultralight::attributes(physical,false)=="s|ON|l|255|d|25|p|501|r|54|e|ok|c|online");
 bool enabled; std::string name;
 assert(ultralight::powerCommand("dist001@off|", "dist001", name, enabled) && !enabled && name=="off");
 assert(ultralight::powerCommand("dist001@on|1", "dist001", name, enabled) && enabled);
 assert(!ultralight::powerCommand("other@on|1", "dist001", name, enabled));
 assert(!ultralight::powerCommand("dist001@reset|1", "dist001", name, enabled));
 assert(!ultralight::powerCommand("off", "dist001", name, enabled));
 std::cout << "UltraLight encoding and command checks passed\n";
}
