#pragma once

#include <cstdio>
#include <string>

// Formats milliseconds-since-boot (as returned by millis()) as HH:MM:SS.mmm,
// shared by every hardware output that timestamps its serial log lines.
inline std::string formatTimestamp(unsigned long ms) {
  unsigned long hours = ms / 3600000UL;
  unsigned long minutes = (ms / 60000UL) % 60;
  unsigned long seconds = (ms / 1000UL) % 60;
  unsigned long millisRemainder = ms % 1000UL;
  char buf[16];
  std::snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu.%03lu", hours, minutes, seconds,
                millisRemainder);
  return buf;
}
