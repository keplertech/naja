-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
-- SPDX-License-Identifier: Apache-2.0
entity probe is port(a : in bit; q : out bit); end;
architecture rtl of probe is begin process(all)
function inv(v : bit) return bit is begin return not v; end;
begin q <= inv(a); end process; end;
