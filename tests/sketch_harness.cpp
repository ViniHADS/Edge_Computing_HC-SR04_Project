// Native behavioral harness: it does not emulate the MCU or physical watchdog.
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "Arduino.h"
#include "Watchdog.h"

FakeSerial Serial;
static unsigned long fakeNow = 0;
static unsigned long fakeEcho = 0;
static unsigned long lastDelay = 0;
static int fakePot = 0;
static int lastPwm = -1;

void pinMode(int, int) {}
void digitalWrite(int, int) {}
void analogWrite(int, int value) { lastPwm = value; }
int analogRead(int) { return fakePot; }
unsigned long pulseIn(int, int, unsigned long timeout) {
  fakeNow += (fakeEcho ? fakeEcho : timeout) / 1000;
  return fakeEcho;
}
unsigned long millis() { return fakeNow; }
void delay(unsigned long ms) { lastDelay = ms; fakeNow += ms; }
void delayMicroseconds(unsigned int) {}
long map(long x, long inMin, long inMax, long outMin, long outMax) {
  return (x - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
}
#include "../Arduino/Arduino.ino"

int main(int argc, char **argv) {
  if (argc < 3) return 2;
  // Uma sequencia permite testar falhas intercaladas e recuperacao do sensor.
  std::vector<unsigned long> echoes;
  std::istringstream inputEchoes(argv[1]);
  std::string item;
  while (std::getline(inputEchoes, item, ','))
    echoes.push_back(std::strtoul(item.c_str(), nullptr, 10));
  fakeEcho = echoes.front();
  fakePot = std::atoi(argv[2]);
  setup();
  Serial.output.str("");
  Serial.output.clear();
  if (argc > 3) Serial.input = argv[3];
  // One loop is enough for a normal command. More allow fragmented input.
  int cycles = argc > 4 ? std::atoi(argv[4]) : 1;
  for (int i = 0; i < cycles; ++i) {
    fakeEcho = echoes[std::min(static_cast<std::size_t>(i), echoes.size() - 1)];
    loop();
  }
  std::cout << Serial.output.str();
  std::cout << "HARNESS " << lastPwm << ' ' << lastDelay << ' '
            << Watchdog::resets << ' ' << Watchdog::enabled << '\n';
}
