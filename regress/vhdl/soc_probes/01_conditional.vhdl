-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
-- SPDX-License-Identifier: Apache-2.0
entity probe is port(clk, en, d : in bit; q : out bit); end;
architecture rtl of probe is begin process(clk) begin
if rising_edge(clk) then q <= d when en = '1'; end if;
end process; end;
