-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
-- SPDX-License-Identifier: Apache-2.0
entity probe is port(a : in bit; q : out bit); end;
architecture rtl of probe is begin process(all)
subtype flag is bit; variable v : flag;
begin v := a; q <= v; end process; end;
