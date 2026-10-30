# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import logging
import json

from najaeda import netlist

logger = logging.getLogger(__name__)


class InstancesStats:
    def __init__(self):
        self.blackboxes = dict()
        self.hier_instances = dict()

    def dump_instance_stats_text(self, instance, stats_files):
        dumped_models = set()
        self.__dump_instance_stats_text(instance, stats_files, dumped_models)

    def __dump_stats(self, file, value_name, value):
        file.write(f"{value_name}: {value} \n")

    def __dump_terms_stats_text(self, instance_stats, file):
        file.write("Terms: ")
        first = True
        for terms in instance_stats.terms.items():
            if not first:
                file.write(", ")
            else:
                first = False
            file.write(terms[0] + ":" + str(terms[1]))
        file.write("\n")

    def __dump_instances(self, stats_file, title, instances):
        if len(instances) == 0:
            return
        sorted_instances = sorted(
            instances.items(), key=lambda item: netlist.get_model_name(item[0])
        )
        stats_file.write(title + " " + str(sum(j for i, j in sorted_instances)) + "\n")
        line_char = 0
        for instance in sorted_instances:
            model_name = netlist.get_model_name(instance[0])
            if line_char != 0:
                stats_file.write(",")
                line_char += 1
            if line_char > 80:
                stats_file.write("\n")
                line_char = 0
            elif line_char != 0:
                stats_file.write(" ")
                line_char += 1
            instance_char = model_name + ":" + str(instance[1])
            line_char += len(instance_char)
            stats_file.write(instance_char)
        stats_file.write("\n\n")

    def __dump_instance_stats_text(self, instance, file, dumped_models):
        if instance.is_primitive() or instance.is_blackbox():
            return
        # Only dump once each model
        model_id = instance.get_model_id()
        if model_id in dumped_models:
            return
        dumped_models.add(model_id)

        file.write("*** " + instance.get_name() + " ***\n")

        instance_stats = self.hier_instances.get(model_id)
        if instance_stats is None:
            print(
                "Cannot find " + str(instance.get_model_name()) + " in instance_stats"
            )
            raise

        if len(instance_stats.terms) > 0:
            self.__dump_terms_stats_text(instance_stats, file)

        # self.__dump_instances(stats_file, "Instances:", design_stats.ins)
        nb_primitives = sum(instance_stats.basic_primitives.values()) + sum(
            instance_stats.primitives.values()
        )
        if nb_primitives > 1:
            self.__dump_stats(file, "Primitives", nb_primitives)

        self.__dump_instances(
            file, "Simple Primitives:", instance_stats.basic_primitives
        )
        self.__dump_instances(file, "Other Primitives:", instance_stats.primitives)
        self.__dump_instances(file, "Blackboxes:", instance_stats.blackboxes)

        if instance_stats.assigns > 0:
            self.__dump_stats(file, "Assigns", instance_stats.assigns)

        self.__dump_instances(file, "Flat Instances:", instance_stats.flat_ins)
        self.__dump_instances(file, "Flat Blackboxes:", instance_stats.flat_blackboxes)

        nb_primitives = sum(instance_stats.flat_basic_primitives.values()) + sum(
            instance_stats.flat_primitives.values()
        )
        if nb_primitives > 1:
            self.__dump_stats(file, "Flat Primitives", nb_primitives)

        self.__dump_instances(
            file, "Flat Simple Primitives:", instance_stats.flat_basic_primitives
        )
        self.__dump_instances(
            file, "Flat Other Primitives:", instance_stats.flat_primitives
        )

        if instance_stats.flat_assigns > 0:
            self.__dump_stats(file, "Flat Assigns", instance_stats.flat_assigns)

        file.write("\n")

        for child_instance in instance.get_child_instances():
            self.__dump_instance_stats_text(child_instance, file, dumped_models)


