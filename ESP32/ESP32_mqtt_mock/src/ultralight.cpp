#include "ultralight.h"
#include "protocol.h"
#include <sstream>
namespace ultralight {
std::string attributes(const std::string &json, bool powered) {
    protocol::Telemetry sample;
    if (!protocol::parseTelemetry(json, sample)) return {};
    std::ostringstream output;
    output << "s|" << ((sample.extended ? sample.enabled : powered) ? "ON" : "OFF")
           << "|l|" << sample.brightness;
    if (sample.distance != "null") output << "|d|" << sample.distance;
    output << "|p|" << sample.potentiometer;
    // Campos adicionais opcionais, provisionados pelo script de preparacao CP2.
    if (sample.extended)
        output << "|r|" << sample.reference << "|e|" << sample.status << "|c|" << sample.communication;
    return output.str();
}
bool powerCommand(const std::string &payload, const std::string &deviceId,
                  std::string &name, bool &enabled) {
    const std::string prefix = deviceId + "@";
    if (payload.compare(0, prefix.size(), prefix) != 0) return false;
    auto separator = payload.find('|', prefix.size());
    if (separator == std::string::npos) return false;
    name = payload.substr(prefix.size(), separator - prefix.size());
    if (name != "on" && name != "off") return false;
    enabled = name == "on"; return true;
}
}
