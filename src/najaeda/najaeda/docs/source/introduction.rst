Introduction
============

``najaeda`` is an Electronic Design Automation (EDA) Python package for
loading, elaborating, navigating, analyzing, and editing hardware designs from
RTL SystemVerilog through structural netlists.  It gives Python users access
to Naja's SNL database: designs, libraries, instances, terms, nets,
attributes, hierarchical paths, occurrences, and equipotentials.

Typical use cases include:

* loading Verilog, elaborated SystemVerilog, experimental VHDL, Liberty,
  primitive libraries, and
  Naja interchange files;
* exploring hierarchy, ports, nets, instance terminals, and flat connectivity;
* writing netlist analysis passes such as fanout, logic-level, and design
  inventory reports;
* applying ECO-style edits such as renaming, reconnecting, deleting, and
  uniquifying designs as needed;
* exporting data to text, JSON, pandas, DOT, Verilog, or Naja interchange for
  downstream tooling.

The recommended API entry point is :mod:`najaeda.netlist`.  It keeps object
identity stable across hierarchy, converts native C++ objects into Python
wrappers, and performs automatic uniquification before edits that would
otherwise affect shared models.

An expert raw API is also available as :mod:`najaeda.naja`.  It exposes the
compiled ``naja`` extension module and mirrors the underlying SNL C++ model
more closely.  See :doc:`raw_api` when you need this lower level.

Information about the **najaeda** PyPI package is available at https://pypi.org/project/najaeda .

If you want more details about the underlying **naja** C++ library,
please visit the **naja** GitHub repository at https://github.com/keplertech/naja .

Quick Start
===========

To get started quickly, try the interactive notebook in Google Colab:

.. image:: https://colab.research.google.com/assets/colab-badge.svg
   :target: https://colab.research.google.com/github/najaeda/najaeda-tutorials/blob/main/notebooks/01_getting_started.ipynb
   :alt: Open in Colab

For a local script-oriented introduction, see :doc:`quickstart`.

Installation
------------
**najaeda** can be easily installed using pip:

.. code-block:: bash
    
    pip install najaeda

For the optional `schematic viewer <https://github.com/najaeda/naja-schematic>`_,
run ``pip install "najaeda[schematic]"``. In a notebook, use
``from najaeda.schematic import show; show()`` to display the currently loaded design.
See :doc:`schematic` for notebook examples, selection, and diagnosis overlays.

Requires Python 3.10 or later. Pre-built wheels are published for:

* Linux x86_64 and AArch64 (``manylinux_2_28``; glibc 2.28 or later)
* macOS Apple Silicon (arm64), macOS 11 or later
* Windows x86_64

Bug Reports
-----------

If you encounter any bugs, please report them on the `najaeda` issue tracker:
https://github.com/keplertech/naja/issues

You’re also welcome to join the discussion on Matrix:

.. image:: https://img.shields.io/badge/Matrix-Join%20Chat-success?logo=matrix
   :target: https://matrix.to/#/#naja:fossi-chat.org
   :alt: Join the Matrix chat

License
-------
This project is licensed under the Apache License 2.0.
See the `LICENSE <https://github.com/keplertech/naja/blob/main/LICENSE>`_ file for details.
