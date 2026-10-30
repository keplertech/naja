-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
-- SPDX-License-Identifier: Apache-2.0
entity probe is port(a : in bit_vector(3 downto 0); q : out bit_vector(1 downto 0)); end;
architecture rtl of probe is alias low : bit_vector(1 downto 0) is a(1 downto 0);
begin q <= low; end;
