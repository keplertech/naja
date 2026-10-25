# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0

"""Synthetic high-level najaeda workflows; fixture setup is never measured."""

import pytest

from najaeda import instance_visitor, naja, netlist, stats


@pytest.fixture(params=[16, 128, 1024], ids=lambda groups: f"groups={groups}")
def hierarchy(request):
    netlist.reset()
    groups = request.param
    try:
        top = netlist.create_top("top")
        universe = naja.NLUniverse.get()
        design = universe.getTopDesign()
        primitives = naja.NLLibrary.createPrimitives(design.getDB(), "PRIMITIVES")
        leaf = naja.SNLDesign.createPrimitive(primitives, "leaf")
        leaf_input = naja.SNLScalarTerm.create(leaf, naja.SNLTerm.Direction.Input, "i")
        block = naja.SNLDesign.create(design.getLibrary(), "block")
        block_input = naja.SNLScalarTerm.create(block, naja.SNLTerm.Direction.Input, "i")
        block_net = naja.SNLScalarNet.create(block, "fanout")
        block_input.setNet(block_net)
        for index in range(8):
            instance = naja.SNLInstance.create(block, leaf, f"u{index}")
            instance.getInstTerm(leaf_input).setNet(block_net)
        top_input = naja.SNLScalarTerm.create(design, naja.SNLTerm.Direction.Input, "i")
        top_net = naja.SNLScalarNet.create(design, "fanout")
        top_input.setNet(top_net)
        for index in range(groups):
            instance = naja.SNLInstance.create(design, block, f"u{index}")
            instance.getInstTerm(block_input).setNet(top_net)
        assert top.count_child_instances() == groups
        yield top, groups
    finally:
        netlist.reset()


def test_leaf_traversal(benchmark, hierarchy):
    top, groups = hierarchy
    count = benchmark(lambda: sum(1 for _ in top.get_leaf_children()))
    assert count == groups * 8


def test_connectivity(benchmark, hierarchy):
    top, groups = hierarchy
    term = top.get_term("i")

    def readers():
        return sum(1 for _ in term.get_equipotential().get_leaf_readers())

    assert benchmark(readers) == groups * 8


def test_statistics(benchmark, hierarchy):
    top, groups = hierarchy

    def collect():
        # A fresh cache is essential: reusing it would benchmark cache hits.
        return stats.compute_instance_stats(top, stats.InstancesStats())

    result = benchmark(collect)
    assert sum(result.flat_primitives.values()) == groups * 8
    assert sum(result.ins.values()) == groups


def test_instance_visitor(benchmark, hierarchy):
    top, groups = hierarchy

    def visit():
        count = 0

        def record(instance):
            nonlocal count
            count += 1

        instance_visitor.visit(top, instance_visitor.VisitorConfig(callback=record))
        return count

    assert benchmark(visit) == 1 + groups * 9
