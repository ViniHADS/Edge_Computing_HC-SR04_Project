#pragma once
#include <functional>
#include <string>
namespace protocol {
struct Telemetry {
    std::string distance, status;
    int reference = 0, brightness = 0, potentiometer = 0;
    bool extended = false, enabled = true;
    std::string communication;
};
bool parseDelayCommand(const std::string &text, unsigned long &value);
bool parseTelemetry(const std::string &text, Telemetry &sample);
bool validTelemetry(const std::string &text);
bool parseAcknowledgement(const std::string &text, std::string &command, unsigned long &id);
class LineBuffer {
public:
    void feed(char character, const std::function<void(const std::string &)> &callback);
private:
    std::string buffer_;
    bool overflow_ = false;
};
}
