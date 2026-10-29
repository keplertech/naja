// SPDX-FileCopyrightText: 2026 The Naja authors
//
// SPDX-License-Identifier: Apache-2.0

// Primitive behavioral shims used to simulate najaeda-generated Verilog.
// Canonical module names are naja_* to match NLDB0 primitive names.

module naja_fa(
  input wire A,
  input wire B,
  input wire CI,
  output wire S,
  output wire CO
);
  assign {CO, S} = A + B + CI;
endmodule

module naja_mux2 #(
  parameter WIDTH = 1
) (
  input wire [WIDTH-1:0] A,
  input wire [WIDTH-1:0] B,
  input wire S,
  output wire [WIDTH-1:0] Y
);
  assign Y = S ? B : A;
endmodule

module naja_mul #(parameter WIDTH = 1) (
  input wire [WIDTH-1:0] A,
  input wire [WIDTH-1:0] B,
  output wire [WIDTH-1:0] Y
);
  assign Y = A * B;
endmodule

module naja_shift #(
  parameter WIDTH = 1,
  parameter COUNT_WIDTH = 1,
  parameter RIGHT = 0,
  parameter ARITHMETIC = 0
) (
  input wire [WIDTH-1:0] DATA,
  input wire [COUNT_WIDTH-1:0] COUNT,
  output wire [WIDTH-1:0] Y
);
  generate
    if (RIGHT && ARITHMETIC) begin
      assign Y = $signed(DATA) >>> COUNT;
    end else if (RIGHT) begin
      assign Y = DATA >> COUNT;
    end else begin
      assign Y = DATA << COUNT;
    end
  endgenerate
endmodule

