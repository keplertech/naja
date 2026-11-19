-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
-- SPDX-License-Identifier: Apache-2.0

library ieee;
use ieee.std_logic_1164.all;

entity conv is
  port (a_i : in std_ulogic;
        a_o : out std_logic);
end entity;

architecture rtl of conv is
begin
  a_o <= std_logic(a_i);
end architecture;
