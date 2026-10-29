-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
-- SPDX-License-Identifier: Apache-2.0
entity probe is port(a : in bit; q : out bit); end;
architecture rtl of probe is begin process(all)
type choice_t is (idle, active); variable choice : choice_t;
begin choice := idle; if a = '1' then choice := active; end if;
if choice = active then q <= '1'; else q <= '0'; end if;
end process; end;