class InstanceStats:
    def __init__(self):
        self.name = ""
        self.assigns = 0
        self.flat_assigns = 0
        self.basic_primitives = dict()
        self.primitives = dict()
        self.flat_basic_primitives = dict()
        self.flat_primitives = dict()
        self.blackboxes = dict()
        self.flat_blackboxes = dict()
        self.ins = dict()
        self.flat_ins = dict()
        self.terms = dict()
        self.bit_terms = dict()
        self.net_stats = dict()

    def add_ins_stats(self, ins_stats):
        self.flat_assigns += ins_stats.flat_assigns
        for ins, nb in ins_stats.flat_ins.items():
            self.flat_ins[ins] = self.flat_ins.get(ins, 0) + nb
        for ins, nb in ins_stats.flat_blackboxes.items():
            self.flat_blackboxes[ins] = self.flat_blackboxes.get(ins, 0) + nb
        for primitive, nb in ins_stats.flat_primitives.items():
            self.flat_primitives[primitive] = (
                self.flat_primitives.get(primitive, 0) + nb
            )
        for primitive, nb in ins_stats.flat_basic_primitives.items():
            self.flat_basic_primitives[primitive] = (
                self.flat_basic_primitives.get(primitive, 0) + nb
            )


def is_basic_primitive(instance):
    return instance.is_const() or instance.is_buf() or instance.is_inv()


def compute_instance_stats(instance, instances_stats):
    if instance.get_model_id() in instances_stats.hier_instances:
        return instances_stats.hier_instances.get(instance.get_model_id())
    instance_stats = InstanceStats()
    instance_stats.name = instance.get_model_name()
    for ins in instance.get_child_instances():
        model_id = ins.get_model_id()
        if ins.is_assign():
            instance_stats.assigns += 1
            instance_stats.flat_assigns += 1
        elif ins.is_primitive():
            if is_basic_primitive(ins):
                instance_stats.basic_primitives[model_id] = (
                    instance_stats.basic_primitives.get(model_id, 0) + 1
                )
                instance_stats.flat_basic_primitives[model_id] = (
                    instance_stats.flat_basic_primitives.get(model_id, 0) + 1
                )
            else:
                instance_stats.primitives[model_id] = (
                    instance_stats.primitives.get(model_id, 0) + 1
                )
                instance_stats.flat_primitives[model_id] = (
                    instance_stats.flat_primitives.get(model_id, 0) + 1
                )
        elif ins.is_blackbox():
            instance_stats.blackboxes[model_id] = (
                instance_stats.blackboxes.get(model_id, 0) + 1
            )
            instance_stats.flat_blackboxes[model_id] = (
                instance_stats.flat_blackboxes.get(model_id, 0) + 1
            )
            if model_id not in instance_stats.blackboxes:
                instance_stats.blackboxes[model_id] = dict()
                compute_instance_terms(model_id, instance_stats.blackboxes[model_id])
        else:
            if model_id in instances_stats.hier_instances:
                model_stats = instances_stats.hier_instances[model_id]
            else:
                model_stats = compute_instance_stats(ins, instances_stats)
            instance_stats.ins[model_id] = instance_stats.ins.get(model_id, 0) + 1
            instance_stats.flat_ins[model_id] = (
                instance_stats.flat_ins.get(model_id, 0) + 1
            )
            instance_stats.add_ins_stats(model_stats)
    compute_instance_terms(instance, instance_stats)
    compute_instance_net_stats(instance, instance_stats)
    instances_stats.hier_instances[instance.get_model_id()] = instance_stats
    return instance_stats


def compute_instance_terms(instance, instance_stats):
    for term in instance.get_terms():
        if term.get_direction() == netlist.Term.Direction.INPUT:
            instance_stats.terms["inputs"] = instance_stats.terms.get("inputs", 0) + 1
            bit_terms = sum(1 for _ in term.get_bits())
            instance_stats.bit_terms["inputs"] = (
                instance_stats.bit_terms.get("inputs", 0) + bit_terms
            )
        elif term.get_direction() == netlist.Term.Direction.OUTPUT:
            instance_stats.terms["outputs"] = instance_stats.terms.get("outputs", 0) + 1
            bit_terms = sum(1 for _ in term.get_bits())
            instance_stats.bit_terms["outputs"] = (
                instance_stats.bit_terms.get("outputs", 0) + bit_terms
            )
        elif term.get_direction() == netlist.Term.Direction.INOUT:
            instance_stats.terms["inouts"] = instance_stats.terms.get("inouts", 0) + 1
            bit_terms = sum(1 for _ in term.get_bits())
            instance_stats.bit_terms["inouts"] = (
                instance_stats.bit_terms.get("inouts", 0) + bit_terms
            )
        else:
            instance_stats.terms["unknowns"] = (
                instance_stats.terms.get("unknowns", 0) + 1
            )
            bit_terms = sum(1 for _ in term.get_bits())
            instance_stats.bit_terms["unknowns"] = (
                instance_stats.bit_terms.get("unknowns", 0) + bit_terms
            )


