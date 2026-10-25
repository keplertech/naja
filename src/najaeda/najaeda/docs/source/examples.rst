najaeda code examples
=====================

Load a design from liberty and Verilog
--------------------------------------

Following snippet shows how to load primitive cells from a liberty file
and a netlist from a Verilog file.


.. code-block:: python

    benchmarks = path.join('..', '..', 'benchmarks')
    liberty_files = [
        'NangateOpenCellLibrary_typical.lib',
        'fakeram45_1024x32.lib',
        'fakeram45_64x32.lib'
    ]
    liberty_files = list(map(lambda p:path.join(benchmarks, 'liberty', p), liberty_files))
        
    netlist.load_liberty(liberty_files)
    top = netlist.load_verilog([path.join(benchmarks, 'verilog', 'tinyrocket.v')])
    
    top.dump_verilog('tinyrocket_naja.v')

Load a design with pre-existing libraries
-----------------------------------------

In FPGA design environments, Liberty files are often unavailable.

To address this, the following example demonstrates how to load primitives
without relying on Liberty files.

Naja EDA comes with pre-configured libraries to simplify this process.
Currently, it includes support for partial Xilinx primitives, but this can be
easily extended in the future. Don't hesitate to reach out if you need help.


.. code-block:: python

    netlist.load_primitives('xilinx')
    benchmarks = path.join('..', '..', 'benchmarks')
    top = netlist.load_verilog(path.join(benchmarks, 'verilog', 'vexriscv.v'))

Print all the instances in the netlist
--------------------------------------

Next example shows how to browse all the netlist and print all its content recursively.


.. code-block:: python

    def print_netlist(instance):
        for child_instance in instance.get_child_instances():
            logging.info(f"{child_instance}:{child_instance.get_model_name()}")
            print_netlist(child_instance)

Similar to the previous example, but utilizing an instance visitor.
This approach allows you to perform operations on each instance while
also defining conditions for stopping or continuing exploration.


.. code-block:: python

    def print_instance(instance):
        logging.info(f"{instance}:{instance.get_model_name()}")
    visitor_config = instance_visitor.VisitorConfig(callback=print_instance)
    instance_visitor.visit(top, visitor_config)

Counting the number of leaves in a netlist
------------------------------------------

The instance visitor provides a tool for collecting various types of information
about a netlist.

The following example demonstrates how to use the visitor’s callback
function to transmit user-defined arguments, allowing for flexible data processing.

This specific use case shows how to count the number of leaf instances in a netlist.


.. code-block:: python

    leaves = {"count": 0, "assigns": 0, "constants": 0}
    def count_leaves(instance, leaves):
        if instance.is_leaf():
            if instance.is_assign():
                leaves["assigns"] += 1
            elif instance.is_const():
                leaves["constants"] += 1
            else:
                leaves["count"] += 1
    visitor_config = instance_visitor.VisitorConfig(callback=count_leaves, args=(leaves,))
    instance_visitor.visit(top, visitor_config)
    logging.info(f"{top} leaves count")
    logging.info(f"nb_assigns={leaves['assigns']}")
    logging.info(f"nb constants={leaves['constants']}")
    logging.info(f"nb other leaves={leaves['count']}")

Design Statistics
-----------------

This example demonstrates how to use **najaeda** stats.
The code below generates a text report file, `design.stats`,
containing detailed statistics for each module in the design.


.. code-block:: python

    top = netlist.load_verilog(path.join(benchmarks, 'verilog', 'tinyrocket.v'))
    design_stats_file = open('design.stats', 'w')
    stats.dump_instance_stats_text(top, design_stats_file)

DLE (Dead Logic Elimination)
----------------------------

This example demonstrates how to perform Dead Logic Elimination (DLE) on a netlist.


.. code-block:: python

    def apply_dle(top, keep_attributes=None):
        # Trace back from design outputs
        visited = set()
        traced_terms = list(top.get_output_bit_terms())
        for leaf in top.get_leaf_children():
            attributes =  list(leaf.get_attributes())
            outputs = 0
            for term in leaf.get_output_bit_terms():
                outputs += 1
            if (outputs == 0):
                for term in leaf.get_input_bit_terms():
                        traced_terms.append(term)
                continue
            for attr in attributes:
                if attr in keep_attributes:
                    for term in leaf.get_input_bit_terms():
                        traced_terms.append(term)
                    break
        
        for termToTrace in traced_terms:
            queue = deque([termToTrace])
            while queue:
                term = queue.popleft()
                if term in visited:
                    continue
                visited.add(term)
                equipotential = term.get_equipotential()
                leaf_drivers = equipotential.get_leaf_drivers()
                for driver in leaf_drivers:
                    instance = driver.get_instance()
                    instances.add(instance)
                    input_terms = instance.get_input_bit_terms()
                    queue.extend(input_terms)
    
        to_delete = [leaf for leaf in top.get_leaf_children() if leaf not in instances]
        for leaf in to_delete:
            leaf.delete()
        return to_delete

Performance monitoring
----------------------

The repository's ``bench/python/test_workflows.py`` benchmarks leaf traversal,
cross-hierarchy connectivity, statistics, and the instance visitor on synthetic
hierarchies using ``pytest-codspeed``. Fixtures and correctness checks stay
outside the measured callables, and statistics starts with a fresh cache on
every call. Run these benchmarks against the native module built from the same
checkout, rather than an installed wheel. The `benchmark guide
<https://github.com/najaeda/naja/blob/main/bench/README.md>`_ describes package
staging, local execution, and the C++ frontend and serialization benchmarks.
