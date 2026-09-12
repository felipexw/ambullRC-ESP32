#include <unity.h>

#include "protocol/light_command_parser.h"

// User Story 1: the app has one lights toggle, so it sends only
// "LIGHTS_ON"/"LIGHTS_OFF" (no per-light targeting). Well-formed words parse
// into the correct LightCommand, case-insensitively.

void test_light_parses_light_on(void) {
  LightCommand cmd;
  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::Ok),
                     static_cast<int>(parseLightCommand("LIGHTS_ON", cmd)));
  TEST_ASSERT_TRUE(cmd.on);
}

void test_light_parses_light_off(void) {
  LightCommand cmd;
  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::Ok),
                     static_cast<int>(parseLightCommand("LIGHTS_OFF", cmd)));
  TEST_ASSERT_FALSE(cmd.on);
}

void test_light_parses_case_insensitively(void) {
  LightCommand cmd;
  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::Ok),
                     static_cast<int>(parseLightCommand("lights_on", cmd)));
  TEST_ASSERT_TRUE(cmd.on);

  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::Ok),
                     static_cast<int>(parseLightCommand("LiGhTs_OfF", cmd)));
  TEST_ASSERT_FALSE(cmd.on);
}

// Every existing drive word/numeric pair, and pure garbage, must be
// Malformed to this parser so the caller correctly falls through to
// DriveCommandAssembler (research.md §3).

void test_light_rejects_existing_drive_words_as_malformed(void) {
  LightCommand cmd;
  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::Malformed),
                     static_cast<int>(parseLightCommand("UP", cmd)));
  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::Malformed),
                     static_cast<int>(parseLightCommand("LEFT", cmd)));
  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::Malformed),
                     static_cast<int>(parseLightCommand("STOP", cmd)));
}

void test_light_rejects_numeric_pair_as_malformed(void) {
  LightCommand cmd;
  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::Malformed),
                     static_cast<int>(parseLightCommand("0,0", cmd)));
}

void test_light_rejects_garbage_as_malformed(void) {
  LightCommand cmd;
  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::Malformed),
                     static_cast<int>(parseLightCommand("garbage", cmd)));
  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::Malformed),
                     static_cast<int>(parseLightCommand("LIGHT_MAYBE", cmd)));
  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::Malformed),
                     static_cast<int>(parseLightCommand("", cmd)));
}

// The app never sends per-light commands; a per-light word must not be
// mistaken for the aggregate toggle.

void test_light_rejects_per_light_words_as_malformed(void) {
  LightCommand cmd;
  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::Malformed),
                     static_cast<int>(parseLightCommand("LIGHT1ON", cmd)));
  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::Malformed),
                     static_cast<int>(parseLightCommand("LIGHT4OFF", cmd)));
}
