// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

// A mixed asynchronous reset must work while the gated clock is stopped.
module async_mixed_reset_repro (
  input logic clk_i,
  input logic rst_ni,
  input logic enable_i,
  input logic [2:0] data_i,
  output logic [2:0] state_o
);
  logic enabled;
  wire gated_clk;

  always_latch begin
    if (!clk_i) enabled = enable_i;
  end
  assign gated_clk = clk_i & enabled;

  always_ff @(posedge gated_clk or negedge rst_ni) begin
    if (!rst_ni) state_o <= 3'b101;
    else state_o <= data_i;
  end
endmodule
