-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
-- SPDX-License-Identifier: Apache-2.0
entity probe is generic(filename : string := "hello.hex"); port(q : out bit); end;
architecture rtl of probe is begin q <= '0'; end;
