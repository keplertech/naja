-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
-- SPDX-License-Identifier: Apache-2.0
entity probe is port(a : in bit; q : out bit := '0'); end;
architecture rtl of probe is begin q <= a; end;
