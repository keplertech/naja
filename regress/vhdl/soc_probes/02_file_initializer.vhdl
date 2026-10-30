-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
-- SPDX-License-Identifier: Apache-2.0
use std.textio.all;
entity probe is port(q : out bit); end;
architecture rtl of probe is
impure function init return bit is
file source : text open read_mode is "data.hex";
begin return '0'; end;
begin q <= init; end;
