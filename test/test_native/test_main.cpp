#include <unity.h>

// Single Unity entry point for the whole test_native suite. Each test_*.cpp
// file in this directory declares its test functions here (extern) and adds
// them to the RUN_TEST list below — PlatformIO's native environment links
// one binary per test/<suite> directory, so only one main() may exist.

extern void test_host_pipeline_runs(void);

// test_command_parser.cpp
extern void test_parses_straight_stopped(void);
extern void test_parses_full_forward(void);
extern void test_parses_negative_steer_and_throttle(void);
extern void test_parses_right_reverse(void);
extern void test_rejects_non_numeric_garbage(void);
extern void test_rejects_empty_line(void);
extern void test_rejects_missing_throttle_field(void);
extern void test_rejects_extra_field(void);
extern void test_rejects_trailing_garbage(void);
extern void test_accepts_steer_and_throttle_at_min_boundary(void);
extern void test_accepts_steer_and_throttle_at_max_boundary(void);
extern void test_rejects_steer_below_min(void);
extern void test_rejects_steer_above_max(void);
extern void test_rejects_throttle_below_min(void);
extern void test_rejects_throttle_above_max(void);

// test_direction_control.cpp
extern void test_direction_stop(void);
extern void test_direction_forward(void);
extern void test_direction_backward(void);
extern void test_direction_left(void);
extern void test_direction_right(void);
extern void test_direction_forward_left(void);
extern void test_direction_forward_right(void);
extern void test_direction_backward_left(void);
extern void test_direction_backward_right(void);
extern void test_control_onCommand_returns_decided_direction(void);
extern void test_control_onTick_stops_once_on_disconnect(void);
extern void test_control_onTick_stops_once_on_timeout(void);
extern void test_control_onTick_no_emission_while_active_and_within_timeout(void);
extern void test_control_resumes_automatically_on_next_valid_command(void);

// test_command_to_direction_flow.cpp
extern void test_flow_valid_commands_produce_expected_directions(void);
extern void test_flow_disconnect_emits_single_stop_and_resumes(void);
extern void test_flow_malformed_line_does_not_crash_or_emit(void);
extern void test_flow_out_of_range_command_preserves_last_direction(void);

// test_connection_monitor.cpp
extern void test_connection_monitor_reports_connected_on_first_connect(void);
extern void test_connection_monitor_no_event_while_staying_connected(void);
extern void test_connection_monitor_reports_disconnected_after_connect(void);
extern void test_connection_monitor_no_event_while_staying_disconnected(void);
extern void test_connection_monitor_reports_connected_again_after_reconnect(void);

// test_connection_logging_flow.cpp
extern void test_flow_logs_connected_then_disconnected_then_reconnected(void);
extern void test_flow_logs_device_id_on_connect_and_retains_it_on_disconnect(void);

// test_motor_servo_vehicle_output.cpp
extern void test_output_forward_drives_forward_immediately(void);
extern void test_output_backward_drives_reverse_immediately(void);
extern void test_output_stop_stops_immediately(void);
extern void test_output_reversal_stops_immediately_then_waits_before_reversing(void);
extern void test_output_right_turn_auto_stops_after_pulse_duration_with_no_new_command(void);
extern void test_output_left_turn_auto_stops_after_pulse_duration_with_no_new_command(void);
extern void test_output_switching_turn_direction_restarts_the_pulse(void);
extern void test_output_stop_centers_servo_immediately_mid_turn_pulse(void);

// test_command_to_actuation_flow.cpp
extern void test_actuation_flow_forward_then_reverse_respects_pause(void);
extern void test_actuation_flow_steering_independent_of_motor(void);
extern void test_actuation_flow_disconnect_stops_motor_and_centers_servo_immediately(void);
extern void test_actuation_flow_disconnect_mid_pause_stays_stopped(void);
extern void test_actuation_flow_malformed_line_does_not_change_hardware_state(void);
extern void test_actuation_flow_turn_auto_stops_without_further_commands(void);
extern void test_actuation_flow_word_commands_up_then_right_stay_independent(void);

// test_drive_command_assembler.cpp
extern void test_assembler_up_sets_throttle_forward_steer_straight(void);
extern void test_assembler_down_sets_throttle_reverse(void);
extern void test_assembler_left_sets_steer_min(void);
extern void test_assembler_right_sets_steer_max(void);
extern void test_assembler_word_commands_are_case_insensitive(void);
extern void test_assembler_up_then_right_preserves_throttle(void);
extern void test_assembler_right_then_up_preserves_steer(void);
extern void test_assembler_numeric_command_overwrites_both_axes(void);
extern void test_assembler_rejects_unrecognized_word(void);
extern void test_assembler_reset_clears_stale_axis_state(void);

void setUp(void) {}
void tearDown(void) {}

