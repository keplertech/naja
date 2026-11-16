# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0

"""Summarize macOS sample call trees inside the raw SystemVerilog load call.

Use exclusive node weights and ancestor membership so recursive frames do not
multiply inclusive counts. Idle worker threads and universe destruction outside
the load call are excluded. Percentages describe sampled main-thread stacks,
not aggregate CPU time across all threads. Raw reports can contain local paths;
keep them outside the repository.
"""

import argparse
import collections
import json
from pathlib import Path
import re


CATEGORIES = {
    "procedural_lowering": "::createSequentialLogic(",
    "combinational_procedural_lowering": "::lowerCombinationalProceduralBlock(",
    "continuous_assignments": "::createContinuousAssigns(",
    "loop_constant_lookup": "::getActiveForLoopConstant(",
    "loop_variable_scan": "::isActiveForLoopVariableExpr(",
    "expression_description": "::describeExpression(",
    "mux_emission": "::createMux2Instance(",
    "mux_cleanup": "::mergeRepeatedMuxSelections(",
    "source_annotation": "::annotateSourceInfo(",
    "source_lookup": "::getSourceInfo(",
    "slang_parse_all_sources": "::parseAllSources(",
    "alias_cleanup": "::collapseAnonymousAssignAliases(",
    "constant_int64": "::getConstantInt64(",
    "case_item_match": "::buildCaseItemMatchBit(",
    "procedural_lhs": "::applyCombinationalStatementForLhs(",
    "assigned_bit_mask": "::makeProceduralAssignedBitMask(",
    "cycle_check": "::checkCombinationalBlockCycles(",
    "term_order_update": "::SNLDesign::setOrderIDs(",
    "instance_post_create": "::SNLInstance::commonPostCreate(",
    "flat_collection_iterator": ">::NajaFlatCollectionIterator::findNextElement(",
}


def summarize(text):
    graph = text.split("Call graph:\n", 1)[1].split("Total number in stack", 1)[0]
    nodes, stack = [], []
    for line in graph.splitlines():
        match = re.match(r"^([ +!:|.]*)(\d+) (.*)$", line)
        if not match:
            continue
        depth, count = len(match[1]), int(match[2])
        name = match[3].split("  (in ", 1)[0]
        while stack and nodes[stack[-1]]["depth"] >= depth:
            stack.pop()
        parent = stack[-1] if stack else None
        index = len(nodes)
        nodes.append(dict(depth=depth, count=count, name=name, parent=parent, children=[]))
        if parent is not None:
            nodes[parent]["children"].append(index)
        stack.append(index)

    categories = collections.Counter()
    main_samples = load_samples = 0
    for index, node in enumerate(nodes):
        weight = node["count"] - sum(nodes[c]["count"] for c in node["children"])
        if weight < 0:
            raise ValueError("child counts exceed parent; unsupported or malformed sample tree")
        if not weight:
            continue
        path = []
        current = index
        while current is not None:
            path.append(nodes[current]["name"])
            current = nodes[current]["parent"]
        if "com.apple.main-thread" not in path[-1]:
            continue
        main_samples += weight
        if not any("PyNLDB_loadSystemVerilog(" in name for name in path):
            continue
        load_samples += weight
        for category, fragment in CATEGORIES.items():
            if any(fragment in name for name in path):
                categories[category] += weight
        loop_query = any("::getActiveForLoopConstant(" in name or
                         "::isActiveForLoopVariableExpr(" in name for name in path)
        if loop_query:
            categories["loop_query_union"] += weight
            if any("::describeExpression(" in name for name in path):
                categories["expression_description_inside_loop_query"] += weight
    if not load_samples:
        raise ValueError("no raw SystemVerilog load samples found")
    return dict(main_samples=main_samples, load_samples=load_samples,
        categories={name: dict(samples=categories[name],
                              percent=100 * categories[name] / load_samples)
                    for name in [*CATEGORIES, "loop_query_union",
                                 "expression_description_inside_loop_query"]})


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("sample", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    args.output.write_text(json.dumps(summarize(args.sample.read_text()), indent=2))


if __name__ == "__main__":
    main()