def compute_instance_net_stats(instance, instance_stats):
    for net in instance.get_bit_nets():
        if net.is_const():
            pass
        nb_components = sum(1 for c in net.get_terms())
        instance_stats.net_stats[nb_components] = (
            instance_stats.net_stats.get(nb_components, 0) + 1
        )
        instance_stats.net_stats = dict(sorted(instance_stats.net_stats.items()))


def dump_instances(stats_file, title, instances):
    if len(instances) == 0:
        return
    sorted_instances = sorted(
        instances.items(), key=lambda item: netlist.get_model_name(item[0])
    )
    stats_file.write(title + " " + str(sum(j for i, j in sorted_instances)) + "\n")
    line_char = 0
    for instance in sorted_instances:
        model_name = netlist.get_model_name(instance[0])
        if line_char != 0:
            stats_file.write(",")
            line_char += 1
        if line_char > 80:
            stats_file.write("\n")
            line_char = 0
        elif line_char != 0:
            stats_file.write(" ")
            line_char += 1
        instance_char = model_name + ":" + str(instance[1])
        line_char += len(instance_char)
        stats_file.write(instance_char)
    stats_file.write("\n\n")


def dump_blackboxes_stats(stats_file, design_stats):
    if len(design_stats.blackboxes) > 0:
        stats_file.write("*** BlackBoxes ***\n")
        for bbox in design_stats.blackboxes.items():
            design = bbox[0]
            design_terms = bbox[1]
            stats_file.write("*** " + design.getName() + " ***\n")
            if len(design_terms) > 0:
                stats_file.write("Terms: ")
                first = True
                for terms in design_terms.items():
                    if not first:
                        stats_file.write(", ")
                    else:
                        first = False
                    stats_file.write(terms[0] + ":" + str(terms[1]))
                stats_file.write("\n")
            stats_file.write("\n")


# def dump_pandas(designs_stats):
#  import pandas
#  import matplotlib.pyplot as plt
#
#  #create a figures directory erase the previous one
#  if os.path.exists('figures'):
#    import shutil
#    shutil.rmtree('figures')
#  os.makedirs('figures')
#
#  data = []
#  for design, design_stats in designs_stats.hier_designs.items():
#    data.append([
#      design.getName(),
#      sum(design_stats.terms.values()),
#      sum(design_stats.bit_terms.values()),
#      sum(design_stats.basic_primitives.values()),
#      sum(design_stats.primitives.values()),
#      sum(design_stats.blackboxes.values()),
#      sum(design_stats.ins.values()),
#      sum(design_stats.flat_ins.values()),
#      sum(design_stats.flat_blackboxes.values()),
#      sum(design_stats.flat_basic_primitives.values()),
#      sum(design_stats.flat_primitives.values())])
#  df = pandas.DataFrame(data, columns=[
#    'Design', 'Terms', 'Bit Terms',
#    'Basic Primitives', 'Primitives', 'Blackboxes', 'Instances',
#    'Flat Instances', 'Flat Blackboxes',
#    'Flat Basic Primitives', 'Flat Primitives'])
#  df.to_csv('figures/designs_stats.csv', index=False)
#
#  net_series = pandas.Series(design_stats.net_stats)
#  nets_plot = net_series.plot(kind='bar',
#      title='Number of nets with a given number of components for\n' + design.getName(),
#      xlabel='number of components', ylabel='number of nets')
#  nets_plot.set_yscale('log')
#  nets_plot.xaxis.set_major_locator(plt.MaxNLocator(100))
#  nets_figure = nets_plot.get_figure()
#  nets_figure.tight_layout()
#  nets_figure.savefig('figures/nets_' + design.getName() + '.png')
#
#  flat_primitives_series = pandas.Series(design_stats.flat_primitives)
#  primitives_plot = flat_primitives_series.plot(kind='bar',
#                  title='Number of  primitives for\n' + design.getName(),
#                  xlabel='primitive', ylabel='number of flat instances')
#  primitives_plot.set_yscale('log')
#  primitives_figure = primitives_plot.get_figure()
#  primitives_figure.tight_layout()
#  primitives_figure.savefig('figures/flat_primitives_' + design.getName() + '.png')


