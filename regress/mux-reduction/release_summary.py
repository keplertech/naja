# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0

"""Summarize balanced Release samples without discarding observations."""

import argparse
import collections
import csv
import json
from pathlib import Path
import random
import statistics


def percentile(values, fraction):
    position = (len(values) - 1) * fraction
    low = int(position)
    high = min(low + 1, len(values) - 1)
    return values[low] + (values[high] - values[low]) * (position - low)


def bootstrap_median(values, seed, resamples=10000):
    rng = random.Random(seed)
    estimates = sorted(statistics.median(rng.choices(values, k=len(values)))
                       for _ in range(resamples))
    return dict(median=statistics.median(values), low=percentile(estimates, .025),
                high=percentile(estimates, .975))


def summarize(path):
    with path.open(newline="") as stream:
        rows = list(csv.DictReader(stream))
    groups = collections.defaultdict(dict)
    labels = []
    workloads = []
    for row in rows:
        label, workload, block = row["label"], row["workload"], int(row["block"])
        if label not in labels:
            labels.append(label)
        if workload not in workloads:
            workloads.append(workload)
        if block in groups[workload, label]:
            raise ValueError("duplicate workload/revision/block")
        row["load_ms"] = float(row["load_ms"])
        row["peak_rss_bytes"] = int(row["peak_rss_bytes"])
        groups[workload, label][block] = row
    if set(labels) != {"before", "after535", "after536"}:
        raise ValueError("expected before, after535 and after536")
    labels = ["before", "after535", "after536"]
    block_set = set(next(iter(groups.values())))
    if len(block_set) != 18 or any(set(group) != block_set for group in groups.values()):
        raise ValueError("expected the same 18 blocks in every group")
    if len(groups) != len(workloads) * len(labels):
        raise ValueError("missing workload/revision group")
    result = {}
    for workload in sorted(workloads):
        summary = {}
        for label in labels:
            group = groups[workload, label]
            values = list(group.values())
            counts = {tuple(row[k] for k in ("instances", "muxes", "mux_bits", "nets", "bitnets"))
                      for row in values}
            if len(counts) != 1:
                raise ValueError("structural counts changed within a group")
            summary[label] = {}
            for metric, scale in [("load_ms", 1), ("peak_rss_bytes", 1048576)]:
                numbers = [row[metric] / scale for row in values]
                summary[label][metric] = dict(median=statistics.median(numbers),
                                              low=min(numbers), high=max(numbers))
        for numerator, denominator in [("after535", "before"), ("after536", "before"),
                                       ("after536", "after535")]:
            pairs = [(groups[workload, numerator][b], groups[workload, denominator][b])
                     for b in sorted(block_set)]
            time_ratios = [new["load_ms"] / old["load_ms"] for new, old in pairs]
            rss_differences = [(new["peak_rss_bytes"] - old["peak_rss_bytes"]) / 1048576
                               for new, old in pairs]
            time = bootstrap_median(time_ratios, 536535)
            summary[f"{numerator}/{denominator}"] = dict(
                time_percent={k: (v - 1) * 100 for k, v in time.items()},
                rss_mib=bootstrap_median(rss_differences, 536535))
        result[workload] = summary
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("samples", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    result = summarize(args.samples)
    args.output.write_text(json.dumps(result, indent=2, sort_keys=True))
    for name, values in result.items():
        medians = [f"{values[label]['load_ms']['median']:.3f}" for label in
                   ("before", "after535", "after536")]
        comparison = values["after536/before"]
        time, rss = comparison["time_percent"], comparison["rss_mib"]
        print(f"{name}: ms {' / '.join(medians)}; final/before "
              f"{time['median']:+.2f}% [{time['low']:+.2f}, {time['high']:+.2f}]; "
              f"RSS {rss['median']:+.3f} MiB [{rss['low']:+.3f}, {rss['high']:+.3f}]")


if __name__ == "__main__":
    main()
