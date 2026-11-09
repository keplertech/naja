-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
-- SPDX-License-Identifier: Apache-2.0

library ieee;
use ieee.std_logic_1164.all;
entity scheduled_overwrite is
  port(clk, en : in std_logic; s : in std_logic_vector(1 downto 0);
       a, b : in std_logic_vector(3 downto 0);
       q, p : out std_logic_vector(3 downto 0); held : out std_logic);
end;
architecture rtl of scheduled_overwrite is begin
  process(clk) begin
    if rising_edge(clk) then
      if s(0) = '1' then q <= a; end if;
      if s(1) = '1' then q <= a; end if;
      q <= b;
      p <= q;
    end if;
  end process;
  process(clk) begin
    if rising_edge(clk) then
      if en = '1' then held <= b(0); end if;
    end if;
  end process;
end;