def convert_instance_stats_to_json(instance_stats):
    json_top = list()
    for _, value in instance_stats.hier_instances.items():
        nb_primitives = sum(value.basic_primitives.values())
        +sum(value.primitives.values())
        nb_terms = sum(value.terms.values())
        nb_nets = sum(value.net_stats.keys())
        nb_ins = sum(value.ins.values()) + nb_primitives
        json_top.append(
            {
                "Name": value.name,
                "primitives": nb_primitives,
                "instances": nb_ins,
                "terms": nb_terms,
                "nets": nb_nets,
                # "primitives": value.primitives,
                # "flat_basic_primitives": value.flat_basic_primitives,
                # "flat_primitives": value.flat_primitives,
                # "blackboxes": value.blackboxes,
                # "flat_blackboxes": value.flat_blackboxes,
                # "ins": value.ins,
                # "flat_ins": value.flat_ins,
                # "terms": value.terms,
                # "bit_terms": value.bit_terms,
                # "net_stats": value.net_stats,
            }
        )
    return json_top


def dump_instance_stats_json(instance, path: str):
    instances_stats = InstancesStats()
    compute_instance_stats(instance, instances_stats)
    json_dict = convert_instance_stats_to_json(instances_stats)
    with open(path, "w") as f:
        json.dump(json_dict, f, indent=4)


def dump_instance_stats_text(instance, path: str):
    instances_stats = InstancesStats()
    compute_instance_stats(instance, instances_stats)
    instances_stats.dump_instance_stats_text(instance, path)


def dump_constants(design, analyzed_models):
    if design.isPrimitive():
        return
    if design in analyzed_models:
        return
    analyzed_models.add(design)
    for bitnet in design.getBitNets():
        if bitnet.isConstant():
            message = f"In design {design.getName()}, \
                    constant net {bitnet.getName()} \
                    of type {bitnet.getTypeAsString()}"
            logger.info(message)
            if all(False for _ in bitnet.getComponents()):
                logger.info(" with zero connections\n")
            else:
                logger.info(" connected to:\n")
                for component in bitnet.getComponents():
                    logger.info(str(component) + "\n")

    for ins in design.getInstances():
        model = ins.getModel()
        dump_constants(model, analyzed_models)


