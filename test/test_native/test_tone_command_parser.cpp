#include <unity.h>

#include "protocol/tone_command_parser.h"

// HORN is the only sound-effect trigger word, parsed case-insensitively,
// with no ON/OFF pair or numeric argument.

void test_tone_parses_horn(void) {
  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::Ok), static_cast<int>(parseHornCommand("HORN")));
}

void test_tone_parses_case_insensitively(void) {
  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::Ok), static_cast<int>(parseHornCommand("horn")));
  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::Ok), static_cast<int>(parseHornCommand("HoRn")));
}

// Every existing drive word, light word, numeric pair, and pure garbage must
// be Malformed to this parser so the caller correctly falls through to the
// existing parsers (research.md §2).

void test_tone_rejects_existing_drive_words_as_malformed(void) {
  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::Malformed), static_cast<int>(parseHornCommand("UP")));
  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::Malformed), static_cast<int>(parseHornCommand("LEFT")));
  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::Malformed), static_cast<int>(parseHornCommand("STOP")));
}

void test_tone_rejects_existing_light_words_as_malformed(void) {
  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::Malformed),
                     static_cast<int>(parseHornCommand("LIGHTS_ON")));
  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::Malformed),
                     static_cast<int>(parseHornCommand("LIGHTS_OFF")));
}

void test_tone_rejects_numeric_pair_as_malformed(void) {
  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::Malformed), static_cast<int>(parseHornCommand("0,0")));
}

void test_tone_rejects_garbage_as_malformed(void) {
  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::Malformed),
                     static_cast<int>(parseHornCommand("garbage")));
  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::Malformed),
                     static_cast<int>(parseHornCommand("HORNS")));
  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::Malformed), static_cast<int>(parseHornCommand("")));
}

// SIREN and ENGINE sound effects have been removed — both must be rejected,
// not silently accepted as stale recognized words.

void test_tone_rejects_removed_siren_word_as_malformed(void) {
  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::Malformed),
                     static_cast<int>(parseHornCommand("SIREN")));
}

void test_tone_rejects_removed_engine_word_as_malformed(void) {
  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::Malformed),
                     static_cast<int>(parseHornCommand("ENGINE")));
}
