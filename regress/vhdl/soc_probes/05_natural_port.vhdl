-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
-- SPDX-License-Identifier: Apache-2.0
entity probe is port(a : in natural range 0 to 255; q : out bit); end;
architecture rtl of probe is begin q <= '1' when a = 255 else '0'; end;
