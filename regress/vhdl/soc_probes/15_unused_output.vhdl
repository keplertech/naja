-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
-- SPDX-License-Identifier: Apache-2.0
entity disabled_peripheral is port(unused : out bit); end;
architecture rtl of disabled_peripheral is begin end;
entity probe is port(q : out bit); end;
architecture rtl of probe is begin
u : entity work.disabled_peripheral port map(unused => open); q <= '0'; end;
