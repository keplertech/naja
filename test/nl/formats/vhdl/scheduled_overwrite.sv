// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

module scheduled_overwrite(input clk, en, input [1:0] s,
                           input [3:0] a, b, output reg [3:0] q, p, output reg held);
  always @(posedge clk) begin
    if (s[0]) q <= a;
    if (s[1]) q <= a;
    q <= b;
    p <= q;
  end
  always @(posedge clk) if (en) held <= b[0]; else held <= held;
endmodule