module naja_table_select #(
  parameter WIDTH = 1,
  parameter DEPTH = 1,
  parameter ABITS = 1
) (
  input wire [WIDTH*DEPTH-1:0] DATA,
  input wire [ABITS-1:0] ADDR,
  output wire [WIDTH-1:0] Y
);
  function [WIDTH-1:0] select_data;
    input [WIDTH*DEPTH-1:0] data;
    input [ABITS-1:0] addr;
    integer i;
    begin
      // Match SV packed indexing for unknown and out-of-range addresses.
      select_data = {WIDTH{1'bx}};
      for (i = 0; i < DEPTH; i = i + 1) begin
        if (addr == i[ABITS-1:0]) begin
          select_data = data[i*WIDTH +: WIDTH];
        end
      end
    end
  endfunction

  assign Y = select_data(DATA, ADDR);
endmodule

module naja_dff #(
  parameter WIDTH = 1,
  parameter INIT = {WIDTH{1'bx}}
) (
  input wire C,
  input wire [WIDTH-1:0] D,
  output reg [WIDTH-1:0] Q
);
  initial Q = INIT;
  always @(posedge C) begin
    Q <= D;
  end
endmodule

/* verilator lint_off LATCH */
module naja_dlatch #(
  parameter WIDTH = 1
) (
  input wire E,
  input wire [WIDTH-1:0] D,
  output reg [WIDTH-1:0] Q
);
  always @* begin
    if (E) Q = D;
  end
endmodule
/* verilator lint_on LATCH */

module naja_dffn #(
  parameter WIDTH = 1,
  parameter INIT = {WIDTH{1'bx}}
) (
  input wire C,
  input wire [WIDTH-1:0] D,
  output reg [WIDTH-1:0] Q
);
  initial Q = INIT;
  always @(negedge C) begin
    Q <= D;
  end
endmodule

module naja_dffrn #(
  parameter WIDTH = 1,
  parameter INIT = {WIDTH{1'bx}}
) (
  input wire C,
  input wire [WIDTH-1:0] D,
  input wire RN,
  output reg [WIDTH-1:0] Q
);
  initial Q = INIT;
  always @(posedge C or negedge RN) begin
    if (!RN) Q <= {WIDTH{1'b0}};
    else Q <= D;
  end
endmodule

module naja_dffr #(
  parameter WIDTH = 1,
  parameter INIT = {WIDTH{1'bx}}
) (
  input wire C,
  input wire [WIDTH-1:0] D,
  input wire R,
  output reg [WIDTH-1:0] Q
);
  initial Q = INIT;
  always @(posedge C or posedge R) begin
    if (R) Q <= {WIDTH{1'b0}};
    else Q <= D;
  end
endmodule

module naja_dffs #(
  parameter WIDTH = 1,
  parameter INIT = {WIDTH{1'bx}}
) (
  input wire C,
  input wire [WIDTH-1:0] D,
  input wire S,
  output reg [WIDTH-1:0] Q
);
  initial Q = INIT;
  always @(posedge C or posedge S) begin
    if (S) Q <= {WIDTH{1'b1}};
    else Q <= D;
  end
endmodule

module naja_dffe #(
  parameter WIDTH = 1,
  parameter INIT = {WIDTH{1'bx}}
) (
  input wire C,
  input wire [WIDTH-1:0] D,
  input wire E,
  output reg [WIDTH-1:0] Q
);
  initial Q = INIT;
  always @(posedge C) begin
    if (E) Q <= D;
  end
endmodule

module naja_dffre #(
  parameter WIDTH = 1,
  parameter INIT = {WIDTH{1'bx}}
) (
  input wire C,
  input wire [WIDTH-1:0] D,
  input wire E,
  input wire R,
  output reg [WIDTH-1:0] Q
);
  initial Q = INIT;
  always @(posedge C or posedge R) begin
    if (R) Q <= {WIDTH{1'b0}};
    else if (E) Q <= D;
  end
endmodule

module naja_dffse #(
  parameter WIDTH = 1,
  parameter INIT = {WIDTH{1'bx}}
) (
  input wire C,
  input wire [WIDTH-1:0] D,
  input wire E,
  input wire S,
  output reg [WIDTH-1:0] Q
);
  initial Q = INIT;
  always @(posedge C or posedge S) begin
    if (S) Q <= {WIDTH{1'b1}};
    else if (E) Q <= D;
  end
endmodule

module naja_dffsr #(
  parameter WIDTH = 1,
  parameter INIT = {WIDTH{1'bx}}
) (
  input wire C,
  input wire [WIDTH-1:0] D,
  input wire R,
  output reg [WIDTH-1:0] Q
);
  initial Q = INIT;
  always @(posedge C) begin
    if (R) Q <= {WIDTH{1'b0}};
    else Q <= D;
  end
endmodule

module naja_dffsrn #(
  parameter WIDTH = 1,
  parameter INIT = {WIDTH{1'bx}}
) (
  input wire C,
  input wire [WIDTH-1:0] D,
  input wire RN,
  output reg [WIDTH-1:0] Q
);
  initial Q = INIT;
  always @(posedge C) begin
    if (!RN) Q <= {WIDTH{1'b0}};
    else Q <= D;
  end
endmodule

module naja_dffss #(
  parameter WIDTH = 1,
  parameter INIT = {WIDTH{1'bx}}
) (
  input wire C,
  input wire [WIDTH-1:0] D,
  input wire S,
  output reg [WIDTH-1:0] Q
);
  initial Q = INIT;
  always @(posedge C) begin
    if (S) Q <= {WIDTH{1'b1}};
    else Q <= D;
  end
endmodule

module naja_dffssn #(
  parameter WIDTH = 1,
  parameter INIT = {WIDTH{1'bx}}
) (
  input wire C,
  input wire [WIDTH-1:0] D,
  input wire SN,
  output reg [WIDTH-1:0] Q
);
  initial Q = INIT;
  always @(posedge C) begin
    if (!SN) Q <= {WIDTH{1'b1}};
    else Q <= D;
  end
endmodule

module naja_dffsre #(
  parameter WIDTH = 1,
  parameter INIT = {WIDTH{1'bx}}
) (
  input wire C,
  input wire [WIDTH-1:0] D,
  input wire E,
  input wire R,
  output reg [WIDTH-1:0] Q
);
  initial Q = INIT;
  always @(posedge C) begin
    if (R) Q <= {WIDTH{1'b0}};
    else if (E) Q <= D;
  end
endmodule

module naja_dffsrne #(
  parameter WIDTH = 1,
  parameter INIT = {WIDTH{1'bx}}
) (
  input wire C,
  input wire [WIDTH-1:0] D,
  input wire E,
  input wire RN,
  output reg [WIDTH-1:0] Q
);
  initial Q = INIT;
  always @(posedge C) begin
    if (!RN) Q <= {WIDTH{1'b0}};
    else if (E) Q <= D;
  end
endmodule

module naja_dffsse #(
  parameter WIDTH = 1,
  parameter INIT = {WIDTH{1'bx}}
) (
  input wire C,
  input wire [WIDTH-1:0] D,
  input wire E,
  input wire S,
  output reg [WIDTH-1:0] Q
);
  initial Q = INIT;
  always @(posedge C) begin
    if (S) Q <= {WIDTH{1'b1}};
    else if (E) Q <= D;
  end
endmodule

module naja_dffssne #(
  parameter WIDTH = 1,
  parameter INIT = {WIDTH{1'bx}}
) (
  input wire C,
  input wire [WIDTH-1:0] D,
  input wire E,
  input wire SN,
  output reg [WIDTH-1:0] Q
);
  initial Q = INIT;
  always @(posedge C) begin
    if (!SN) Q <= {WIDTH{1'b1}};
    else if (E) Q <= D;
  end
endmodule

module naja_mem #(
  parameter WIDTH = 1,
  parameter DEPTH = 1,
  parameter ABITS = 1,
  parameter RD_PORTS = 1,
  parameter WR_PORTS = 1,
  parameter MULTI_CLOCK = 0,
  parameter RST_ENABLE = 0,
  parameter RST_ASYNC = 0,
  parameter RST_ACTIVE_LOW = 0,
  parameter INIT_ENABLE = 0,
  parameter INIT = 1'b0
) (
  input CLK,
  input RST,
  input [RD_PORTS*ABITS-1:0] RADDR,
  output reg [RD_PORTS*WIDTH-1:0] RDATA,
  input [(WR_PORTS > 0 ? WR_PORTS : 1)*ABITS-1:0] WADDR,
  input [(WR_PORTS > 0 ? WR_PORTS : 1)*WIDTH-1:0] WDATA,
  input [(WR_PORTS > 0 ? WR_PORTS : 1)-1:0] WE,
  input [(WR_PORTS > 0 ? WR_PORTS : 1)-1:0] WCLK,
  input [(WR_PORTS > 0 ? WR_PORTS : 1)*WIDTH-1:0] WMASK
);

  reg [WIDTH-1:0] mem [0:DEPTH-1];
  integer i;
  integer rp;
  integer addr_index;
  reg [ABITS-1:0] addr_value;

  task automatic load_init;
    integer init_idx;
    begin
      /* verilator lint_off SELRANGE */
      for (init_idx = 0; init_idx < DEPTH; init_idx = init_idx + 1)
        if (INIT === '0) mem[init_idx] = WIDTH'(0);
        else mem[init_idx] = INIT[init_idx*WIDTH +: WIDTH];
      /* verilator lint_on SELRANGE */
    end
  endtask

  task automatic write_ports;
    integer wp;
    integer later;
    integer addr_index;
    reg allow_write;
    reg [ABITS-1:0] addr_value;
    begin
      for (wp = 0; wp < WR_PORTS; wp = wp + 1) begin
        allow_write = WE[WR_PORTS-1-wp];
        addr_value = WADDR[wp*ABITS +: ABITS];
        for (later = wp + 1; later < WR_PORTS; later = later + 1) begin
          if (WE[WR_PORTS-1-later] && WADDR[later*ABITS +: ABITS] == addr_value)
            allow_write = 1'b0;
        end
        addr_index = integer'(addr_value);
        if (allow_write && addr_index < DEPTH)
          mem[addr_index] <= WDATA[wp*WIDTH +: WIDTH];
      end
    end
  endtask

  wire reset_active = RST_ENABLE && (RST_ACTIVE_LOW ? ~RST : RST);

  initial begin
    if (INIT_ENABLE)
      load_init();
  end

  always @* begin
    for (rp = 0; rp < RD_PORTS; rp = rp + 1) begin
      addr_value = RADDR[rp*ABITS +: ABITS];
      addr_index = integer'(addr_value);
      if (addr_index < DEPTH)
        RDATA[rp*WIDTH +: WIDTH] = mem[addr_index];
      else
        RDATA[rp*WIDTH +: WIDTH] = WR_PORTS == 0 ? WIDTH'(1'sbx) : WIDTH'(0);
    end
  end

  generate
    if (WR_PORTS == 0) begin : read_only
    end else if (MULTI_CLOCK) begin : independent_write_clocks
      genvar port, bit_index;
      for (port = 0; port < WR_PORTS; port = port + 1) begin : write_port
        for (bit_index = 0; bit_index < WIDTH; bit_index = bit_index + 1) begin : write_bit
          always @(posedge WCLK[port]) begin
            if (WE[WR_PORTS-1-port] && WMASK[port*WIDTH+bit_index] &&
                WADDR[port*ABITS +: ABITS] < DEPTH)
              mem[WADDR[port*ABITS +: ABITS]][bit_index] <= WDATA[port*WIDTH+bit_index];
          end
        end
      end
    end else if (RST_ENABLE && RST_ASYNC && RST_ACTIVE_LOW) begin : async_low_reset
      always @(posedge CLK or negedge RST) begin
        if (!RST)
          load_init();
        else
          write_ports();
      end
    end else if (RST_ENABLE && RST_ASYNC) begin : async_high_reset
      always @(posedge CLK or posedge RST) begin
        if (RST)
          load_init();
        else
          write_ports();
      end
    end else if (RST_ENABLE) begin : sync_reset
      always @(posedge CLK) begin
        if (reset_active)
          load_init();
        else
          write_ports();
      end
    end else begin : no_reset
      always @(posedge CLK) begin
        write_ports();
      end
    end
  endgenerate
endmodule

module naja_add #(parameter WIDTH = 1) (
  input [WIDTH-1:0] A, B,
  output [WIDTH-1:0] Y
);
  assign Y = A + B;
endmodule

module naja_sub #(parameter WIDTH = 1) (
  input [WIDTH-1:0] A, B,
  output [WIDTH-1:0] Y
);
  assign Y = A - B;
endmodule
