#include "../ESP32/ESP32_mqtt_mock/src/protocol.h"
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

int main() {
  unsigned long delay = 0;
  for (const std::string value : {"100", "2000", "60000"}) {
    assert(protocol::parseDelayCommand(value, delay));
    assert(delay == std::stoul(value));
  }
  for (const std::string value : {"", "99", "60001", "-100", "100x",
       "2000\n", " 100", "1.0", "999999999999999999999999999999"}) {
    assert(!protocol::parseDelayCommand(value, delay));
  }

  const std::string valid = "{\"distancia_cm\":50.00,\"referencia_cm\":100,\"brilho\":127,\"status\":\"ok\",\"uptime_ms\":1234}";
  const std::string noEcho = "{\"distancia_cm\":null,\"referencia_cm\":10,\"brilho\":0,\"status\":\"sem_eco\",\"uptime_ms\":0}";
  assert(protocol::validTelemetry(valid));
  assert(protocol::validTelemetry(noEcho));
  for (const std::string& invalid : std::vector<std::string>{"boot message", "{}", "{\"status\":\"ok\"}",
       valid.substr(0, valid.size() - 1), valid + "garbage"}) {
    assert(!protocol::validTelemetry(invalid));
  }
  // JSON syntax alone is insufficient: ranges and status must agree.
  const auto replace = [](std::string text, const std::string& from,
                          const std::string& to) {
    text.replace(text.find(from), from.size(), to);
    return text;
  };
  assert(!protocol::validTelemetry(replace(valid, "50.00", "401")));
  assert(!protocol::validTelemetry(replace(valid, "50.00", "null")));
  assert(!protocol::validTelemetry(replace(valid, "\"brilho\":127", "\"brilho\":256")));
  assert(!protocol::validTelemetry(replace(valid, "\"referencia_cm\":100", "\"referencia_cm\":101")));
  assert(!protocol::validTelemetry(replace(valid, "1234", "4294967296")));
  assert(!protocol::validTelemetry(replace(noEcho, "\"brilho\":0", "\"brilho\":1")));
  auto transient = replace(noEcho, "\"brilho\":0", "\"brilho\":255");
  assert(protocol::validTelemetry(transient));
  assert(!protocol::validTelemetry(replace(transient, "sem_eco", "fora_faixa")));
  assert(protocol::validTelemetry(replace(noEcho, "sem_eco", "fora_faixa")));

  const std::string extended = R"({"distancia_cm":25,"referencia_cm":54,"brilho":255,"status":"ok","uptime_ms":10,"potenciometro":501,"habilitado":true,"comunicacao":"online"})";
  protocol::Telemetry physical;
  assert(protocol::parseTelemetry(extended, physical));
  assert(physical.extended && physical.potentiometer == 501 && physical.enabled);
  assert(!protocol::validTelemetry(replace(extended, "501", "1024")));
  assert(!protocol::validTelemetry(replace(extended, "true", "false")));
  std::string ackName; unsigned long ackId;
  assert(protocol::parseAcknowledgement(R"({"comando":"off","id":42,"resultado":"OK"})", ackName, ackId));
  assert(ackName == "off" && ackId == 42);
  assert(!protocol::parseAcknowledgement(R"({"comando":"off","id":0,"resultado":"OK"})", ackName, ackId));
  assert(!protocol::parseAcknowledgement(R"({"comando":"off","id":4294967296,"resultado":"OK"})", ackName, ackId));

  protocol::LineBuffer buffer;
  std::vector<std::string> lines;
  const auto collect = [&lines](const std::string& line) { lines.push_back(line); };
  const std::string first = valid.substr(0, 35);
  const std::string second = valid.substr(35) + "\r\n" + noEcho + "\n";
  for (char c : first) buffer.feed(c, collect);
  assert(lines.empty());
  for (char c : second) buffer.feed(c, collect);
  assert(lines.size() == 2);
  assert(lines[0] == valid);
  assert(lines[1] == noEcho);

  // A lost/oversize packet must not poison the following valid sample.
  for (char c : std::string(600, 'x') + "\n" + valid + "\n") buffer.feed(c, collect);
  assert(lines.size() == 3);
  assert(lines.back() == valid);
  std::cout << "Protocol checks passed\n";
}
