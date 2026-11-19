-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
-- SPDX-License-Identifier: Apache-2.0

library ieee;
use ieee.std_logic_1164.all;

entity conv_vec is
  port (v_i : in std_ulogic_vector(3 downto 0);
        v_o : out std_logic_vector(3 downto 0));
end entity;

architecture rtl of conv_vec is
begin
  v_o <= std_logic_vector(v_i);
end architecture;
