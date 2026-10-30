-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
-- SPDX-License-Identifier: Apache-2.0
entity probe is port(q : out bit); end;
architecture rtl of probe is impure function init return bit is begin return '1'; end;
begin q <= init; end;
