-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
-- SPDX-License-Identifier: Apache-2.0
library ieee; use ieee.std_logic_1164.all;
entity child is port(clk,d : in std_logic; q : out std_logic); end;
architecture rtl of child is begin
process(clk) begin if rising_edge(clk) then q <= d; end if; end process; end;
library ieee; use ieee.std_logic_1164.all;
entity probe is port(clk,d : in std_logic; q : out std_logic); end;
architecture rtl of probe is signal s : std_logic := '0';
begin u : entity work.child port map(clk,d,s); q <= s; end;
