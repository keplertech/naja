# najaeda Tutorials

Interactive notebooks for learning the `najaeda` Python API.
Each chapter can be opened directly in Google Colab — no local install needed.

| # | Chapter | Open in Colab |
| --- | --- | --- |
| 1 | [Getting Started](notebooks/01_getting_started.ipynb) — load a Verilog netlist, navigate hierarchy and connectivity | [![Colab](https://colab.research.google.com/assets/colab-badge.svg)](https://colab.research.google.com/github/keplertech/naja/blob/main/tutorials/notebooks/01_getting_started.ipynb) |
| 2 | [Liberty Primitives and Design](notebooks/02_liberty_primitives_design.ipynb) — load a gate-level design with Liberty primitives, browse and collect stats | [![Colab](https://colab.research.google.com/assets/colab-badge.svg)](https://colab.research.google.com/github/keplertech/naja/blob/main/tutorials/notebooks/02_liberty_primitives_design.ipynb) |
| 3 | [Editing a Netlist](notebooks/03_editing_a_netlist.ipynb) — rename instances and nets, disconnect and reconnect signals, delete logic | [![Colab](https://colab.research.google.com/assets/colab-badge.svg)](https://colab.research.google.com/github/keplertech/naja/blob/main/tutorials/notebooks/03_editing_a_netlist.ipynb) |
| 4 | [SystemVerilog Elaborated Netlist](notebooks/04_systemverilog_elaborated_netlist.ipynb) — load a parameterised SV design and inspect the elaborated hierarchy | [![Colab](https://colab.research.google.com/assets/colab-badge.svg)](https://colab.research.google.com/github/keplertech/naja/blob/main/tutorials/notebooks/04_systemverilog_elaborated_netlist.ipynb) |
| 5 | [Exploring ibex, a RISC-V Core](notebooks/05_ibex_riscv_core.ipynb) — load a real-world SV core, navigate the pipeline hierarchy, inspect RTL source ranges, collect module statistics | [![Colab](https://colab.research.google.com/assets/colab-badge.svg)](https://colab.research.google.com/github/keplertech/naja/blob/main/tutorials/notebooks/05_ibex_riscv_core.ipynb) |
| 6 | [Gate-Level Analysis: Fanout and Net Statistics](notebooks/06_fanout_analysis.ipynb) — compute fanout for every net using the Equipotential API, trace drivers, export to pandas | [![Colab](https://colab.research.google.com/assets/colab-badge.svg)](https://colab.research.google.com/github/keplertech/naja/blob/main/tutorials/notebooks/06_fanout_analysis.ipynb) |

## Benchmarks

Design files used by the tutorials are in [`benchmarks/`](benchmarks/):

- `benchmarks/liberty/` — NangateOpenCellLibrary and FakeRAM Liberty files (used by Chapter 2)
- `benchmarks/verilog/tinyrocket/` — TinyRocket gate-level netlist (used by Chapter 2)
- `benchmarks/verilog/adder/` — simple full-adder (reference)

## Running Locally

```bash
pip install najaeda==0.7.25 jupyter nbmake nbstripout
# one-time git hook to keep notebooks clean
nbstripout --install
# run all tutorials as a test suite
pytest --nbmake tutorials/notebooks/
```