def analyze_lowering_efficiency(instance, *, warn_threshold=256):
    """Audit reachable models for avoidable mux work, independent of frontend.

    Return JSON-compatible counts and source groups. Counts are estimates and
    categories overlap; they must not be added. ``warn_threshold`` is the
    expanded bit count per category/model; None disables logging warnings.
    Non-mux consumers are conservatively treated as live roots.
    """
    if warn_threshold is not None and (isinstance(warn_threshold, bool)
                                      or not isinstance(warn_threshold, int)
                                      or warn_threshold < 1):
        raise ValueError("warn_threshold must be a positive integer or None")
    from najaeda import naja

    top = instance._Instance__get_snl_model()
    models, order, visiting = {}, [], set()

    def visit(model):
        key = model.getNLID().toTuple()
        if key in visiting:
            raise ValueError("Recursive model hierarchy cannot be audited")
        if key in models:
            return
        visiting.add(key)
        for child in model.getInstances():
            if not child.getModel().isPrimitive():
                visit(child.getModel())
        visiting.remove(key)
        models[key] = model
        order.append(key)

    visit(top)
    occurrences = {key: 0 for key in order}
    occurrences[top.getNLID().toTuple()] = 1
    for key in reversed(order):
        for child in models[key].getInstances():
            ck = child.getModel().getNLID().toTuple()
            if ck in occurrences:
                occurrences[ck] += occurrences[key]

    def net_key(net):
        return net.getNLID().toTuple() if net is not None else None

    result = {"models": [], "mux_instances": 0, "mux_bits": 0,
              "expanded_mux_instances": 0, "expanded_mux_bits": 0}
    for key in reversed(order):
        model = models[key]
        muxes, roots = [], set()
        for term in model.getBitTerms():
            roots.add(net_key(term.getNet()))
        for child in model.getInstances():
            primitive = child.getModel()
            terms = {term.getName(): term for term in primitive.getTerms()}
            if primitive.isMux() and all(name in terms for name in ("A", "B", "S", "Y")):
                pins = {name: tuple(net_key(child.getInstTerm(bit).getNet())
                                    for bit in terms[name].getBits())
                        for name in ("A", "B", "S", "Y")}
                # Ignore incomplete/unrecognized mux signatures.
                if len(pins["A"]) == len(pins["B"]) == len(pins["Y"]) and len(pins["S"]) == 1:
                    muxes.append((child, pins))
                    continue
            for term in child.getInstTerms():
                if term.getDirection() != naja.SNLTerm.Direction.Output:
                    roots.add(net_key(term.getNet()))
        producers, consumers = {}, {}
        for index, (_, pins) in enumerate(muxes):
            for pin in ("A", "B", "S"):
                for position, input_net in enumerate(pins[pin]):
                    consumers.setdefault(input_net, set()).add((index, pin, position))
            for output in pins["Y"]:
                if output is not None:
                    producers.setdefault(output, []).append(index)
        live, pending = set(), list(roots)
        while pending:
            for index in producers.get(pending.pop(), ()):
                if index in live:
                    continue
                live.add(index)
                pins = muxes[index][1]
                pending.extend(pins["A"] + pins["B"] + pins["S"])
        by_output = {pins["Y"]: pins for _, pins in muxes}
        groups, seen = {}, set()
        for index, (child, pins) in enumerate(muxes):
            signature = (pins["A"], pins["B"], pins["S"])
            outputs = set(pins["Y"])
            repeated_arm = False
            for data_pin, common_pin in (("A", "B"), ("B", "A")):
                inner = by_output.get(pins[data_pin])
                if (inner is not None and inner is not pins
                        and inner[common_pin] == pins[common_pin]
                        and all(net not in roots and len(producers.get(net, ())) == 1
                                and consumers.get(net) == {(index, data_pin, position)}
                                for position, net in enumerate(inner["Y"]))
                        and all(bit is not None and bit not in outputs
                                for vector in (inner["A"], inner["B"], inner["S"], pins["S"])
                                for bit in vector)
                        and all(bit is not None for vector in signature for bit in vector)):
                    repeated_arm = True
                    break
            metrics = {"repeated_mux_arm": len(pins["Y"]) if repeated_arm else 0,
                       "identical_mux_inputs": sum(a is not None and a == b
                                                   for a, b in zip(pins["A"], pins["B"])),
                       "duplicate_mux": len(pins["Y"]) if signature in seen else 0,
                       "unconsumed_mux": len(pins["Y"]) if index not in live else 0}
            # Disconnected inputs cannot establish equivalent logic.
            if all(bit is not None for vector in signature for bit in vector):
                seen.add(signature)
            origins = child.getSourceOrigins()
            source = tuple(origins[0][0]) if origins else None
            for code, bits in metrics.items():
                if bits:
                    group = groups.setdefault((code, source), {"code": code, "source": source,
                                                              "instances": 0, "bits": 0})
                    group["instances"] += 1
                    group["bits"] += bits
        count = occurrences[key]
        row = {"model": model.getName(), "model_id": list(key), "occurrences": count,
               "mux_instances": len(muxes), "mux_bits": sum(len(p["Y"]) for _, p in muxes),
               "findings": list(groups.values())}
        totals = {}
        for finding in row["findings"]:
            finding["expanded_bits"] = finding["bits"] * count
            totals[finding["code"]] = totals.get(finding["code"], 0) + finding["expanded_bits"]
        if warn_threshold is not None:
            for code, bits in totals.items():
                if bits >= warn_threshold:
                    source = next((group["source"] for group in row["findings"]
                                   if group["code"] == code and group["source"]), None)
                    location = ":".join(map(str, source[:3])) if source else "source unavailable"
                    logger.warning("Lowering efficiency [%s]: model %s, %d expanded mux bits "
                                   "(%d hierarchy occurrences), %s; inspect report source groups",
                                   code, model.getName(), bits, count, location)
        result["models"].append(row)
        result["mux_instances"] += row["mux_instances"]
        result["mux_bits"] += row["mux_bits"]
        result["expanded_mux_instances"] += row["mux_instances"] * count
        result["expanded_mux_bits"] += row["mux_bits"] * count
    return result