int main(int argc, char **argv) {
  UNITY_BEGIN();

  RUN_TEST(test_host_pipeline_runs);

  RUN_TEST(test_parses_straight_stopped);
  RUN_TEST(test_parses_full_forward);
  RUN_TEST(test_parses_negative_steer_and_throttle);
  RUN_TEST(test_parses_right_reverse);
  RUN_TEST(test_rejects_non_numeric_garbage);
  RUN_TEST(test_rejects_empty_line);
  RUN_TEST(test_rejects_missing_throttle_field);
  RUN_TEST(test_rejects_extra_field);
  RUN_TEST(test_rejects_trailing_garbage);
  RUN_TEST(test_accepts_steer_and_throttle_at_min_boundary);
  RUN_TEST(test_accepts_steer_and_throttle_at_max_boundary);
  RUN_TEST(test_rejects_steer_below_min);
  RUN_TEST(test_rejects_steer_above_max);
  RUN_TEST(test_rejects_throttle_below_min);
  RUN_TEST(test_rejects_throttle_above_max);

  RUN_TEST(test_direction_stop);
  RUN_TEST(test_direction_forward);
  RUN_TEST(test_direction_backward);
  RUN_TEST(test_direction_left);
  RUN_TEST(test_direction_right);
  RUN_TEST(test_direction_forward_left);
  RUN_TEST(test_direction_forward_right);
  RUN_TEST(test_direction_backward_left);
  RUN_TEST(test_direction_backward_right);
  RUN_TEST(test_control_onCommand_returns_decided_direction);
  RUN_TEST(test_control_onTick_stops_once_on_disconnect);
  RUN_TEST(test_control_onTick_stops_once_on_timeout);
  RUN_TEST(test_control_onTick_no_emission_while_active_and_within_timeout);
  RUN_TEST(test_control_resumes_automatically_on_next_valid_command);

  RUN_TEST(test_flow_valid_commands_produce_expected_directions);
  RUN_TEST(test_flow_disconnect_emits_single_stop_and_resumes);
  RUN_TEST(test_flow_malformed_line_does_not_crash_or_emit);
  RUN_TEST(test_flow_out_of_range_command_preserves_last_direction);

  RUN_TEST(test_connection_monitor_reports_connected_on_first_connect);
  RUN_TEST(test_connection_monitor_no_event_while_staying_connected);
  RUN_TEST(test_connection_monitor_reports_disconnected_after_connect);
  RUN_TEST(test_connection_monitor_no_event_while_staying_disconnected);
  RUN_TEST(test_connection_monitor_reports_connected_again_after_reconnect);

  RUN_TEST(test_flow_logs_connected_then_disconnected_then_reconnected);
  RUN_TEST(test_flow_logs_device_id_on_connect_and_retains_it_on_disconnect);

  RUN_TEST(test_output_forward_drives_forward_immediately);
  RUN_TEST(test_output_backward_drives_reverse_immediately);
  RUN_TEST(test_output_stop_stops_immediately);
  RUN_TEST(test_output_reversal_stops_immediately_then_waits_before_reversing);
  RUN_TEST(test_output_right_turn_auto_stops_after_pulse_duration_with_no_new_command);
  RUN_TEST(test_output_left_turn_auto_stops_after_pulse_duration_with_no_new_command);
  RUN_TEST(test_output_switching_turn_direction_restarts_the_pulse);
  RUN_TEST(test_output_stop_centers_servo_immediately_mid_turn_pulse);

  RUN_TEST(test_actuation_flow_forward_then_reverse_respects_pause);
  RUN_TEST(test_actuation_flow_steering_independent_of_motor);
  RUN_TEST(test_actuation_flow_disconnect_stops_motor_and_centers_servo_immediately);
  RUN_TEST(test_actuation_flow_disconnect_mid_pause_stays_stopped);
  RUN_TEST(test_actuation_flow_malformed_line_does_not_change_hardware_state);
  RUN_TEST(test_actuation_flow_turn_auto_stops_without_further_commands);
  RUN_TEST(test_actuation_flow_word_commands_up_then_right_stay_independent);

  RUN_TEST(test_assembler_up_sets_throttle_forward_steer_straight);
  RUN_TEST(test_assembler_down_sets_throttle_reverse);
  RUN_TEST(test_assembler_left_sets_steer_min);
  RUN_TEST(test_assembler_right_sets_steer_max);
  RUN_TEST(test_assembler_word_commands_are_case_insensitive);
  RUN_TEST(test_assembler_up_then_right_preserves_throttle);
  RUN_TEST(test_assembler_right_then_up_preserves_steer);
  RUN_TEST(test_assembler_numeric_command_overwrites_both_axes);
  RUN_TEST(test_assembler_rejects_unrecognized_word);
  RUN_TEST(test_assembler_reset_clears_stale_axis_state);

  return UNITY_END();
}
