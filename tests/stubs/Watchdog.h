#pragma once

class Watchdog {
public:
  enum Timeout { TIMEOUT_1S };
  inline static bool enabled = false;
  inline static unsigned resets = 0;
  void enable(Timeout) { enabled = true; }
  void reset() { ++resets; }
};
