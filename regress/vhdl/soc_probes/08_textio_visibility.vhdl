-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
-- SPDX-License-Identifier: Apache-2.0
use std.textio.all; use std.env.stop;
entity probe is port(a : in bit; q : out bit); end;
architecture rtl of probe is begin q <= a; end;
