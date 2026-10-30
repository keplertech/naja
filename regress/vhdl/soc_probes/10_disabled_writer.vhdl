-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
-- SPDX-License-Identifier: Apache-2.0
entity probe is generic(enabled : boolean := false); port(a : in bit; q : out bit_vector(2 downto 0)); end;
architecture rtl of probe is signal stored : bit_vector(2 downto 0) := "101";
begin writer : if enabled generate stored <= (others => a); end generate;
q <= stored; end;
