#pragma once
// Superficie Arduino usada pelos modulos; implementada no harness nativo.
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>

#define A0 54
#define OUTPUT 1
#define INPUT 0
#define LOW 0
#define HIGH 1
#define F(value) value

struct FakeSerial {
  std::string input;
  std::ostringstream output;
  void begin(unsigned long) {}
  void flush() {}
  int available() const { return static_cast<int>(input.size()); }
  int read() { char c = input.front(); input.erase(0, 1); return c; }
  template<class T> void print(T value) { output << value; }
  void print(float value, int precision) {
    output << std::fixed << std::setprecision(precision) << value;
  }
  void print(double value, int precision) {
    output << std::fixed << std::setprecision(precision) << value;
  }
  void println() { output << '\n'; }
  template<class T> void println(T value) { print(value); println(); }
};

extern FakeSerial Serial;

void pinMode(int, int);
void digitalWrite(int, int);
void analogWrite(int, int);
int analogRead(int);
unsigned long pulseIn(int, int, unsigned long);
unsigned long millis();
void delay(unsigned long);
void delayMicroseconds(unsigned int);
long map(long, long, long, long, long);
template<class T, class U, class V> T constrain(T x, U low, V high) {
  return std::min(std::max(x, static_cast<T>(low)), static_cast<T>(high));
}

