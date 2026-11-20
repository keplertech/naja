# SPDX-FileCopyrightText: 2024 The Naja authors
# <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

from os import path
import logging
import time

from najaeda import naja
from najaeda.native import stats

logging.basicConfig(level=logging.INFO)

benchmarks = path.join('..', '..', 'benchmarks')
liberty_files = ['NangateOpenCellLibrary_typical.lib', 'fakeram45_1024x32.lib', 'fakeram45_64x32.lib']
liberty_files = list(map(lambda p:path.join(benchmarks, 'liberty', p), liberty_files))

u = naja.NLUniverse.create()
db = naja.NLDB.create(u)
db.loadLibertyPrimitives(liberty_files)

files = [path.join(benchmarks, 'verilog', 'tinyrocket.v')]
start_time = time.time()
logging.info(f"Loading verilog: {', '.join(files)}")
db.loadGateLevelVerilog(files)
execution_time = time.time() - start_time
logging.info(f"Loading done in {execution_time:.2f} seconds")
top = db.getTopDesign()
    
design_stats_file = open('design.stats', 'w')
stats.compute_and_dump_design_stats(top, design_stats_file)
# snippet-end: design_stats
