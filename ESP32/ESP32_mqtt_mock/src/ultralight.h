#pragma once
#include <string>

namespace ultralight
{
// Converte a amostra do simulador nos object_ids cadastrados no IoT Agent.
std::string attributes(const std::string &json, bool powered);
bool powerCommand(const std::string &payload, const std::string &deviceId,
                  std::string &name, bool &enabled);
}
