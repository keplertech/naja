-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
-- SPDX-License-Identifier: Apache-2.0
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use std.textio.all;
entity probe is
  generic(filename : string := "image.hex");
  port(clk, we : in std_logic; addr : in unsigned(2 downto 0);
       d : in std_logic_vector(11 downto 0); q : out std_logic_vector(11 downto 0));
end;
architecture rtl of probe is
  type words_t is array(0 to 6) of std_logic_vector(11 downto 0);
  impure function load_image(path : string) return words_t is
    file input_file : text open read_mode is path;
    variable row : line;
    variable word : std_logic_vector(11 downto 0);
    variable result : words_t := (others => (others => '0'));
  begin
    for i in result'range loop
      exit when endfile(input_file);
      readline(input_file, row);
      hread(row, word);
      result(i) := word;
    end loop;
    return result;
  end;
  signal storage : words_t := load_image(filename);
begin
  process(clk) begin
    if rising_edge(clk) then
      if we='1' then storage(to_integer(addr)) <= d; end if;
      q <= storage(to_integer(addr));
    end if;
  end process;
end;
