// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

module async_mixed_reset_tb;
  timeunit 1ns;
  timeprecision 1ps;

  logic clk = 0;
  logic reset_n = 1;
  wire [2:0] state_value;

  always #5ns clk = ~clk;

  async_mixed_reset_repro dut (
    .clk_i(clk),
    .rst_ni(reset_n),
    .enable_i(1'b0),
    .data_i(3'b010),
    .state_o(state_value)
  );

  initial begin
    #2ns;
    reset_n = 0;
    #4ns;
    reset_n = 1;
    #1ns;
    $display("RESET_RESULT state=%b", state_value);
    assert (state_value === 3'b101)
      else $fatal(1, "Mixed asynchronous reset was lost");
    $finish;
  end
endmodule
