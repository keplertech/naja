najaeda
=======

.. image:: https://img.shields.io/pypi/v/najaeda
   :target: https://pypi.org/project/najaeda/
   :alt: PyPI version

.. image:: https://img.shields.io/badge/License-Apache_2.0-blue.svg
   :target: https://opensource.org/licenses/Apache-2.0
   :alt: Apache 2.0 License

.. image:: https://img.shields.io/badge/Matrix-Join%20Chat-success?logo=matrix
   :target: https://matrix.to/#/#naja:fossi-chat.org
   :alt: Join the Matrix chat

----

**najaeda** is a Python package for loading, elaborating, navigating, editing,
and analysing hardware designs from gate-level Verilog netlists to large
SystemVerilog RTL cores.

.. code-block:: python

    from najaeda import netlist

    # Load a gate-level Verilog design with a Liberty standard-cell library
    netlist.load_liberty(['NangateOpenCellLibrary.lib'])
    top = netlist.load_gate_level_verilog('my_design.v')

    # Navigate the hierarchy
    for inst in top.get_child_instances():
        print(f'{inst.get_name()} → {inst.get_model_name()}')

    # Flat connectivity across hierarchy boundaries
    for iterm in top.get_term('clk').get_equipotential().get_inst_terms():
        print(iterm)

    # Edit: rename, reconnect, delete
    top.get_net('old_name').set_name('new_name')


What you can do
---------------

- **Load** gate-level Verilog netlists and elaborate supported
  Verilog/SystemVerilog RTL, with or without Liberty standard-cell libraries
- **Navigate** hierarchy, nets, and ports at any level of detail —
  instance-by-instance or flat via equipotentials
- **Edit** netlists: rename instances and nets, disconnect and reconnect
  signals, delete logic
- **Analyse** designs with the visitor API and export results to pandas
  for further processing or visualisation

Installation
------------

.. code-block:: bash

    pip install najaeda

For the optional `schematic viewer <https://github.com/najaeda/naja-schematic>`_,
run ``pip install "najaeda[schematic]"``. In a notebook, use
``from najaeda.schematic import show; show()`` to display the currently loaded design.

Requires Python 3.10 or later. Pre-built wheels are published for:

* Linux x86_64 and AArch64 (``manylinux_2_28``; glibc 2.28 or later)
* macOS Apple Silicon (arm64), macOS 11 or later
* Windows x86_64

Building from a checkout
------------------------

Initialize the submodules, install the native build dependencies (including Flex
and Bison), then run ``python -m pip wheel .``. Parser-source selection is automatic:
a Git checkout generates the Verilog parser, while release source distributions
with bundled generated parser files use those files. An explicit
``-C cmake.define.PREGENERATED_PARSER_SOURCES=ON`` or ``OFF`` still overrides the
selection; ``ON`` requires the bundled files to exist.

Tutorials
---------

Six hands-on notebooks — open any of them in Google Colab with no local
install needed:

.. list-table::
   :header-rows: 1
   :widths: 5 60 15

   * - #
     - Topic
     - Colab
   * - 1
     - Getting started — load Verilog, navigate hierarchy, visualize
     - .. image:: https://colab.research.google.com/assets/colab-badge.svg
          :target: https://colab.research.google.com/github/keplertech/naja/blob/main/tutorials/notebooks/01_getting_started.ipynb
          :alt: Open in Colab
   * - 2
     - Liberty primitives — load a synthesised design with standard cells
     - .. image:: https://colab.research.google.com/assets/colab-badge.svg
          :target: https://colab.research.google.com/github/keplertech/naja/blob/main/tutorials/notebooks/02_liberty_primitives_design.ipynb
          :alt: Open in Colab
   * - 3
     - Editing a netlist — rename, disconnect, reconnect, delete
     - .. image:: https://colab.research.google.com/assets/colab-badge.svg
          :target: https://colab.research.google.com/github/keplertech/naja/blob/main/tutorials/notebooks/03_editing_a_netlist.ipynb
          :alt: Open in Colab
   * - 4
     - SystemVerilog elaboration — load and browse an elaborated SV design
     - .. image:: https://colab.research.google.com/assets/colab-badge.svg
          :target: https://colab.research.google.com/github/keplertech/naja/blob/main/tutorials/notebooks/04_systemverilog_elaborated_netlist.ipynb
          :alt: Open in Colab
   * - 5
     - ibex RISC-V core — explore a real-world SV core, collect module stats
     - .. image:: https://colab.research.google.com/assets/colab-badge.svg
          :target: https://colab.research.google.com/github/keplertech/naja/blob/main/tutorials/notebooks/05_ibex_riscv_core.ipynb
          :alt: Open in Colab
   * - 6
     - Fanout analysis — compute fanout for every net, trace drivers, export
       to pandas
     - .. image:: https://colab.research.google.com/assets/colab-badge.svg
          :target: https://colab.research.google.com/github/keplertech/naja/blob/main/tutorials/notebooks/06_fanout_analysis.ipynb
          :alt: Open in Colab

Links
-----

- `API documentation <https://najaeda.readthedocs.io/en/latest/>`_
- `GitHub repository <https://github.com/keplertech/naja>`_
- `Issue tracker <https://github.com/keplertech/naja/issues>`_
- `Matrix chat <https://matrix.to/#/#naja:fossi-chat.org>`_
- `contact@keplertech.io <mailto:contact@keplertech.io>`_

License
-------

Apache License 2.0.
See the `LICENSE <https://github.com/keplertech/naja/blob/main/LICENSE>`_ file for details.
