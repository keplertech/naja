window.BENCHMARK_DATA = {
  "lastUpdate": 1792879014426,
  "repoUrl": "https://github.com/keplertech/naja",
  "entries": {
    "SNL Benchmarks": [
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "8aecabcee68458f2389838f86ab8d5d1af06a1d9",
          "message": "add smoke test in docker build test (#327)\n\n* add smoke test in docker build test\n\n* cleaning flows",
          "timestamp": "2026-01-23T17:52:21+01:00",
          "tree_id": "da4d8fbea04c0f5c5c6693a89c6ae7f2da6b321e",
          "url": "https://github.com/najaeda/naja/commit/8aecabcee68458f2389838f86ab8d5d1af06a1d9"
        },
        "date": 1769187363254,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 18546.2968472412,
            "unit": "ns/iter",
            "extra": "iterations: 37713\ncpu: 18546.015962665395 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 90277.19020691545,
            "unit": "ns/iter",
            "extra": "iterations: 7781\ncpu: 90250.97108340828 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1193964.589134132,
            "unit": "ns/iter",
            "extra": "iterations: 589\ncpu: 1193594.4906621391 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17309582.1951219,
            "unit": "ns/iter",
            "extra": "iterations: 41\ncpu: 17307790.878048774 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1451.168940734702,
            "unit": "ns/iter",
            "extra": "iterations: 482542\ncpu: 1450.6785440438348 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14243.095158699767,
            "unit": "ns/iter",
            "extra": "iterations: 49181\ncpu: 14239.870885097898 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 216093.3788819867,
            "unit": "ns/iter",
            "extra": "iterations: 3220\ncpu: 216062.3236024845 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 210346.4765931731,
            "unit": "ns/iter",
            "extra": "iterations: 3311\ncpu: 210334.5850196315 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 170110.60847333795,
            "unit": "ns/iter",
            "extra": "iterations: 4107\ncpu: 170105.3861699537 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 105136.45782231282,
            "unit": "ns/iter",
            "extra": "iterations: 6686\ncpu: 105131.31453784027 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 57161901.09999957,
            "unit": "ns/iter",
            "extra": "iterations: 10\ncpu: 57156435.79999999 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24031.684230398794,
            "unit": "ns/iter",
            "extra": "iterations: 29132\ncpu: 24030.978820540942 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 167321.93520280733,
            "unit": "ns/iter",
            "extra": "iterations: 4244\ncpu: 164484.28934968158 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "6384a8a7115c9b25cffbb22a00c11d6eecd10bda",
          "message": "xSet msb and set msb on busses (#328)\n\n* can you add associated unit test\n\n* implementation for BusNet\n\n* coverage\n\n* coverage\n\n* coverage\n\n* export to python\n\n* more testing\n\n* support until najaeda level\n\n* improve coverage\n\n* fix for empty busses (no bits)\n\n* boost cleaning\n\n* add early error for getting package on linux\n\n* try to be more resiliient\n\n* go back\n\n* new try\n\n* new attempt\n\n* new try\n\n* re-cleaning",
          "timestamp": "2026-01-27T08:17:22+01:00",
          "tree_id": "89555820944d98b395985809f1d81d68d13d48e4",
          "url": "https://github.com/najaeda/naja/commit/6384a8a7115c9b25cffbb22a00c11d6eecd10bda"
        },
        "date": 1769498480648,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 18491.483962915947,
            "unit": "ns/iter",
            "extra": "iterations: 37538\ncpu: 18490.81831743833 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 83702.666387557,
            "unit": "ns/iter",
            "extra": "iterations: 8360\ncpu: 83682.96447368423 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1142456.7843137619,
            "unit": "ns/iter",
            "extra": "iterations: 612\ncpu: 1142389.003267974 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17118129.82926947,
            "unit": "ns/iter",
            "extra": "iterations: 41\ncpu: 17115534.65853659 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1472.7542961847485,
            "unit": "ns/iter",
            "extra": "iterations: 476993\ncpu: 1472.3138515659557 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14397.532796837853,
            "unit": "ns/iter",
            "extra": "iterations: 48587\ncpu: 14396.64657212834 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 220353.8821506684,
            "unit": "ns/iter",
            "extra": "iterations: 3199\ncpu: 220331.6323851201 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 221163.26644294494,
            "unit": "ns/iter",
            "extra": "iterations: 2980\ncpu: 221156.04999999987 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 177121.5259559466,
            "unit": "ns/iter",
            "extra": "iterations: 3949\ncpu: 177120.73765510242 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 110114.8412823799,
            "unit": "ns/iter",
            "extra": "iterations: 6332\ncpu: 110112.65745420076 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 65304869.77778259,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 65300436.33333351 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 23936.37133694753,
            "unit": "ns/iter",
            "extra": "iterations: 29313\ncpu: 23935.20826936859 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 167182.65476903805,
            "unit": "ns/iter",
            "extra": "iterations: 4258\ncpu: 164296.76373884644 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "3df926407c3b26e2b6816dfbff69d61a5d93c6e1",
          "message": "B0.3.6 (#329)\n\n* switch to b0.3.6\n\n* more linux distribs tested with Docker\n\n* update\n\n* clean docker build\n\n* use latest\n\n* update version\n\n* switch back to rockylinux\n\n* switch back to rockylinux:9\n\n* setting for rockylinux compile\n\n* new attempt",
          "timestamp": "2026-01-31T00:41:08+01:00",
          "tree_id": "abc16c2aa4cb77609dc1b86856b70e9fb6908e6f",
          "url": "https://github.com/najaeda/naja/commit/3df926407c3b26e2b6816dfbff69d61a5d93c6e1"
        },
        "date": 1769816692126,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 18424.09182228578,
            "unit": "ns/iter",
            "extra": "iterations: 37431\ncpu: 18422.051588255727 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 85736.64259530546,
            "unit": "ns/iter",
            "extra": "iterations: 8184\ncpu: 85732.84506353861 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1167395.0450000349,
            "unit": "ns/iter",
            "extra": "iterations: 600\ncpu: 1167336.3466666664 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17384325.560976222,
            "unit": "ns/iter",
            "extra": "iterations: 41\ncpu: 17382894.70731707 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1502.4185540536457,
            "unit": "ns/iter",
            "extra": "iterations: 466518\ncpu: 1502.3794258742428 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14762.830048641683,
            "unit": "ns/iter",
            "extra": "iterations: 47696\ncpu: 14760.516521301573 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 206769.39368670987,
            "unit": "ns/iter",
            "extra": "iterations: 3358\ncpu: 206756.33740321617 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 220078.2773400634,
            "unit": "ns/iter",
            "extra": "iterations: 3173\ncpu: 220046.17239205795 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 174425.03028785173,
            "unit": "ns/iter",
            "extra": "iterations: 3995\ncpu: 174410.67934918642 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 108437.88214395924,
            "unit": "ns/iter",
            "extra": "iterations: 6474\ncpu: 108435.41581711457 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 65641815.22221841,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 65637838.33333324 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24079.536305557016,
            "unit": "ns/iter",
            "extra": "iterations: 29114\ncpu: 24077.840111286696 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 164025.01619843257,
            "unit": "ns/iter",
            "extra": "iterations: 4260\ncpu: 163626.48051644646 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "c89decf6faf85d152d38aa561994cab9111a3e29",
          "message": "integrating naja-verilog preprocessor (#330)",
          "timestamp": "2026-02-02T15:27:03+01:00",
          "tree_id": "7dcff09da26ed504aa752df5fc3caaa811ee3f37",
          "url": "https://github.com/najaeda/naja/commit/c89decf6faf85d152d38aa561994cab9111a3e29"
        },
        "date": 1770042662479,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 22382.665706932523,
            "unit": "ns/iter",
            "extra": "iterations: 32648\ncpu: 22375.00928081353 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 96033.43609880505,
            "unit": "ns/iter",
            "extra": "iterations: 7003\ncpu: 96028.08139368842 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1257657.827648079,
            "unit": "ns/iter",
            "extra": "iterations: 557\ncpu: 1257597.3231597845 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 19200362.24999914,
            "unit": "ns/iter",
            "extra": "iterations: 36\ncpu: 19200052.083333332 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1503.1751816805725,
            "unit": "ns/iter",
            "extra": "iterations: 464552\ncpu: 1502.927700235927 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14534.245013188252,
            "unit": "ns/iter",
            "extra": "iterations: 48528\ncpu: 14533.42711424332 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 218902.13520021347,
            "unit": "ns/iter",
            "extra": "iterations: 3321\ncpu: 218881.58084914193 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 222070.10508157726,
            "unit": "ns/iter",
            "extra": "iterations: 3188\ncpu: 222027.20075282306 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 175682.0604616152,
            "unit": "ns/iter",
            "extra": "iterations: 3986\ncpu: 175634.0429001505 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 109501.29390625563,
            "unit": "ns/iter",
            "extra": "iterations: 6400\ncpu: 109481.15625000004 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 65045299.22221991,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 65039870.66666659 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 23828.166898752363,
            "unit": "ns/iter",
            "extra": "iterations: 29443\ncpu: 23827.491322215763 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 165866.2923445585,
            "unit": "ns/iter",
            "extra": "iterations: 4180\ncpu: 165735.38660288742 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "43e5519d690220a0f015605038e7f33c7baf3a6d",
          "message": "Liberty zlib support (#331)\n\n* towards 0.4.0 adding zlib support in liberty parsing\n\n* more testing\n\n* install zlib on rockylinux\n\n* cleaning coverage\n\n* coverage\n\n* coverage",
          "timestamp": "2026-02-03T09:46:11+01:00",
          "tree_id": "a939b506831fd318dce800f3d585f61eb006a838",
          "url": "https://github.com/najaeda/naja/commit/43e5519d690220a0f015605038e7f33c7baf3a6d"
        },
        "date": 1770108617000,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 19537.62726213762,
            "unit": "ns/iter",
            "extra": "iterations: 35199\ncpu: 19536.188386033697 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 90626.48643059944,
            "unit": "ns/iter",
            "extra": "iterations: 7738\ncpu: 90615.70715947275 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1196769.7858407302,
            "unit": "ns/iter",
            "extra": "iterations: 565\ncpu: 1196666.391150442 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17861554.4864857,
            "unit": "ns/iter",
            "extra": "iterations: 37\ncpu: 17860576.75675675 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1581.0783709030968,
            "unit": "ns/iter",
            "extra": "iterations: 443264\ncpu: 1580.9866896477038 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 15549.163411327272,
            "unit": "ns/iter",
            "extra": "iterations: 45003\ncpu: 15547.192987134204 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 161077.6494916845,
            "unit": "ns/iter",
            "extra": "iterations: 4328\ncpu: 161055.23059149724 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 179916.49600413765,
            "unit": "ns/iter",
            "extra": "iterations: 3879\ncpu: 179910.33513792214 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 141661.10787879338,
            "unit": "ns/iter",
            "extra": "iterations: 4950\ncpu: 141654.0353535352 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 87996.67988848308,
            "unit": "ns/iter",
            "extra": "iterations: 7891\ncpu: 87991.41097452797 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 63518354.99999777,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 63516069.22222213 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 25740.833026509972,
            "unit": "ns/iter",
            "extra": "iterations: 27160\ncpu: 25739.4840942563 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 157061.5196942931,
            "unit": "ns/iter",
            "extra": "iterations: 4443\ncpu: 156990.89354043495 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "d3f19c7f423f6688988ec12eeacc9d6ba0c97a19",
          "message": "B0.4.1 (#332)\n\n* adding Perf in Python\n\n* improve perf report\n\n* more cleaning\n\n* code cleaning and more perf",
          "timestamp": "2026-02-04T10:40:47+01:00",
          "tree_id": "d9eb6d9e775f3ff90f43177f5a20b280ee27bdde",
          "url": "https://github.com/najaeda/naja/commit/d3f19c7f423f6688988ec12eeacc9d6ba0c97a19"
        },
        "date": 1770198281121,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 19097.19460882723,
            "unit": "ns/iter",
            "extra": "iterations: 36838\ncpu: 19096.808350073294 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 86407.86739103428,
            "unit": "ns/iter",
            "extra": "iterations: 8099\ncpu: 86406.08704778367 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1177060.376884449,
            "unit": "ns/iter",
            "extra": "iterations: 597\ncpu: 1176717.8257956447 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17348367.400001053,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17348065.125 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1468.6198822804597,
            "unit": "ns/iter",
            "extra": "iterations: 476726\ncpu: 1468.302534369847 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14362.663774359018,
            "unit": "ns/iter",
            "extra": "iterations: 48750\ncpu: 14362.231405128203 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 201089.29062405825,
            "unit": "ns/iter",
            "extra": "iterations: 3317\ncpu: 201068.1872173649 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 219279.27737683602,
            "unit": "ns/iter",
            "extra": "iterations: 3187\ncpu: 219257.46062127396 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 173504.27689243734,
            "unit": "ns/iter",
            "extra": "iterations: 4016\ncpu: 173496.00572709175 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 107779.1088864932,
            "unit": "ns/iter",
            "extra": "iterations: 6493\ncpu: 107777.83382103793 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 64445178.6666688,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 64437441.44444451 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24187.52037424257,
            "unit": "ns/iter",
            "extra": "iterations: 29179\ncpu: 24187.160594948393 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 164079.6770354295,
            "unit": "ns/iter",
            "extra": "iterations: 4276\ncpu: 163948.77712814428 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "a0e7d5abf641c36a5113c70ed84d8a7970bf184b",
          "message": "switch to macos15 (#334)",
          "timestamp": "2026-02-16T10:31:33+01:00",
          "tree_id": "89699ac1c589f8aed31825993fec1f5469fdf299",
          "url": "https://github.com/najaeda/naja/commit/a0e7d5abf641c36a5113c70ed84d8a7970bf184b"
        },
        "date": 1771234525739,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 18515.019379540485,
            "unit": "ns/iter",
            "extra": "iterations: 38133\ncpu: 18514.30031731047 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 85364.48004380497,
            "unit": "ns/iter",
            "extra": "iterations: 8218\ncpu: 85338.76758335362 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1159542.8580858875,
            "unit": "ns/iter",
            "extra": "iterations: 606\ncpu: 1159113.646864686 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17405130.400000248,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17403848.624999996 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1487.8220124697664,
            "unit": "ns/iter",
            "extra": "iterations: 467853\ncpu: 1487.6972702964385 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14738.835010609571,
            "unit": "ns/iter",
            "extra": "iterations: 47597\ncpu: 14738.185179738226 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 214037.9201228998,
            "unit": "ns/iter",
            "extra": "iterations: 3255\ncpu: 214020.0298003072 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 220366.59440778432,
            "unit": "ns/iter",
            "extra": "iterations: 3183\ncpu: 220356.5249764373 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 173384.61336633505,
            "unit": "ns/iter",
            "extra": "iterations: 4040\ncpu: 173369.7873762378 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 108667.09035957481,
            "unit": "ns/iter",
            "extra": "iterations: 6452\ncpu: 108664.05207687533 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 64194998.444444686,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 64190902.11111111 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24117.0175486983,
            "unit": "ns/iter",
            "extra": "iterations: 29005\ncpu: 24115.921186002386 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 164370.1606847129,
            "unit": "ns/iter",
            "extra": "iterations: 4263\ncpu: 164198.25991084496 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "b35fdd957f286f8344ce678d803c8879ff451fbb",
          "message": "enable assign compression (#335)\n\n* enable assign compression\n\n* new tests\n\n* fix and coverage\n\n* local coverage and testing\n\n* more coverage\n\n* fix license and coverage\n\n* cleaning and coverage",
          "timestamp": "2026-02-18T13:45:26+01:00",
          "tree_id": "f2805ee397832ceec9a84825a81c11cb931ec83f",
          "url": "https://github.com/najaeda/naja/commit/b35fdd957f286f8344ce678d803c8879ff451fbb"
        },
        "date": 1771418971301,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 18593.474217884923,
            "unit": "ns/iter",
            "extra": "iterations: 36983\ncpu: 18593.179893464567 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 84811.07379627212,
            "unit": "ns/iter",
            "extra": "iterations: 8266\ncpu: 84808.20868618437 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1161422.9884298202,
            "unit": "ns/iter",
            "extra": "iterations: 605\ncpu: 1161346.575206611 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17742333.299999304,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17740953.049999997 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1472.0380684110173,
            "unit": "ns/iter",
            "extra": "iterations: 476064\ncpu: 1471.9952086274106 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14395.496461176166,
            "unit": "ns/iter",
            "extra": "iterations: 48745\ncpu: 14394.792963380854 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 201246.49953662435,
            "unit": "ns/iter",
            "extra": "iterations: 3237\ncpu: 200959.78220574607 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 221000.92552846478,
            "unit": "ns/iter",
            "extra": "iterations: 3075\ncpu: 220981.66666666666 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 174807.81608619308,
            "unit": "ns/iter",
            "extra": "iterations: 3991\ncpu: 174803.5968428966 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 109176.65595256288,
            "unit": "ns/iter",
            "extra": "iterations: 6409\ncpu: 109171.373693244 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 65649534.222220354,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 65647092.666666664 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 23926.444742106738,
            "unit": "ns/iter",
            "extra": "iterations: 29489\ncpu: 23925.141917325123 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 164141.1350193136,
            "unit": "ns/iter",
            "extra": "iterations: 4229\ncpu: 163981.7569165468 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "899fc098cc513859e355789b05893c54a58ee153",
          "message": "better reporting and implicit AND support (#336)",
          "timestamp": "2026-02-19T23:33:26+01:00",
          "tree_id": "1b6b4df1d63e0db955f5d1db116ebc5c8a3e8c7e",
          "url": "https://github.com/najaeda/naja/commit/899fc098cc513859e355789b05893c54a58ee153"
        },
        "date": 1771540639086,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 20035.05308617691,
            "unit": "ns/iter",
            "extra": "iterations: 35659\ncpu: 20033.608598109873 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 93839.5617540532,
            "unit": "ns/iter",
            "extra": "iterations: 7457\ncpu: 93808.19350945418 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1141802.7829313318,
            "unit": "ns/iter",
            "extra": "iterations: 539\ncpu: 1141774.8812615955 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17749361.12820441,
            "unit": "ns/iter",
            "extra": "iterations: 39\ncpu: 17748064.102564104 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1582.7481232086343,
            "unit": "ns/iter",
            "extra": "iterations: 443443\ncpu: 1582.7015828415372 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 15548.415957328576,
            "unit": "ns/iter",
            "extra": "iterations: 44995\ncpu: 15543.606645182806 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 163675.89484483982,
            "unit": "ns/iter",
            "extra": "iterations: 3899\ncpu: 163657.66709412658 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 179411.79350881002,
            "unit": "ns/iter",
            "extra": "iterations: 3913\ncpu: 179357.52900587796 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 141394.23410696033,
            "unit": "ns/iter",
            "extra": "iterations: 4955\ncpu: 141353.27951564087 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 88809.90794451057,
            "unit": "ns/iter",
            "extra": "iterations: 7930\ncpu: 88797.50264817147 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 64540182.2222248,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 64537502.999999985 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 25717.328801261596,
            "unit": "ns/iter",
            "extra": "iterations: 27287\ncpu: 25715.202074247816 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 158022.35637160216,
            "unit": "ns/iter",
            "extra": "iterations: 4442\ncpu: 157910.16434039795 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "8ae43f5ce018d863660064ea99759c8a246b565b",
          "message": "Slang testing (#333)\n\n* init sv support\n\n* more testing\n\n* bump slang version\n\n* some progress\n\n* Add FA (Full Adder) primitive to NLDB0\n\n- Ports: A, B, CI (inputs), S (sum), CO (carry-out)\n- Combinatorial arcs from all inputs to both outputs\n- getFASumTruthTable() -> SNLTruthTable(3, 0x96) [XOR-3]\n- getFACoutTruthTable() -> SNLTruthTable(3, 0xE8) [majority]\n- getPrimitiveTruthTable throws for FA (two outputs, use dedicated methods)\n- Unit tests in NLDB0Test and SNLGateTruthTablesTest\n\n* add source info and option for json post elaboration generation\n\n* Use FA\n\n* refactor sv test start action with simulation\n\n* bump slang version\n\n* add system verilog round testing through najaeda\n\n* add license\n\n* use external build for slang\n\n* macos-14 build\n\n* new attempt\n\n* new attempt\n\n* new version\n\n* revert changes on macos build\n\n* cleaning compile\n\n* -fPIC on slang build\n\n* removing macos-14 soon to be discontinued on github\n\n* new attempt\n\n* switch to najaeda fork and add PIC option in slang\n\n* update slang\n\n* new attempt\n\n* new attempt\n\n* update compile\n\n* add boost unordered to windows setup\n\n* new version\n\n* new attempt\n\n* restrain to windows\n\n* FMT clash\n\n* cleaning binary ops tests\n\n* more testing\n\n* error reporting and testing\n\n* cleaning gates support\n\n* NLDB0 cleaning\n\n* test wrong file when loading SV\n\n* coverage\n\n* more testing\n\n* more testing\n\n* coverage\n\n* improve testing and coverage\n\n* logging cleaning\n\n* coverage\n\n* more coverage\n\n* more testing\n\n* mux2 cover\n\n* bump slang version\n\n* license and coverage\n\n* verilator support",
          "timestamp": "2026-02-22T03:08:23+01:00",
          "tree_id": "505f65bf0bac3226f66007b2a185cde3a28b62aa",
          "url": "https://github.com/najaeda/naja/commit/8ae43f5ce018d863660064ea99759c8a246b565b"
        },
        "date": 1771726332313,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 29617.92011566306,
            "unit": "ns/iter",
            "extra": "iterations: 23171\ncpu: 29611.499417375166 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 97138.22367515446,
            "unit": "ns/iter",
            "extra": "iterations: 7265\ncpu: 97087.94439091533 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1194050.8406779773,
            "unit": "ns/iter",
            "extra": "iterations: 590\ncpu: 1193769.2898305086 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17656752.736842774,
            "unit": "ns/iter",
            "extra": "iterations: 38\ncpu: 17655370.342105262 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1456.5288526349516,
            "unit": "ns/iter",
            "extra": "iterations: 480684\ncpu: 1456.2430806933457 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14248.469869458191,
            "unit": "ns/iter",
            "extra": "iterations: 49103\ncpu: 14247.808158360984 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 216242.2778497464,
            "unit": "ns/iter",
            "extra": "iterations: 3088\ncpu: 216221.41418393762 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 240742.9858395256,
            "unit": "ns/iter",
            "extra": "iterations: 2966\ncpu: 240263.05461901543 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 189511.29261597272,
            "unit": "ns/iter",
            "extra": "iterations: 3643\ncpu: 189506.7452648918 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 123161.31846235291,
            "unit": "ns/iter",
            "extra": "iterations: 5671\ncpu: 123156.314582966 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 64980475.44444161,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 64977042.66666678 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24669.47512715116,
            "unit": "ns/iter",
            "extra": "iterations: 28706\ncpu: 24668.841775238612 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 180716.75468378956,
            "unit": "ns/iter",
            "extra": "iterations: 3897\ncpu: 180511.5563253975 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "a5a75781b287649f65b3046b93137f9b60147fd0",
          "message": "supporting unquoted enable (#338)",
          "timestamp": "2026-02-25T12:46:33+01:00",
          "tree_id": "590e167d756d66180bfb5dccea56ca630bb00e0a",
          "url": "https://github.com/najaeda/naja/commit/a5a75781b287649f65b3046b93137f9b60147fd0"
        },
        "date": 1772020220679,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 29695.365662192795,
            "unit": "ns/iter",
            "extra": "iterations: 23694\ncpu: 29686.573225289103 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 97585.09275201506,
            "unit": "ns/iter",
            "extra": "iterations: 7202\ncpu: 97557.70410996948 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1182086.777777809,
            "unit": "ns/iter",
            "extra": "iterations: 594\ncpu: 1182043.0538720544 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 16845690.634146728,
            "unit": "ns/iter",
            "extra": "iterations: 41\ncpu: 16845457.68292683 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1469.8569612986082,
            "unit": "ns/iter",
            "extra": "iterations: 480758\ncpu: 1469.8083880039437 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14273.71281811316,
            "unit": "ns/iter",
            "extra": "iterations: 49157\ncpu: 14271.412738775747 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 202451.03521537434,
            "unit": "ns/iter",
            "extra": "iterations: 3436\ncpu: 202081.6094295694 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 239322.64376067658,
            "unit": "ns/iter",
            "extra": "iterations: 2925\ncpu: 239290.83487179512 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 191912.30953041618,
            "unit": "ns/iter",
            "extra": "iterations: 3599\ncpu: 191900.8560711308 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 126140.06021272934,
            "unit": "ns/iter",
            "extra": "iterations: 5547\ncpu: 126132.56751397137 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 64324226.22222272,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 64320422.66666668 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24234.204430424437,
            "unit": "ns/iter",
            "extra": "iterations: 28846\ncpu: 24232.977189211688 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 178906.85725127807,
            "unit": "ns/iter",
            "extra": "iterations: 3930\ncpu: 178835.83460559955 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "2a5ef178dcb6fc015f5c17d35c87817e7a9abab3",
          "message": "Sv integration (#337)\n\n* new options better diagnose\n\n* more support\n\n* support packed arrays\n\n* new operator\n\n* add shifter support\n\n* adding dffn/dffr\n\n* new feature\n\n* more support\n\n* support +\n\n* more support\n\n* more support\n\n* bump slang and add perf in SV loading\n\n* adding coverage\n\n* adding in naja_edit\n\n* coverage\n\n* coverage\n\n* refactor code\n\n* coverage\n\n* more coverage\n\n* more coverage\n\n* coverage\n\n* coverage\n\n* cover\n\n* missing license\n\n* more coverage\n\n* coverage\n\n* more coverage\n\n* coverage\n\n* coverage",
          "timestamp": "2026-02-27T15:12:02+01:00",
          "tree_id": "439bce19fee3e0d2d1f0244df86627af391f0cd5",
          "url": "https://github.com/najaeda/naja/commit/2a5ef178dcb6fc015f5c17d35c87817e7a9abab3"
        },
        "date": 1772201761761,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 33716.915417550066,
            "unit": "ns/iter",
            "extra": "iterations: 20548\ncpu: 33714.598306404514 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 102020.96043377697,
            "unit": "ns/iter",
            "extra": "iterations: 6824\ncpu: 102013.85697538102 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1216171.4473225814,
            "unit": "ns/iter",
            "extra": "iterations: 579\ncpu: 1216165.164075993 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17493149.65853755,
            "unit": "ns/iter",
            "extra": "iterations: 41\ncpu: 17476866.682926834 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1465.7514932667325,
            "unit": "ns/iter",
            "extra": "iterations: 478816\ncpu: 1465.6716776381745 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14618.01425247287,
            "unit": "ns/iter",
            "extra": "iterations: 48132\ncpu: 14615.309565361918 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 226571.81494212046,
            "unit": "ns/iter",
            "extra": "iterations: 3199\ncpu: 226572.30415754905 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 242927.40561131292,
            "unit": "ns/iter",
            "extra": "iterations: 2887\ncpu: 242892.3134741946 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 198284.24781759086,
            "unit": "ns/iter",
            "extra": "iterations: 3551\ncpu: 198247.32131793836 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 131329.09996253782,
            "unit": "ns/iter",
            "extra": "iterations: 5342\ncpu: 131325.7023586673 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 64201362.444439135,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 64199810.00000005 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24956.067589135262,
            "unit": "ns/iter",
            "extra": "iterations: 28185\ncpu: 24955.693312045434 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 184586.78272492843,
            "unit": "ns/iter",
            "extra": "iterations: 3797\ncpu: 184413.33737155198 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "e854ad98634b4caa35aa36552de1e38ee17f5f6a",
          "message": "Liberty: bus direction coming from child pins (#339)\n\n* add test and support\n\n* new testing",
          "timestamp": "2026-03-04T14:50:20+01:00",
          "tree_id": "dabb346e4feea145071bbce56356661f8cf98587",
          "url": "https://github.com/najaeda/naja/commit/e854ad98634b4caa35aa36552de1e38ee17f5f6a"
        },
        "date": 1772632460345,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 33128.91581108801,
            "unit": "ns/iter",
            "extra": "iterations: 20454\ncpu: 33128.401290701084 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 104182.29715223095,
            "unit": "ns/iter",
            "extra": "iterations: 6707\ncpu: 104182.12121663935 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1238482.2353982423,
            "unit": "ns/iter",
            "extra": "iterations: 565\ncpu: 1238464.8371681415 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17702619.52499936,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17701818.94999999 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1463.1731292672755,
            "unit": "ns/iter",
            "extra": "iterations: 478342\ncpu: 1463.1851081443815 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14353.310974110693,
            "unit": "ns/iter",
            "extra": "iterations: 48824\ncpu: 14352.305177781427 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 213430.29827914605,
            "unit": "ns/iter",
            "extra": "iterations: 3138\ncpu: 213424.3467176547 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 240134.63602057987,
            "unit": "ns/iter",
            "extra": "iterations: 2915\ncpu: 240111.22504288162 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 194155.90596394203,
            "unit": "ns/iter",
            "extra": "iterations: 3605\ncpu: 194127.23495145648 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 127218.21649109249,
            "unit": "ns/iter",
            "extra": "iterations: 5506\ncpu: 127213.55793679625 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 65353155.88889211,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 65350555.55555541 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 25069.068234455986,
            "unit": "ns/iter",
            "extra": "iterations: 28065\ncpu: 25067.314626759293 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 186720.6383652408,
            "unit": "ns/iter",
            "extra": "iterations: 3769\ncpu: 186195.49854073676 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "20f660677b572158bf3d3f8019ce5e356e3b94b7",
          "message": "B0.5.1 (#340)",
          "timestamp": "2026-03-18T02:19:44+01:00",
          "tree_id": "e315345a6e1169989d0b15b842f5a8acf1220aa1",
          "url": "https://github.com/najaeda/naja/commit/20f660677b572158bf3d3f8019ce5e356e3b94b7"
        },
        "date": 1773797020287,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 47011.23020527768,
            "unit": "ns/iter",
            "extra": "iterations: 15004\ncpu: 47010.232071447615 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 111556.99012738948,
            "unit": "ns/iter",
            "extra": "iterations: 6280\ncpu: 111555.81289808918 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1380619.618577103,
            "unit": "ns/iter",
            "extra": "iterations: 506\ncpu: 1380596.458498024 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17109668.048780207,
            "unit": "ns/iter",
            "extra": "iterations: 41\ncpu: 17109090.46341463 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1478.5915502172597,
            "unit": "ns/iter",
            "extra": "iterations: 473811\ncpu: 1478.5346351182227 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14484.685114385198,
            "unit": "ns/iter",
            "extra": "iterations: 48389\ncpu: 14484.014734753759 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 197441.7341772117,
            "unit": "ns/iter",
            "extra": "iterations: 3555\ncpu: 197436.72573839672 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 259742.28518380917,
            "unit": "ns/iter",
            "extra": "iterations: 2693\ncpu: 259736.1318232455 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 214770.29199999938,
            "unit": "ns/iter",
            "extra": "iterations: 3250\ncpu: 214764.97138461535 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 149462.37033107126,
            "unit": "ns/iter",
            "extra": "iterations: 4712\ncpu: 149461.12818336152 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 66328702.00000223,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 66326279.888888866 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24522.334716506615,
            "unit": "ns/iter",
            "extra": "iterations: 28678\ncpu: 24522.336111304878 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 202676.45444182435,
            "unit": "ns/iter",
            "extra": "iterations: 3468\ncpu: 202537.42070361238 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "a7792f81f69fe0c3486c30a7c45d18b45b146d5d",
          "message": "rework naja-regress (#341)\n\n* rework naja-regress\n\n* code cleaning and NajaEdit cleaning",
          "timestamp": "2026-03-18T19:56:08+01:00",
          "tree_id": "b3786ace26ec3d2579afef2ce5b43c30ba03f6cd",
          "url": "https://github.com/najaeda/naja/commit/a7792f81f69fe0c3486c30a7c45d18b45b146d5d"
        },
        "date": 1773860419672,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 42035.89472745626,
            "unit": "ns/iter",
            "extra": "iterations: 16823\ncpu: 42035.67265053796 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 109645.42013942942,
            "unit": "ns/iter",
            "extra": "iterations: 6455\ncpu: 109644.66893880712 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1194932.0856164242,
            "unit": "ns/iter",
            "extra": "iterations: 584\ncpu: 1194670.7619863017 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 15596391.400000207,
            "unit": "ns/iter",
            "extra": "iterations: 45\ncpu: 15593960.822222227 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1223.9466178200112,
            "unit": "ns/iter",
            "extra": "iterations: 568036\ncpu: 1223.8970839876342 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12565.248338736694,
            "unit": "ns/iter",
            "extra": "iterations: 55831\ncpu: 12562.851587827561 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 130610.87149704975,
            "unit": "ns/iter",
            "extra": "iterations: 5424\ncpu: 130607.35232300874 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 213446.53665241797,
            "unit": "ns/iter",
            "extra": "iterations: 3274\ncpu: 213442.31734880898 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 173352.19552238702,
            "unit": "ns/iter",
            "extra": "iterations: 4020\ncpu: 173352.19701492533 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 112396.62826889448,
            "unit": "ns/iter",
            "extra": "iterations: 6233\ncpu: 112390.64784213077 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 61877357.555557765,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 61873181.333333224 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24367.81547660756,
            "unit": "ns/iter",
            "extra": "iterations: 28598\ncpu: 24366.37464158337 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 131792.53279303218,
            "unit": "ns/iter",
            "extra": "iterations: 5306\ncpu: 131755.20523936985 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "f2aa8ad570e32ebdd74eff9d4e0991e59c0e25d2",
          "message": "new options in dump verilog to dump or not RTL informations (#342)\n\n* new options in dump verilog to dump or not RTL informations\n\n* add TT for not\n\n* new attempt",
          "timestamp": "2026-03-19T23:27:43+01:00",
          "tree_id": "023e03fc938fe4bf91e1e55edba3e1ca89f224ca",
          "url": "https://github.com/najaeda/naja/commit/f2aa8ad570e32ebdd74eff9d4e0991e59c0e25d2"
        },
        "date": 1773959503020,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 45086.47846233981,
            "unit": "ns/iter",
            "extra": "iterations: 15322\ncpu: 45075.02617151809 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 111023.18269841195,
            "unit": "ns/iter",
            "extra": "iterations: 6300\ncpu: 111017.81555555557 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1372692.18590999,
            "unit": "ns/iter",
            "extra": "iterations: 511\ncpu: 1372699.3189823874 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 16859460.333333366,
            "unit": "ns/iter",
            "extra": "iterations: 42\ncpu: 16856231.64285714 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1468.2057840924945,
            "unit": "ns/iter",
            "extra": "iterations: 475926\ncpu: 1467.9701403159306 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14391.386186050277,
            "unit": "ns/iter",
            "extra": "iterations: 48632\ncpu: 14388.161580852111 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 195689.1290420993,
            "unit": "ns/iter",
            "extra": "iterations: 3278\ncpu: 195681.0018303846 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 260730.32692306678,
            "unit": "ns/iter",
            "extra": "iterations: 2704\ncpu: 260725.3820266274 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 212157.24012158453,
            "unit": "ns/iter",
            "extra": "iterations: 3290\ncpu: 212158.05866261417 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 146678.7676810141,
            "unit": "ns/iter",
            "extra": "iterations: 4765\ncpu: 146679.40692549845 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 66167727.44444512,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 66168176.333333254 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 25001.9850346458,
            "unit": "ns/iter",
            "extra": "iterations: 27998\ncpu: 25001.77534109579 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 199957.7595885323,
            "unit": "ns/iter",
            "extra": "iterations: 3494\ncpu: 199842.57441327267 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "19c944a2abc9303c7c8d07cfc976ec4536af51d4",
          "message": "codecov run cleaning (#343)\n\n* codecov run cleaning\n\n* update naja-verilog\n\n* update slang and add multiple combinations for compiling on ubuntu\n\n* new testing\n\n* update coverage\n\n* update coverage\n\n* switch to gcovr\n\n* cleaning\n\n* cleaning coverage\n\n* new attempt\n\n* new setting\n\n* update config file",
          "timestamp": "2026-03-20T22:55:10+01:00",
          "tree_id": "61ad335a9001f3ea60b84726d431f769aff1fa0e",
          "url": "https://github.com/najaeda/naja/commit/19c944a2abc9303c7c8d07cfc976ec4536af51d4"
        },
        "date": 1774043957102,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 44583.68034357775,
            "unit": "ns/iter",
            "extra": "iterations: 15135\ncpu: 44582.18566237199 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 110734.06931476707,
            "unit": "ns/iter",
            "extra": "iterations: 6319\ncpu: 110727.66719417632 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1368632.5957031897,
            "unit": "ns/iter",
            "extra": "iterations: 512\ncpu: 1368519.6093750005 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 16866902.43902221,
            "unit": "ns/iter",
            "extra": "iterations: 41\ncpu: 16864972.780487806 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1499.7689109399935,
            "unit": "ns/iter",
            "extra": "iterations: 466595\ncpu: 1499.700843343799 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14705.079072980752,
            "unit": "ns/iter",
            "extra": "iterations: 47766\ncpu: 14704.538877025512 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 210032.77391038724,
            "unit": "ns/iter",
            "extra": "iterations: 3304\ncpu: 210025.41918886188 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 258335.18620942437,
            "unit": "ns/iter",
            "extra": "iterations: 2712\ncpu: 258331.06637168172 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 212264.46940642333,
            "unit": "ns/iter",
            "extra": "iterations: 3285\ncpu: 212256.68371385074 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 147723.91621052427,
            "unit": "ns/iter",
            "extra": "iterations: 4750\ncpu: 147720.8160000002 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 65804122.44444586,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 65803576.11111115 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24901.400064029913,
            "unit": "ns/iter",
            "extra": "iterations: 28113\ncpu: 24901.106569914256 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 200449.344748138,
            "unit": "ns/iter",
            "extra": "iterations: 3504\ncpu: 200298.0479451981 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "committer": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "distinct": true,
          "id": "800693539f4283942f0c968c0fbf84220475dfe1",
          "message": "install gcovr with python",
          "timestamp": "2026-03-20T23:46:45+01:00",
          "tree_id": "d386ca34a08a02390dda6cfa9ffc8ed56ff72051",
          "url": "https://github.com/najaeda/naja/commit/800693539f4283942f0c968c0fbf84220475dfe1"
        },
        "date": 1774047063081,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 45587.29832373064,
            "unit": "ns/iter",
            "extra": "iterations: 15272\ncpu: 45577.32968831849 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 110145.69055476386,
            "unit": "ns/iter",
            "extra": "iterations: 6363\ncpu: 110114.67122426529 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1389604.2848723114,
            "unit": "ns/iter",
            "extra": "iterations: 509\ncpu: 1389541.4597249515 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 16975446.58536738,
            "unit": "ns/iter",
            "extra": "iterations: 41\ncpu: 16974871.121951208 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1500.072711736044,
            "unit": "ns/iter",
            "extra": "iterations: 466926\ncpu: 1500.0475107404588 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14687.258514550567,
            "unit": "ns/iter",
            "extra": "iterations: 47595\ncpu: 14686.780901355181 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 211505.46427592429,
            "unit": "ns/iter",
            "extra": "iterations: 3653\ncpu: 211501.9170544758 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 254186.7971543268,
            "unit": "ns/iter",
            "extra": "iterations: 2741\ncpu: 254166.8139365193 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 205851.1311378794,
            "unit": "ns/iter",
            "extra": "iterations: 3401\ncpu: 205846.30667450753 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 140725.37928270333,
            "unit": "ns/iter",
            "extra": "iterations: 4991\ncpu: 140718.62111801255 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 64248681.111103505,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 64246348.444444366 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 28999.85624419352,
            "unit": "ns/iter",
            "extra": "iterations: 27978\ncpu: 28997.89252269641 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 199697.03077670682,
            "unit": "ns/iter",
            "extra": "iterations: 3509\ncpu: 199537.16699911805 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "committer": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "distinct": true,
          "id": "3698bfe3c599ce9bbea43555e63aada4faa1a9c2",
          "message": "cleaning codecov",
          "timestamp": "2026-03-21T00:18:22+01:00",
          "tree_id": "7f2ab41fe4ec480e7d84651f8db9f8f0e6a87678",
          "url": "https://github.com/najaeda/naja/commit/3698bfe3c599ce9bbea43555e63aada4faa1a9c2"
        },
        "date": 1774049022879,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 45532.62520918227,
            "unit": "ns/iter",
            "extra": "iterations: 14939\ncpu: 45530.71095789544 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 111605.43148502665,
            "unit": "ns/iter",
            "extra": "iterations: 6276\ncpu: 111598.44534735504 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1389998.4356434701,
            "unit": "ns/iter",
            "extra": "iterations: 505\ncpu: 1389932.544554455 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17412551.29268239,
            "unit": "ns/iter",
            "extra": "iterations: 41\ncpu: 17410486.902439017 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1520.4231368148544,
            "unit": "ns/iter",
            "extra": "iterations: 455309\ncpu: 1520.336727365372 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 15006.046021237156,
            "unit": "ns/iter",
            "extra": "iterations: 46522\ncpu: 15005.855294269393 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 215190.9780117546,
            "unit": "ns/iter",
            "extra": "iterations: 3229\ncpu: 215183.70672034682 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 259934.25565863226,
            "unit": "ns/iter",
            "extra": "iterations: 2695\ncpu: 259924.7833024117 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 215319.4201550409,
            "unit": "ns/iter",
            "extra": "iterations: 3225\ncpu: 215309.0226356589 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 148302.43888769497,
            "unit": "ns/iter",
            "extra": "iterations: 4639\ncpu: 148298.0993748652 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 68860626.55555254,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 68854462.0000001 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 25076.5538119813,
            "unit": "ns/iter",
            "extra": "iterations: 27912\ncpu: 25075.02246345655 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 201541.9734945776,
            "unit": "ns/iter",
            "extra": "iterations: 3471\ncpu: 201392.82886774727 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "committer": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "distinct": true,
          "id": "c4a6dcba1181705bfaa77db67dbb5868de83d5c8",
          "message": "coverage cleaning",
          "timestamp": "2026-03-21T00:39:45+01:00",
          "tree_id": "e27c7f34044ca06bd46141a56f9d7c06657ceccc",
          "url": "https://github.com/najaeda/naja/commit/c4a6dcba1181705bfaa77db67dbb5868de83d5c8"
        },
        "date": 1774050929139,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 45868.69555174392,
            "unit": "ns/iter",
            "extra": "iterations: 15152\ncpu: 45867.74346620908 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 112891.80495654982,
            "unit": "ns/iter",
            "extra": "iterations: 6214\ncpu: 112889.97682652078 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1386265.5821781973,
            "unit": "ns/iter",
            "extra": "iterations: 505\ncpu: 1386257.8277227723 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17158176.536584992,
            "unit": "ns/iter",
            "extra": "iterations: 41\ncpu: 17145730.85365854 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1500.56106993854,
            "unit": "ns/iter",
            "extra": "iterations: 466195\ncpu: 1500.537279464601 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14716.404788384298,
            "unit": "ns/iter",
            "extra": "iterations: 47657\ncpu: 14715.951885347391 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 191617.12020810263,
            "unit": "ns/iter",
            "extra": "iterations: 3652\ncpu: 191605.16100766708 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 255703.71794872987,
            "unit": "ns/iter",
            "extra": "iterations: 2730\ncpu: 255665.98095238107 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 208793.27784396973,
            "unit": "ns/iter",
            "extra": "iterations: 3358\ncpu: 208781.41751042297 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 141076.1746513085,
            "unit": "ns/iter",
            "extra": "iterations: 4947\ncpu: 141071.2862340815 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 64794996.999997534,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 64792479.77777764 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 25137.746455794153,
            "unit": "ns/iter",
            "extra": "iterations: 28074\ncpu: 25136.652026786363 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 202400.7610267179,
            "unit": "ns/iter",
            "extra": "iterations: 3469\ncpu: 202292.85471323095 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "d529aacf231568235d88843e76688b851ebaa66b",
          "message": "new exclusion pattern (#344)",
          "timestamp": "2026-03-21T15:32:35+01:00",
          "tree_id": "5b678e9c2ca8a4140e9dcb1c31bb31cc8d359329",
          "url": "https://github.com/najaeda/naja/commit/d529aacf231568235d88843e76688b851ebaa66b"
        },
        "date": 1774104429273,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 44787.404220983306,
            "unit": "ns/iter",
            "extra": "iterations: 15494\ncpu: 44782.42138892475 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 111047.25603558657,
            "unit": "ns/iter",
            "extra": "iterations: 6296\ncpu: 111036.36308767472 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1380311.0389863914,
            "unit": "ns/iter",
            "extra": "iterations: 513\ncpu: 1380156.3859649121 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17563360.90243888,
            "unit": "ns/iter",
            "extra": "iterations: 41\ncpu: 17562635.95121951 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1502.1184481490002,
            "unit": "ns/iter",
            "extra": "iterations: 465689\ncpu: 1502.0517448339995 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14701.756726056665,
            "unit": "ns/iter",
            "extra": "iterations: 47539\ncpu: 14700.537537600714 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 196122.0342172718,
            "unit": "ns/iter",
            "extra": "iterations: 3507\ncpu: 196110.6395779869 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 259332.44903986677,
            "unit": "ns/iter",
            "extra": "iterations: 2708\ncpu: 259300.58197932053 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 214531.6373928954,
            "unit": "ns/iter",
            "extra": "iterations: 3268\ncpu: 214517.41554467572 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 148340.7327130421,
            "unit": "ns/iter",
            "extra": "iterations: 4729\ncpu: 148307.8012264748 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 65942314.11111196,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 65938498.777777754 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 25014.67253470666,
            "unit": "ns/iter",
            "extra": "iterations: 27948\ncpu: 25012.171854873297 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 203018.83583350392,
            "unit": "ns/iter",
            "extra": "iterations: 3466\ncpu: 202782.63906521592 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "0d8fe466ac566208680f5f5d1692a3e346ead691",
          "message": "add blackbox detection in SV constructor (#346)\n\n* add blackbox detection in SV constructor\n\n* clean coverage",
          "timestamp": "2026-03-23T13:20:50+01:00",
          "tree_id": "54db2c9e9add7d991d062f3371b5a0b4fbe8d3d4",
          "url": "https://github.com/najaeda/naja/commit/0d8fe466ac566208680f5f5d1692a3e346ead691"
        },
        "date": 1774268702261,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 45790.252927401474,
            "unit": "ns/iter",
            "extra": "iterations: 15372\ncpu: 45788.19607077804 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 111538.33343948874,
            "unit": "ns/iter",
            "extra": "iterations: 6280\ncpu: 111526.52261146496 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1384445.0670611542,
            "unit": "ns/iter",
            "extra": "iterations: 507\ncpu: 1384325.4654832345 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 16893214.73170727,
            "unit": "ns/iter",
            "extra": "iterations: 41\ncpu: 16892544.34146342 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1501.4966825438034,
            "unit": "ns/iter",
            "extra": "iterations: 465869\ncpu: 1501.478924332806 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14674.485384163789,
            "unit": "ns/iter",
            "extra": "iterations: 47688\ncpu: 14673.414443885264 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 214230.68874723554,
            "unit": "ns/iter",
            "extra": "iterations: 3608\ncpu: 214226.51773835922 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 261973.19042232854,
            "unit": "ns/iter",
            "extra": "iterations: 2652\ncpu: 261954.2209653091 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 214518.8690184062,
            "unit": "ns/iter",
            "extra": "iterations: 3260\ncpu: 214502.80276073635 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 147512.1657551508,
            "unit": "ns/iter",
            "extra": "iterations: 4754\ncpu: 147508.0925536389 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 66342689.22222318,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 66342361.00000004 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 25203.976826268146,
            "unit": "ns/iter",
            "extra": "iterations: 28049\ncpu: 25203.592534493222 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 203367.86316355743,
            "unit": "ns/iter",
            "extra": "iterations: 3464\ncpu: 203257.74480368927 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "24e307110d1e4cad5b9eaea86105c772560aab8a",
          "message": "Rtl memory inferring (#345)\n\n* First steps\n\n* naja_mem in progress\n\n* bused mux2\n\n* clean coverage\n\n* perf coverage\n\n* save current state\n\n* cleaning and coverage\n\n* coverage\n\n* coverage\n\n* coverage\n\n* coverage\n\n* clean coverage\n\n* NLDB0 optims and SV Constructor coverage\n\n* code cleaning\n\n* coverage\n\n* cleaning and coverage\n\n* cleaning\n\n* coverage\n\n* cleaning and coverage",
          "timestamp": "2026-03-30T18:26:24+02:00",
          "tree_id": "51371b56505f4e88f5a0aa26a1cababfb4f9282c",
          "url": "https://github.com/najaeda/naja/commit/24e307110d1e4cad5b9eaea86105c772560aab8a"
        },
        "date": 1774888218582,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 36322.87213750092,
            "unit": "ns/iter",
            "extra": "iterations: 19607\ncpu: 36316.99535880042 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 104369.02310674099,
            "unit": "ns/iter",
            "extra": "iterations: 6708\ncpu: 104369.25193798449 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1190567.7741936056,
            "unit": "ns/iter",
            "extra": "iterations: 589\ncpu: 1190550.4550084886 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 16092817.51162799,
            "unit": "ns/iter",
            "extra": "iterations: 43\ncpu: 16092845.255813956 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1222.0546768178767,
            "unit": "ns/iter",
            "extra": "iterations: 574119\ncpu: 1221.8165606781865 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12602.754064784727,
            "unit": "ns/iter",
            "extra": "iterations: 55661\ncpu: 12602.66790032518 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 129368.35363128356,
            "unit": "ns/iter",
            "extra": "iterations: 5370\ncpu: 129367.99664804473 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 217836.75116640108,
            "unit": "ns/iter",
            "extra": "iterations: 3215\ncpu: 217836.70451010886 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 176596.57836198705,
            "unit": "ns/iter",
            "extra": "iterations: 3956\ncpu: 176595.71941354877 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 115992.87566225213,
            "unit": "ns/iter",
            "extra": "iterations: 6040\ncpu: 115985.15993377482 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 62022773.19999894,
            "unit": "ns/iter",
            "extra": "iterations: 10\ncpu: 62020734.500000075 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24242.143674370225,
            "unit": "ns/iter",
            "extra": "iterations: 27270\ncpu: 24242.002603593734 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 126900.63530430071,
            "unit": "ns/iter",
            "extra": "iterations: 5495\ncpu: 126662.8687898404 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "7b13cb0aeff525802fd22e30af6f00a5db18c964",
          "message": "towards v0.5.3 (#347)\n\n* towards v0.5.3\n\n* fix typo",
          "timestamp": "2026-03-31T15:46:56+02:00",
          "tree_id": "997bf35c560e676044c183483efc7f0f43ea9143",
          "url": "https://github.com/najaeda/naja/commit/7b13cb0aeff525802fd22e30af6f00a5db18c964"
        },
        "date": 1774965049494,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 40037.202550007656,
            "unit": "ns/iter",
            "extra": "iterations: 16549\ncpu: 40033.55018430117 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 105488.10514372183,
            "unit": "ns/iter",
            "extra": "iterations: 6610\ncpu: 105466.93933434189 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1218926.085763254,
            "unit": "ns/iter",
            "extra": "iterations: 583\ncpu: 1218808.6981132072 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 16764001.404762246,
            "unit": "ns/iter",
            "extra": "iterations: 42\ncpu: 16763639.571428569 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1460.9071504735753,
            "unit": "ns/iter",
            "extra": "iterations: 479227\ncpu: 1460.83908252248 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14372.228050356924,
            "unit": "ns/iter",
            "extra": "iterations: 48691\ncpu: 14371.376024316614 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 210896.08686502976,
            "unit": "ns/iter",
            "extra": "iterations: 3327\ncpu: 210890.98707544323 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 256916.15522938356,
            "unit": "ns/iter",
            "extra": "iterations: 2725\ncpu: 256893.2873394495 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 212463.25553870236,
            "unit": "ns/iter",
            "extra": "iterations: 3295\ncpu: 212446.2321699542 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 144349.192339894,
            "unit": "ns/iter",
            "extra": "iterations: 4778\ncpu: 144339.00439514418 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 64392694.88888992,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 64391494.222222194 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24217.05179351843,
            "unit": "ns/iter",
            "extra": "iterations: 28826\ncpu: 24216.003920072144 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 194916.91394955898,
            "unit": "ns/iter",
            "extra": "iterations: 3591\ncpu: 194758.53689778486 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "c74285ebf18152a4aaa320472533d0330452bfc0",
          "message": "B0.5.4 (#348)\n\n* Fix Perf on macos\n\n* Fix bloating of memory gen\n\n* code coverage exclusion",
          "timestamp": "2026-04-01T20:38:54+02:00",
          "tree_id": "611fb8f8289acde572e40ca9823efc3051f69635",
          "url": "https://github.com/najaeda/naja/commit/c74285ebf18152a4aaa320472533d0330452bfc0"
        },
        "date": 1775068999986,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 38755.89421515604,
            "unit": "ns/iter",
            "extra": "iterations: 17857\ncpu: 38748.82040656325 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 104897.89580341581,
            "unit": "ns/iter",
            "extra": "iterations: 6267\ncpu: 104895.99664911442 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1207937.3510272575,
            "unit": "ns/iter",
            "extra": "iterations: 584\ncpu: 1207834.8116438356 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18067087.00000096,
            "unit": "ns/iter",
            "extra": "iterations: 39\ncpu: 18062063.743589748 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1461.4701464489362,
            "unit": "ns/iter",
            "extra": "iterations: 478938\ncpu: 1460.9278466106261 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14451.243155070455,
            "unit": "ns/iter",
            "extra": "iterations: 48430\ncpu: 14449.440119760467 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 208869.44707272583,
            "unit": "ns/iter",
            "extra": "iterations: 3382\ncpu: 208852.98196333533 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 257730.09793816667,
            "unit": "ns/iter",
            "extra": "iterations: 2716\ncpu: 257657.30081001492 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 211385.5128282632,
            "unit": "ns/iter",
            "extra": "iterations: 3313\ncpu: 211361.311500151 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 143372.12454062395,
            "unit": "ns/iter",
            "extra": "iterations: 4898\ncpu: 143355.3983258475 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 65536782.1111036,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 65535100.66666668 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24461.21185545173,
            "unit": "ns/iter",
            "extra": "iterations: 28392\ncpu: 24458.732142857112 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 196663.227347664,
            "unit": "ns/iter",
            "extra": "iterations: 3554\ncpu: 196523.61142375128 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "committer": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "distinct": true,
          "id": "348eb18cb7dd4410ccdb483bdec35b98f70fea22",
          "message": "switching to 0.5.4",
          "timestamp": "2026-04-02T18:23:33+02:00",
          "tree_id": "3950721ba3fa858fd760c1ce0141da3eea95742f",
          "url": "https://github.com/najaeda/naja/commit/348eb18cb7dd4410ccdb483bdec35b98f70fea22"
        },
        "date": 1775147258424,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 40247.19890299647,
            "unit": "ns/iter",
            "extra": "iterations: 17320\ncpu: 40246.05848729792 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 106637.47166362408,
            "unit": "ns/iter",
            "extra": "iterations: 6564\ncpu: 106636.28915295552 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1215708.2055268008,
            "unit": "ns/iter",
            "extra": "iterations: 579\ncpu: 1215695.2590673578 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 16745568.476188682,
            "unit": "ns/iter",
            "extra": "iterations: 42\ncpu: 16745405.309523815 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1478.0924994729971,
            "unit": "ns/iter",
            "extra": "iterations: 474500\ncpu: 1478.0932623814542 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14832.517647798688,
            "unit": "ns/iter",
            "extra": "iterations: 47683\ncpu: 14832.201539332671 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 200722.91344490295,
            "unit": "ns/iter",
            "extra": "iterations: 3466\ncpu: 200717.41431044438 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 254671.4227967837,
            "unit": "ns/iter",
            "extra": "iterations: 2746\ncpu: 254665.4067734887 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 209664.57342450527,
            "unit": "ns/iter",
            "extra": "iterations: 3364\ncpu: 209654.99227110576 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 144328.2519021188,
            "unit": "ns/iter",
            "extra": "iterations: 4863\ncpu: 144320.2687641373 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 64388394.33332709,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 64386375.888888925 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24466.045689357517,
            "unit": "ns/iter",
            "extra": "iterations: 28650\ncpu: 24465.17815008723 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 194675.98718288424,
            "unit": "ns/iter",
            "extra": "iterations: 3589\ncpu: 194569.18612428056 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "noam.chn1@gmail.com",
            "name": "Noam Cohen",
            "username": "nanocoh"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "5136b9f699b7cb3caedefe6678a19603f9204e35",
          "message": "work towards full RTL support + fix in tt",
          "timestamp": "2026-04-07T01:25:37+02:00",
          "tree_id": "5d1b72bb79599ceb48606b217b0b923ca9b8d5ee",
          "url": "https://github.com/najaeda/naja/commit/5136b9f699b7cb3caedefe6678a19603f9204e35"
        },
        "date": 1775518189467,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 43198.07769393366,
            "unit": "ns/iter",
            "extra": "iterations: 16964\ncpu: 43117.81684744164 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 107746.74016355618,
            "unit": "ns/iter",
            "extra": "iterations: 6481\ncpu: 107744.13655300111 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1220449.9825784916,
            "unit": "ns/iter",
            "extra": "iterations: 574\ncpu: 1220409.0087108011 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17098834.926830534,
            "unit": "ns/iter",
            "extra": "iterations: 41\ncpu: 17098818.000000007 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1453.7031523116957,
            "unit": "ns/iter",
            "extra": "iterations: 481139\ncpu: 1453.6253078632162 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14260.98192219108,
            "unit": "ns/iter",
            "extra": "iterations: 49121\ncpu: 14260.650902872494 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 195732.60267859357,
            "unit": "ns/iter",
            "extra": "iterations: 3584\ncpu: 195726.82979910713 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 261190.6969810201,
            "unit": "ns/iter",
            "extra": "iterations: 2683\ncpu: 261172.6243011556 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 214152.62854528063,
            "unit": "ns/iter",
            "extra": "iterations: 3279\ncpu: 214140.62275083835 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 147012.7036493063,
            "unit": "ns/iter",
            "extra": "iterations: 4768\ncpu: 147005.10528523487 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 65311943.88888808,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 65311143.00000004 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24719.305014121266,
            "unit": "ns/iter",
            "extra": "iterations: 29038\ncpu: 24718.614539568847 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 203924.4918959616,
            "unit": "ns/iter",
            "extra": "iterations: 3456\ncpu: 203365.58478010356 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "319865a0f291ad7ad501fd8e77767700ae5168c1",
          "message": "B0.5.5 (#350)\n\n* coverage process cleaning\n\n* switch back to lcov\n\n* add expected failing test\n\n* extend always support in SV\n\n* coverage settings\n\n* update coverage",
          "timestamp": "2026-04-07T16:46:42+02:00",
          "tree_id": "f23f7e94a3390ac12a39f9f39caa496451c06288",
          "url": "https://github.com/najaeda/naja/commit/319865a0f291ad7ad501fd8e77767700ae5168c1"
        },
        "date": 1775573475905,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 41597.39720630353,
            "unit": "ns/iter",
            "extra": "iterations: 16752\ncpu: 41597.725465616044 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 108071.52694332272,
            "unit": "ns/iter",
            "extra": "iterations: 6458\ncpu: 108016.80984825021 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1227854.4624783588,
            "unit": "ns/iter",
            "extra": "iterations: 573\ncpu: 1227531.8603839446 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 19080234.00000201,
            "unit": "ns/iter",
            "extra": "iterations: 37\ncpu: 19073199.62162162 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1451.92949587462,
            "unit": "ns/iter",
            "extra": "iterations: 482142\ncpu: 1451.7891285140063 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14291.79660499306,
            "unit": "ns/iter",
            "extra": "iterations: 49072\ncpu: 14291.426291979118 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 213106.66860998038,
            "unit": "ns/iter",
            "extra": "iterations: 3259\ncpu: 213102.90702669523 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 257074.11378385185,
            "unit": "ns/iter",
            "extra": "iterations: 2619\ncpu: 257049.41771668577 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 210888.30207393607,
            "unit": "ns/iter",
            "extra": "iterations: 3327\ncpu: 210866.43552750233 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 144785.8052620822,
            "unit": "ns/iter",
            "extra": "iterations: 4827\ncpu: 144786.91278226636 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 65341307.111111745,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 65336704.11111123 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24020.85549152656,
            "unit": "ns/iter",
            "extra": "iterations: 29154\ncpu: 24020.68755573852 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 198590.25912089972,
            "unit": "ns/iter",
            "extra": "iterations: 3535\ncpu: 198504.37114567877 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "f348ecc80d49fb655dcdbf0048223f5e246cd94a",
          "message": "Adding Bundle Term in SNL (#351)",
          "timestamp": "2026-04-11T23:49:51+02:00",
          "tree_id": "8eadd722fe2d23435e5d7c6f01b8cdf0df61bcd9",
          "url": "https://github.com/najaeda/naja/commit/f348ecc80d49fb655dcdbf0048223f5e246cd94a"
        },
        "date": 1775944453389,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 64996.426398684394,
            "unit": "ns/iter",
            "extra": "iterations: 11046\ncpu: 64989.44713018288 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 164933.14155252115,
            "unit": "ns/iter",
            "extra": "iterations: 4161\ncpu: 164926.72098053352 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1641747.8470590569,
            "unit": "ns/iter",
            "extra": "iterations: 425\ncpu: 1641654.8776470588 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17581747.575002283,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17581592 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1486.1296427258658,
            "unit": "ns/iter",
            "extra": "iterations: 470871\ncpu: 1486.084095219285 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14574.9662813882,
            "unit": "ns/iter",
            "extra": "iterations: 48104\ncpu: 14574.726280558782 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 212957.62029644963,
            "unit": "ns/iter",
            "extra": "iterations: 3508\ncpu: 212950.90165336395 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 286096.14221680927,
            "unit": "ns/iter",
            "extra": "iterations: 2454\ncpu: 286093.63406682963 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 235181.66028874475,
            "unit": "ns/iter",
            "extra": "iterations: 2979\ncpu: 235177.60792212136 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 165003.76375559688,
            "unit": "ns/iter",
            "extra": "iterations: 4271\ncpu: 164995.41676422357 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 72219894.87499058,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 72214325.25000004 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 23981.09325573553,
            "unit": "ns/iter",
            "extra": "iterations: 29210\ncpu: 23978.862444368333 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 222792.5047753115,
            "unit": "ns/iter",
            "extra": "iterations: 3142\ncpu: 222702.2883513775 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "763b85e2d9cb7f726be465db7dddef4d3f38a6d9",
          "message": "work on common usage for najaeda and naja_edit level for primitives (#352)\n\n* work on common usage for najaeda and naja_edit level for primitives\n\n* clean coverage\n\n* cleaning najaeda build and remove xilinx.py from primitives installation\n\n* clean SNLPyLoader\n\n* cleaning files\n\n* add rule in python exclude coverage\n\n* coverage cleaning",
          "timestamp": "2026-04-14T14:02:44+02:00",
          "tree_id": "a8cdd49df8114d2f1a6267928051545e81cc1bc9",
          "url": "https://github.com/najaeda/naja/commit/763b85e2d9cb7f726be465db7dddef4d3f38a6d9"
        },
        "date": 1776168415340,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 62236.70442930054,
            "unit": "ns/iter",
            "extra": "iterations: 11740\ncpu: 62237.41132879046 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 155640.78810068732,
            "unit": "ns/iter",
            "extra": "iterations: 4370\ncpu: 155611.81739130436 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1513303.3844492007,
            "unit": "ns/iter",
            "extra": "iterations: 463\ncpu: 1513111.9416846645 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18290730.025640666,
            "unit": "ns/iter",
            "extra": "iterations: 39\ncpu: 18288285.33333333 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1228.8254266720628,
            "unit": "ns/iter",
            "extra": "iterations: 570110\ncpu: 1228.534891512164 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12660.938754474711,
            "unit": "ns/iter",
            "extra": "iterations: 55302\ncpu: 12660.411431774619 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 137300.61786371964,
            "unit": "ns/iter",
            "extra": "iterations: 5430\ncpu: 137296.28342541435 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 247924.53455571647,
            "unit": "ns/iter",
            "extra": "iterations: 2836\ncpu: 247913.0744005641 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 204392.87842563615,
            "unit": "ns/iter",
            "extra": "iterations: 3430\ncpu: 204388.2903790086 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 136604.9978649068,
            "unit": "ns/iter",
            "extra": "iterations: 5152\ncpu: 136606.42701863346 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 73869684.5555498,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 73867951.88888879 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24726.298417268194,
            "unit": "ns/iter",
            "extra": "iterations: 27800\ncpu: 24726.002589928077 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 153483.11488579746,
            "unit": "ns/iter",
            "extra": "iterations: 4561\ncpu: 153428.16684934928 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "committer": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "distinct": true,
          "id": "3075cd639bb0db400d16815bd1ea895b01a747d1",
          "message": "refactor in order to remove repeating nl directories",
          "timestamp": "2026-04-15T12:06:40+02:00",
          "tree_id": "57ee4c1fcaa9cce5ba6c3d38169e019275facd24",
          "url": "https://github.com/najaeda/naja/commit/3075cd639bb0db400d16815bd1ea895b01a747d1"
        },
        "date": 1776247848307,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 59545.17317854893,
            "unit": "ns/iter",
            "extra": "iterations: 11543\ncpu: 59541.197175777525 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 160501.54770397602,
            "unit": "ns/iter",
            "extra": "iterations: 4486\ncpu: 160490.90347748552 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1513121.7840172108,
            "unit": "ns/iter",
            "extra": "iterations: 463\ncpu: 1512783.0518358527 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17446474.099998,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17444461.049999993 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1223.968511889785,
            "unit": "ns/iter",
            "extra": "iterations: 571295\ncpu: 1223.8781557689108 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12555.50000906254,
            "unit": "ns/iter",
            "extra": "iterations: 55169\ncpu: 12554.139607388208 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 130764.95830176071,
            "unit": "ns/iter",
            "extra": "iterations: 5276\ncpu: 130757.11144806672 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 251113.60480113598,
            "unit": "ns/iter",
            "extra": "iterations: 2791\ncpu: 251094.67610175576 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 209363.69670658803,
            "unit": "ns/iter",
            "extra": "iterations: 3340\ncpu: 209348.48712574854 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 141611.1891837058,
            "unit": "ns/iter",
            "extra": "iterations: 4937\ncpu: 141602.47822564305 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 72016325.11110903,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 72011463.8888888 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24724.000634229775,
            "unit": "ns/iter",
            "extra": "iterations: 28381\ncpu: 24721.92442126773 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 154554.833041978,
            "unit": "ns/iter",
            "extra": "iterations: 4558\ncpu: 154458.77446247733 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "9c1cf1da9c1e8fb4d37ebb8f06b61ef9f1f85bb3",
          "message": "Liberty bundle no pin (#353)\n\n* liberty-bundle-no-pin\n\n* adding failing test\n\n* bypass \"internal\" bundle pins in liberty parser",
          "timestamp": "2026-04-16T10:14:57+02:00",
          "tree_id": "da7db4c97febe290437d1003809b7b7ce1fbe6d2",
          "url": "https://github.com/najaeda/naja/commit/9c1cf1da9c1e8fb4d37ebb8f06b61ef9f1f85bb3"
        },
        "date": 1776327542293,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 64217.97211413795,
            "unit": "ns/iter",
            "extra": "iterations: 10794\ncpu: 64212.77682045582 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 174025.32170640194,
            "unit": "ns/iter",
            "extra": "iterations: 3985\ncpu: 174013.3982434128 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1591131.5221445437,
            "unit": "ns/iter",
            "extra": "iterations: 429\ncpu: 1591021.1025641027 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17797056.76923034,
            "unit": "ns/iter",
            "extra": "iterations: 39\ncpu: 17796286.1025641 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1599.6937348092274,
            "unit": "ns/iter",
            "extra": "iterations: 437353\ncpu: 1599.5076105571477 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 15743.76636102153,
            "unit": "ns/iter",
            "extra": "iterations: 44496\ncpu: 15742.190017080178 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 169502.77742717927,
            "unit": "ns/iter",
            "extra": "iterations: 4120\ncpu: 169479.56480582518 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 243851.40689176464,
            "unit": "ns/iter",
            "extra": "iterations: 2873\ncpu: 243817.00974591012 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 197037.9777715362,
            "unit": "ns/iter",
            "extra": "iterations: 3554\ncpu: 197006.0785030952 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 135187.00038646976,
            "unit": "ns/iter",
            "extra": "iterations: 5175\ncpu: 135175.24173913043 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 74053597.62500297,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 74043340.49999984 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 25214.661140940763,
            "unit": "ns/iter",
            "extra": "iterations: 27749\ncpu: 25213.203646978243 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 214989.53702566633,
            "unit": "ns/iter",
            "extra": "iterations: 3268\ncpu: 214869.8271113782 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "cb3aa27eb920d33d41bed9d8e8694b6833da6d19",
          "message": "Sv updates (#354)\n\n* report more unsupported SV elements\n\n* more SV support\n\n* negedge flops\n\n* support '|'\n\n* more sv support\n\n* more SV support\n\n* new SV supports : latch added in NLDB0\n\n* SV parsing\n\n* more SV support\n\n* add coverage scripts, switch to 0.6.0, more coverage\n\n* improve coverage\n\n* cleaning\n\n* coverage\n\n* more testing\n\n* coverage\n\n* coverage\n\n* bump slang version\n\n* exclude from cov\n\n* improve coverage\n\n* coverage\n\n* more coverage\n\n* more coverage\n\n* more coverage\n\n* cleaning",
          "timestamp": "2026-04-23T01:02:53+02:00",
          "tree_id": "2ce7a66f43c578ad163936e7a5755fda67be2663",
          "url": "https://github.com/najaeda/naja/commit/cb3aa27eb920d33d41bed9d8e8694b6833da6d19"
        },
        "date": 1776899224477,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 63294.089237671564,
            "unit": "ns/iter",
            "extra": "iterations: 11150\ncpu: 63285.53192825112 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 171594.13522537658,
            "unit": "ns/iter",
            "extra": "iterations: 4193\ncpu: 171553.25232530406 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1654805.4573459863,
            "unit": "ns/iter",
            "extra": "iterations: 422\ncpu: 1654494.5426540284 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17818277.435897775,
            "unit": "ns/iter",
            "extra": "iterations: 39\ncpu: 17816546.871794872 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1491.4078913394399,
            "unit": "ns/iter",
            "extra": "iterations: 468615\ncpu: 1491.3160312836771 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14703.762041561507,
            "unit": "ns/iter",
            "extra": "iterations: 47689\ncpu: 14683.048669504504 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 191340.00858614605,
            "unit": "ns/iter",
            "extra": "iterations: 3494\ncpu: 191329.477676016 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 285509.9946960403,
            "unit": "ns/iter",
            "extra": "iterations: 2451\ncpu: 285489.7474500204 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 238432.10094851328,
            "unit": "ns/iter",
            "extra": "iterations: 2952\ncpu: 238424.24559620576 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 162550.05815569652,
            "unit": "ns/iter",
            "extra": "iterations: 4316\ncpu: 162545.08572752532 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 73078455.75000016,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 73072785.49999996 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24129.475016289907,
            "unit": "ns/iter",
            "extra": "iterations: 29159\ncpu: 24128.84107822631 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 224600.24567084917,
            "unit": "ns/iter",
            "extra": "iterations: 3118\ncpu: 224513.430083381 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "162f008f1dadc054864254180e2e6ab4f9efd25a",
          "message": "fix always_comb overrides, improve VRLDumper layout (#355)\n\n* fix always_comb overrides, improve VRLDumper layout\n\n* clean dumper",
          "timestamp": "2026-04-23T22:40:25+02:00",
          "tree_id": "a96a20bfff22273c9e2d4d29f2752c4df5ced87c",
          "url": "https://github.com/najaeda/naja/commit/162f008f1dadc054864254180e2e6ab4f9efd25a"
        },
        "date": 1776977123207,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 64362.00874206292,
            "unit": "ns/iter",
            "extra": "iterations: 10867\ncpu: 64345.403883316474 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 166084.065248592,
            "unit": "ns/iter",
            "extra": "iterations: 4184\ncpu: 166078.49737093694 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1659928.9239902885,
            "unit": "ns/iter",
            "extra": "iterations: 421\ncpu: 1659866.729216152 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18400840.777777225,
            "unit": "ns/iter",
            "extra": "iterations: 36\ncpu: 18400248.16666667 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1492.1198568988452,
            "unit": "ns/iter",
            "extra": "iterations: 469318\ncpu: 1491.916214592237 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14730.918100827816,
            "unit": "ns/iter",
            "extra": "iterations: 47705\ncpu: 14730.020941201126 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 202230.5137588838,
            "unit": "ns/iter",
            "extra": "iterations: 3525\ncpu: 202206.4014184396 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 296992.6012711975,
            "unit": "ns/iter",
            "extra": "iterations: 2360\ncpu: 296964.47923728806 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 246119.3064516178,
            "unit": "ns/iter",
            "extra": "iterations: 2852\ncpu: 246072.68127629734 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 172228.12306937046,
            "unit": "ns/iter",
            "extra": "iterations: 4079\ncpu: 172210.33145378742 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 74341238.87500732,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 74328964.25000001 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24096.976497709245,
            "unit": "ns/iter",
            "extra": "iterations: 29061\ncpu: 24094.69994150236 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 231032.61043292246,
            "unit": "ns/iter",
            "extra": "iterations: 3029\ncpu: 230818.22020466847 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "2681f9d395472e3bdd615189d04a7e51e3d4f976",
          "message": "tests and fixes for always overrides (#356)\n\n* tests and fixes for always overrides\n\n* improve coverage\n\n* new option to dump assign as instances for debug in VRL dumper and new fix in SV Constructor\n\n* improve coverage\n\n* coverage again",
          "timestamp": "2026-04-25T03:51:49+02:00",
          "tree_id": "4192f080c4f4007cf9c14d3385c51d18ea31ff77",
          "url": "https://github.com/najaeda/naja/commit/2681f9d395472e3bdd615189d04a7e51e3d4f976"
        },
        "date": 1777082149841,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 66445.43010752842,
            "unit": "ns/iter",
            "extra": "iterations: 10416\ncpu: 66440.01862519201 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 165311.0002384877,
            "unit": "ns/iter",
            "extra": "iterations: 4193\ncpu: 165310.11853088482 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1646631.1009389437,
            "unit": "ns/iter",
            "extra": "iterations: 426\ncpu: 1646608.0704225346 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17699206.474999584,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17698942.6 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1494.5861528238875,
            "unit": "ns/iter",
            "extra": "iterations: 468081\ncpu: 1494.5321942142489 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14736.283822851907,
            "unit": "ns/iter",
            "extra": "iterations: 47734\ncpu: 14735.67886621694 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 211195.5457725902,
            "unit": "ns/iter",
            "extra": "iterations: 3430\ncpu: 211178.27288629743 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 292110.85910222976,
            "unit": "ns/iter",
            "extra": "iterations: 2406\ncpu: 292102.5083125522 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 239869.6689466469,
            "unit": "ns/iter",
            "extra": "iterations: 2924\ncpu: 239853.8242134063 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 165353.1948512139,
            "unit": "ns/iter",
            "extra": "iterations: 4234\ncpu: 165343.3656117146 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 73544483.75000544,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 73534218.00000004 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24367.481804044484,
            "unit": "ns/iter",
            "extra": "iterations: 28935\ncpu: 24365.64544669088 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 228217.15344432998,
            "unit": "ns/iter",
            "extra": "iterations: 3063\ncpu: 228111.25106103916 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "a90c4027da7ca387bf43fd82926c2b27f3d2f4ed",
          "message": "mem inference fix (#357)\n\n* mem inference fix\n\n* coverage\n\n* upgrade wheels internal actions\n\nCo-authored-by: Copilot <copilot@github.com>\n\n* coverage\n\n* coverage\n\n---------\n\nCo-authored-by: Copilot <copilot@github.com>",
          "timestamp": "2026-04-25T14:37:48+02:00",
          "tree_id": "3186d8623f2d3ecef8c58906c934562eca9d995e",
          "url": "https://github.com/najaeda/naja/commit/a90c4027da7ca387bf43fd82926c2b27f3d2f4ed"
        },
        "date": 1777120909689,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 67282.57349916549,
            "unit": "ns/iter",
            "extra": "iterations: 10211\ncpu: 67279.63705807464 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 166634.763908707,
            "unit": "ns/iter",
            "extra": "iterations: 4206\ncpu: 166627.06680932 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1658622.1398103635,
            "unit": "ns/iter",
            "extra": "iterations: 422\ncpu: 1658624.5734597158 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17784576.179487944,
            "unit": "ns/iter",
            "extra": "iterations: 39\ncpu: 17784325.128205128 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1517.7144164244532,
            "unit": "ns/iter",
            "extra": "iterations: 461217\ncpu: 1517.6978515536061 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 15022.77027316801,
            "unit": "ns/iter",
            "extra": "iterations: 46638\ncpu: 15021.322076418372 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 208076.10309910844,
            "unit": "ns/iter",
            "extra": "iterations: 3259\ncpu: 208064.04725375897 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 289462.38205978984,
            "unit": "ns/iter",
            "extra": "iterations: 2408\ncpu: 289444.8255813955 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 240204.88808788822,
            "unit": "ns/iter",
            "extra": "iterations: 2913\ncpu: 240196.7562650189 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 165486.7263033135,
            "unit": "ns/iter",
            "extra": "iterations: 4220\ncpu: 165470.07156398133 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 73145154.87500018,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 72949655.49999999 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24027.00355098943,
            "unit": "ns/iter",
            "extra": "iterations: 29006\ncpu: 24025.333241398326 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 228077.87792885245,
            "unit": "ns/iter",
            "extra": "iterations: 3072\ncpu: 227886.9189453169 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "907187b45ae424e70f8a88074e16f98db86e0e2b",
          "message": "Sv assign fix (#358)\n\n* Fix createAssignInstance\n\n* new fixes and starting SV regression by cloning existing repos\n\n* coverage",
          "timestamp": "2026-04-26T15:49:15+02:00",
          "tree_id": "3e525bef4610c7efac2c1b090d39c0ac7d69367e",
          "url": "https://github.com/najaeda/naja/commit/907187b45ae424e70f8a88074e16f98db86e0e2b"
        },
        "date": 1777211613203,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 66288.89758753439,
            "unit": "ns/iter",
            "extra": "iterations: 10653\ncpu: 66270.88970243125 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 169120.99830754605,
            "unit": "ns/iter",
            "extra": "iterations: 4136\ncpu: 169122.25048355895 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1704282.1868932212,
            "unit": "ns/iter",
            "extra": "iterations: 412\ncpu: 1704239.4563106797 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17929223.43589855,
            "unit": "ns/iter",
            "extra": "iterations: 39\ncpu: 17929380.076923076 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1497.8539527281566,
            "unit": "ns/iter",
            "extra": "iterations: 467424\ncpu: 1497.7745237728493 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14602.479475580229,
            "unit": "ns/iter",
            "extra": "iterations: 47748\ncpu: 14601.5719192427 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 216351.9657902222,
            "unit": "ns/iter",
            "extra": "iterations: 3537\ncpu: 216328.08934124958 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 298695.8850278807,
            "unit": "ns/iter",
            "extra": "iterations: 2331\ncpu: 298672.50922350946 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 270330.2392817768,
            "unit": "ns/iter",
            "extra": "iterations: 2729\ncpu: 270318.570172224 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 175136.8413340107,
            "unit": "ns/iter",
            "extra": "iterations: 3958\ncpu: 175128.52223345096 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 74351926.37500165,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 74347494.37499999 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24199.332583414154,
            "unit": "ns/iter",
            "extra": "iterations: 28892\ncpu: 24198.74685033924 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 231477.62865613916,
            "unit": "ns/iter",
            "extra": "iterations: 3043\ncpu: 231294.9408478525 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "committer": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "distinct": true,
          "id": "4fc095dfdd5ff3f51d8dd9e67dd66fbb585ce58f",
          "message": "bp regress",
          "timestamp": "2026-04-26T16:50:05+02:00",
          "tree_id": "20ea8e8acc20c1cd2ba0a4981a04409c2a415b2f",
          "url": "https://github.com/najaeda/naja/commit/4fc095dfdd5ff3f51d8dd9e67dd66fbb585ce58f"
        },
        "date": 1777215237503,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 49738.50055086524,
            "unit": "ns/iter",
            "extra": "iterations: 13615\ncpu: 49736.57605582079 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 130965.99411541077,
            "unit": "ns/iter",
            "extra": "iterations: 5268\ncpu: 130956.5 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1236731.7476979312,
            "unit": "ns/iter",
            "extra": "iterations: 543\ncpu: 1236616.4972375687 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 14253182.940000214,
            "unit": "ns/iter",
            "extra": "iterations: 50\ncpu: 14251921.579999998 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1250.8185288451377,
            "unit": "ns/iter",
            "extra": "iterations: 561654\ncpu: 1250.7406285720388 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12271.414958291762,
            "unit": "ns/iter",
            "extra": "iterations: 56584\ncpu: 12270.787254347533 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 130809.67537594622,
            "unit": "ns/iter",
            "extra": "iterations: 5320\ncpu: 130798.47274436093 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 193192.60488743556,
            "unit": "ns/iter",
            "extra": "iterations: 3642\ncpu: 193187.5354200987 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 151907.0484968222,
            "unit": "ns/iter",
            "extra": "iterations: 4557\ncpu: 151894.8951064296 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 104990.50292836923,
            "unit": "ns/iter",
            "extra": "iterations: 6659\ncpu: 104981.75762126448 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 57698597.000000976,
            "unit": "ns/iter",
            "extra": "iterations: 10\ncpu: 57697909.600000046 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20056.375489180704,
            "unit": "ns/iter",
            "extra": "iterations: 34752\ncpu: 20055.09915976061 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 167825.08688452095,
            "unit": "ns/iter",
            "extra": "iterations: 4178\ncpu: 167764.95955002742 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "0157dd45161580a776b520cbb3415992fe754e8a",
          "message": "Sv fix range selects (#359)\n\n* Fix for Dynamic element-select compound assignment\n\n* update black-parrot setup\n\nCo-authored-by: Copilot <copilot@github.com>\n\n* coverage and rework SV regress\n\n---------\n\nCo-authored-by: Copilot <copilot@github.com>",
          "timestamp": "2026-04-26T21:57:18+02:00",
          "tree_id": "eff5c7907c5098f97cd5c5f3a9c831a34c05f75a",
          "url": "https://github.com/najaeda/naja/commit/0157dd45161580a776b520cbb3415992fe754e8a"
        },
        "date": 1777233694443,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 63930.19478699557,
            "unit": "ns/iter",
            "extra": "iterations: 10704\ncpu: 63926.3389387145 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 165374.4107903257,
            "unit": "ns/iter",
            "extra": "iterations: 4226\ncpu: 165371.5468528159 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1618808.5953492487,
            "unit": "ns/iter",
            "extra": "iterations: 430\ncpu: 1618798.4209302324 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18332517.473685294,
            "unit": "ns/iter",
            "extra": "iterations: 38\ncpu: 18332522.947368417 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1608.788929608292,
            "unit": "ns/iter",
            "extra": "iterations: 433860\ncpu: 1608.671518462176 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 15786.846152113345,
            "unit": "ns/iter",
            "extra": "iterations: 44284\ncpu: 15786.073028633356 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 168532.49570615197,
            "unit": "ns/iter",
            "extra": "iterations: 4192\ncpu: 168518.61784351134 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 245684.53632860424,
            "unit": "ns/iter",
            "extra": "iterations: 2849\ncpu: 245660.7490347492 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 201172.47419728056,
            "unit": "ns/iter",
            "extra": "iterations: 3488\ncpu: 201155.8491972477 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 140587.54432993475,
            "unit": "ns/iter",
            "extra": "iterations: 4850\ncpu: 140579.17525773184 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 74778770.37501912,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 74779725.375 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 25933.09697327522,
            "unit": "ns/iter",
            "extra": "iterations: 27059\ncpu: 25931.694704164944 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 216078.55407555477,
            "unit": "ns/iter",
            "extra": "iterations: 3236\ncpu: 215972.3828801309 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "783b5de40080b91fabb873435f3584535c9b5400",
          "message": "Sv elaboration multidriver fixes (#360)",
          "timestamp": "2026-04-27T23:47:29+02:00",
          "tree_id": "b77ed50e7275e14068acc144f2d403984905f40d",
          "url": "https://github.com/najaeda/naja/commit/783b5de40080b91fabb873435f3584535c9b5400"
        },
        "date": 1777326696021,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 61144.96064837713,
            "unit": "ns/iter",
            "extra": "iterations: 10241\ncpu: 61129.25036617519 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 155797.840863563,
            "unit": "ns/iter",
            "extra": "iterations: 4493\ncpu: 155752.43957266855 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1581562.8036528982,
            "unit": "ns/iter",
            "extra": "iterations: 438\ncpu: 1581427.1986301367 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18484805.48717828,
            "unit": "ns/iter",
            "extra": "iterations: 39\ncpu: 18444184.666666668 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1607.7774445857651,
            "unit": "ns/iter",
            "extra": "iterations: 434184\ncpu: 1607.7304161369384 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 15830.3843352773,
            "unit": "ns/iter",
            "extra": "iterations: 44214\ncpu: 15829.588614466025 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 168202.00406503517,
            "unit": "ns/iter",
            "extra": "iterations: 4182\ncpu: 168195.76996652308 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 243227.0715029535,
            "unit": "ns/iter",
            "extra": "iterations: 2881\ncpu: 243217.5439083652 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 200432.44014890736,
            "unit": "ns/iter",
            "extra": "iterations: 3492\ncpu: 200423.1537800689 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 140063.5572457923,
            "unit": "ns/iter",
            "extra": "iterations: 4996\ncpu: 140058.5806645315 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 73821953.62499999,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 73817535.50000015 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 26391.65218198593,
            "unit": "ns/iter",
            "extra": "iterations: 26925\ncpu: 26389.845942432716 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 217333.8105091169,
            "unit": "ns/iter",
            "extra": "iterations: 3235\ncpu: 217289.61947456113 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "6c3ee26cebd7b7738158f90fcded32461917f3bc",
          "message": "clean coverage and more sv regress (#361)",
          "timestamp": "2026-04-28T11:14:35+02:00",
          "tree_id": "4a9fe5bfd38c88d7373619877f86e4f621b3e64a",
          "url": "https://github.com/najaeda/naja/commit/6c3ee26cebd7b7738158f90fcded32461917f3bc"
        },
        "date": 1777368878654,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 63985.15117744859,
            "unit": "ns/iter",
            "extra": "iterations: 10319\ncpu: 63982.168039538716 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 166221.7482202358,
            "unit": "ns/iter",
            "extra": "iterations: 4214\ncpu: 166217.09729473188 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1587837.4426602481,
            "unit": "ns/iter",
            "extra": "iterations: 436\ncpu: 1587759.6857798162 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18052756.43589947,
            "unit": "ns/iter",
            "extra": "iterations: 39\ncpu: 18051919.538461532 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1607.9067440467286,
            "unit": "ns/iter",
            "extra": "iterations: 432841\ncpu: 1607.8733876873962 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 15836.076499030387,
            "unit": "ns/iter",
            "extra": "iterations: 44262\ncpu: 15835.281957435278 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 176367.60109017635,
            "unit": "ns/iter",
            "extra": "iterations: 4036\ncpu: 176365.74033696728 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 244681.91692957806,
            "unit": "ns/iter",
            "extra": "iterations: 2853\ncpu: 244666.03259726582 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 200537.04507612504,
            "unit": "ns/iter",
            "extra": "iterations: 3483\ncpu: 200528.87367212182 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 141395.50603134287,
            "unit": "ns/iter",
            "extra": "iterations: 4974\ncpu: 141387.45195014053 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 73603288.3750113,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 73592281.87499988 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 25846.981652726943,
            "unit": "ns/iter",
            "extra": "iterations: 27143\ncpu: 25845.559628633524 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 225652.14368846657,
            "unit": "ns/iter",
            "extra": "iterations: 3097\ncpu: 225534.30513400355 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "ffd9b13a4d022c262a140faea2b938f8a53ed767",
          "message": "SV fixes and more regress (#362)\n\n* SV fixes and more regress\n\n* cleaning\n\n* coverage\n\n* new fixes",
          "timestamp": "2026-04-29T09:39:10+02:00",
          "tree_id": "7e9559a1f5a2ea71f65ecb81ff7af1e227189c99",
          "url": "https://github.com/najaeda/naja/commit/ffd9b13a4d022c262a140faea2b938f8a53ed767"
        },
        "date": 1777448604988,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 63430.35274934361,
            "unit": "ns/iter",
            "extra": "iterations: 11039\ncpu: 63427.34595524957 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 165951.15942029233,
            "unit": "ns/iter",
            "extra": "iterations: 4278\ncpu: 165945.56545114538 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1647723.7146226957,
            "unit": "ns/iter",
            "extra": "iterations: 424\ncpu: 1647698.4268867918 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17621052.349997513,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17619986.250000007 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1494.296907379059,
            "unit": "ns/iter",
            "extra": "iterations: 469537\ncpu: 1494.1977139181797 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14654.676635319669,
            "unit": "ns/iter",
            "extra": "iterations: 47850\ncpu: 14654.262006269604 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 211001.9849537181,
            "unit": "ns/iter",
            "extra": "iterations: 3456\ncpu: 210997.71383101828 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 286965.4399670907,
            "unit": "ns/iter",
            "extra": "iterations: 2432\ncpu: 286949.53741776285 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 237191.2468675926,
            "unit": "ns/iter",
            "extra": "iterations: 2953\ncpu: 237180.57433118898 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 162994.60176170702,
            "unit": "ns/iter",
            "extra": "iterations: 4314\ncpu: 162986.94668521098 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 73764964.74999782,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 73761364.87499996 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24089.55481269814,
            "unit": "ns/iter",
            "extra": "iterations: 29017\ncpu: 24087.09191163801 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 223437.91602796077,
            "unit": "ns/iter",
            "extra": "iterations: 3132\ncpu: 223342.98020434624 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "a472a822064d5a2d942bebc89ef873676ea076c9",
          "message": "sim fixes and ibex hello world (#363)",
          "timestamp": "2026-05-01T11:23:11+02:00",
          "tree_id": "61749b68d9cd089543528647a29429877cf621b0",
          "url": "https://github.com/najaeda/naja/commit/a472a822064d5a2d942bebc89ef873676ea076c9"
        },
        "date": 1777627641742,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 65101.91273996536,
            "unit": "ns/iter",
            "extra": "iterations: 10887\ncpu: 65096.42105263158 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 164071.9695937749,
            "unit": "ns/iter",
            "extra": "iterations: 4111\ncpu: 164070.75188518612 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1643683.5938967485,
            "unit": "ns/iter",
            "extra": "iterations: 426\ncpu: 1643576.1079812204 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17728224.15384722,
            "unit": "ns/iter",
            "extra": "iterations: 39\ncpu: 17726552.97435897 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1506.598434325141,
            "unit": "ns/iter",
            "extra": "iterations: 467530\ncpu: 1505.1690608089323 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14701.922143727326,
            "unit": "ns/iter",
            "extra": "iterations: 47562\ncpu: 14701.600100920905 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 207757.18216823292,
            "unit": "ns/iter",
            "extra": "iterations: 3376\ncpu: 207735.84004739326 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 293343.0058552846,
            "unit": "ns/iter",
            "extra": "iterations: 2391\ncpu: 293334.97114178166 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 243032.0521408843,
            "unit": "ns/iter",
            "extra": "iterations: 2896\ncpu: 243011.44751381225 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 169885.32905569675,
            "unit": "ns/iter",
            "extra": "iterations: 4130\ncpu: 169878.0426150121 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 72350346.74999952,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 72348230.00000001 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24122.303338369915,
            "unit": "ns/iter",
            "extra": "iterations: 29116\ncpu: 24121.006250858623 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 222907.60451545217,
            "unit": "ns/iter",
            "extra": "iterations: 3143\ncpu: 222751.35475656533 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "83a2dbaa96f77f824d49e902a292c2e27eb5d10f",
          "message": "clean coverage (#365)",
          "timestamp": "2026-05-02T22:28:56+02:00",
          "tree_id": "875ea6d0b19e5f3289522d8a5e768271890e8d77",
          "url": "https://github.com/najaeda/naja/commit/83a2dbaa96f77f824d49e902a292c2e27eb5d10f"
        },
        "date": 1777754032713,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 64802.40595683264,
            "unit": "ns/iter",
            "extra": "iterations: 10979\ncpu: 64794.657072593145 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 166158.71480939613,
            "unit": "ns/iter",
            "extra": "iterations: 4092\ncpu: 166150.5146627566 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1658027.8625591989,
            "unit": "ns/iter",
            "extra": "iterations: 422\ncpu: 1657820.9834123214 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17973838.769232012,
            "unit": "ns/iter",
            "extra": "iterations: 39\ncpu: 17971590.461538456 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1505.1090120090316,
            "unit": "ns/iter",
            "extra": "iterations: 465490\ncpu: 1504.9878472147623 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14658.346132886902,
            "unit": "ns/iter",
            "extra": "iterations: 47710\ncpu: 14655.467009012786 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 212235.57480085216,
            "unit": "ns/iter",
            "extra": "iterations: 3389\ncpu: 212208.9589849511 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 292103.6544240376,
            "unit": "ns/iter",
            "extra": "iterations: 2396\ncpu: 292083.4632721201 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 242139.22954777774,
            "unit": "ns/iter",
            "extra": "iterations: 2897\ncpu: 242116.77597514712 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 170164.25699805465,
            "unit": "ns/iter",
            "extra": "iterations: 4144\ncpu: 170152.55743243246 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 74696074.6250096,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 74694466.75000001 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24109.88249672801,
            "unit": "ns/iter",
            "extra": "iterations: 29046\ncpu: 24107.71972044346 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 224087.3783409607,
            "unit": "ns/iter",
            "extra": "iterations: 3140\ncpu: 223981.79904459108 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "9dde4931c2c2492d162c4a2989c6736508dc5952",
          "message": "cleaning regress (#366)\n\n* cleaning regress\n\n* add round script to verify SV loading - CVA6 loading\n\n* fix SV\n\n* SV fix\n\n* fix multi driver\n\n* compute coverange with clang\n\n* fix coverage\n\n* new attempt\n\n* coverage\n\n* new testing",
          "timestamp": "2026-05-04T23:05:23+02:00",
          "tree_id": "fdfe2546cc4612e929db6b50ff3700d4870e433a",
          "url": "https://github.com/najaeda/naja/commit/9dde4931c2c2492d162c4a2989c6736508dc5952"
        },
        "date": 1777929082012,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 66445.29497850491,
            "unit": "ns/iter",
            "extra": "iterations: 10933\ncpu: 66424.8613372359 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 166669.39733206175,
            "unit": "ns/iter",
            "extra": "iterations: 4198\ncpu: 166620.12243925678 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1665143.9857819306,
            "unit": "ns/iter",
            "extra": "iterations: 422\ncpu: 1664757.6279620857 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17729596.384615667,
            "unit": "ns/iter",
            "extra": "iterations: 39\ncpu: 17729151.589743588 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1498.989876104578,
            "unit": "ns/iter",
            "extra": "iterations: 468298\ncpu: 1498.9612810646208 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14785.089008851175,
            "unit": "ns/iter",
            "extra": "iterations: 47793\ncpu: 14784.630887368434 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 202019.56215887377,
            "unit": "ns/iter",
            "extra": "iterations: 3298\ncpu: 202019.3599150999 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 288082.16220665636,
            "unit": "ns/iter",
            "extra": "iterations: 2429\ncpu: 288085.144092219 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 238309.7568212817,
            "unit": "ns/iter",
            "extra": "iterations: 2932\ncpu: 238303.83799454296 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 164261.55289940457,
            "unit": "ns/iter",
            "extra": "iterations: 4225\ncpu: 164261.03857988166 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 72283337.99999832,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 72279950.25 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24126.11891798755,
            "unit": "ns/iter",
            "extra": "iterations: 29020\ncpu: 24125.931564438317 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 226344.76880814857,
            "unit": "ns/iter",
            "extra": "iterations: 3097\ncpu: 226173.2102034208 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "noam.chn1@gmail.com",
            "name": "Noam Cohen",
            "username": "nanocoh"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "05a14c34844b62a2e40e8ff459d0cf7bb5d6d9ee",
          "message": "memory modeling (#364)",
          "timestamp": "2026-05-05T09:47:35+02:00",
          "tree_id": "3f530f55aba064adb86a5d804a7157c4180a194d",
          "url": "https://github.com/najaeda/naja/commit/05a14c34844b62a2e40e8ff459d0cf7bb5d6d9ee"
        },
        "date": 1777967509005,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 68850.65832291047,
            "unit": "ns/iter",
            "extra": "iterations: 10387\ncpu: 68845.31828246848 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 169191.8628115307,
            "unit": "ns/iter",
            "extra": "iterations: 4133\ncpu: 169147.99806436003 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1662839.8622327899,
            "unit": "ns/iter",
            "extra": "iterations: 421\ncpu: 1662785.0546318288 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18162168.153848004,
            "unit": "ns/iter",
            "extra": "iterations: 39\ncpu: 18161307.58974358 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1493.0967169589794,
            "unit": "ns/iter",
            "extra": "iterations: 468925\ncpu: 1493.077340726128 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14660.215318918343,
            "unit": "ns/iter",
            "extra": "iterations: 47771\ncpu: 14659.136379812015 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 206853.15352816696,
            "unit": "ns/iter",
            "extra": "iterations: 3387\ncpu: 206820.86920578673 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 298380.5255754512,
            "unit": "ns/iter",
            "extra": "iterations: 2346\ncpu: 298296.1884057971 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 247021.5408016902,
            "unit": "ns/iter",
            "extra": "iterations: 2794\ncpu: 246989.17752326385 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 173405.18961231835,
            "unit": "ns/iter",
            "extra": "iterations: 4024\ncpu: 173390.02037773374 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 73098221.12499376,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 73084998.37499993 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24237.24224913209,
            "unit": "ns/iter",
            "extra": "iterations: 28900\ncpu: 24234.92055363323 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 227557.9571009058,
            "unit": "ns/iter",
            "extra": "iterations: 3077\ncpu: 226801.33994147487 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "cfefd37c8e2c5d64d574105b83f86641792e70ce",
          "message": "Cv32e40p simulation (#367)\n\n* adding simulation setup\n\n* fix simulation\n\n* sim again\n\n* update regress\n\n* clean getString\n\n* name assign\n\n* clean test\n\n* cleaning and coverage\n\n* cleaning\n\n* test bundle terms\n\n* setting global coverage\n\n* add license",
          "timestamp": "2026-05-07T10:37:58+02:00",
          "tree_id": "85830e878a67aae4df9620a219ce532975af08ff",
          "url": "https://github.com/najaeda/naja/commit/cfefd37c8e2c5d64d574105b83f86641792e70ce"
        },
        "date": 1778143338594,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 69754.9769589849,
            "unit": "ns/iter",
            "extra": "iterations: 10069\ncpu: 69750.01072599068 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 167537.4248568605,
            "unit": "ns/iter",
            "extra": "iterations: 4192\ncpu: 167535.8373091603 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1658631.3466980453,
            "unit": "ns/iter",
            "extra": "iterations: 424\ncpu: 1658629.9033018867 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18449255.02702633,
            "unit": "ns/iter",
            "extra": "iterations: 37\ncpu: 18447721.216216207 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1509.3277644641062,
            "unit": "ns/iter",
            "extra": "iterations: 466103\ncpu: 1508.9952714314213 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14717.557545026368,
            "unit": "ns/iter",
            "extra": "iterations: 47528\ncpu: 14705.090809627996 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 212574.67580491543,
            "unit": "ns/iter",
            "extra": "iterations: 3137\ncpu: 212535.62958240343 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 292869.19616027095,
            "unit": "ns/iter",
            "extra": "iterations: 2396\ncpu: 292847.85225375637 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 241846.55498105392,
            "unit": "ns/iter",
            "extra": "iterations: 2901\ncpu: 241821.70492933484 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 167951.53849844166,
            "unit": "ns/iter",
            "extra": "iterations: 4169\ncpu: 167941.15543295737 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 72230230.0000024,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 72226162.25 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24138.492338946948,
            "unit": "ns/iter",
            "extra": "iterations: 29043\ncpu: 24137.187928244315 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 229203.69356971834,
            "unit": "ns/iter",
            "extra": "iterations: 3048\ncpu: 229107.83497374287 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "6a3d849c8c0f4fedb93c6a5be0d07ac4e5b1df60",
          "message": "new cv32e40p sim and fix for bug detected with the design (#368)",
          "timestamp": "2026-05-07T21:46:03+02:00",
          "tree_id": "79f1b8a83e5a9512c30ceed9a24e32382a58e23a",
          "url": "https://github.com/najaeda/naja/commit/6a3d849c8c0f4fedb93c6a5be0d07ac4e5b1df60"
        },
        "date": 1778183417863,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 67912.50979637541,
            "unit": "ns/iter",
            "extra": "iterations: 10412\ncpu: 67892.19746446409 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 166710.77539342578,
            "unit": "ns/iter",
            "extra": "iterations: 4194\ncpu: 166701.43800667624 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1644363.6861822596,
            "unit": "ns/iter",
            "extra": "iterations: 427\ncpu: 1644320.5573770485 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17587399.12500005,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17585670.825 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1588.202610963497,
            "unit": "ns/iter",
            "extra": "iterations: 442059\ncpu: 1587.7579825317428 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 15572.718072828622,
            "unit": "ns/iter",
            "extra": "iterations: 45040\ncpu: 15570.907504440489 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 208025.90504099915,
            "unit": "ns/iter",
            "extra": "iterations: 3412\ncpu: 208004.44812426754 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 296326.364179049,
            "unit": "ns/iter",
            "extra": "iterations: 2345\ncpu: 296301.09936034086 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 247847.85714290992,
            "unit": "ns/iter",
            "extra": "iterations: 2842\ncpu: 247629.51337086546 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 174900.57525671917,
            "unit": "ns/iter",
            "extra": "iterations: 3993\ncpu: 174892.10493363408 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 72906311.49999171,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 72669894.37500015 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24106.015097721138,
            "unit": "ns/iter",
            "extra": "iterations: 29011\ncpu: 24105.005928785606 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 228710.61038201992,
            "unit": "ns/iter",
            "extra": "iterations: 3062\ncpu: 228061.6463095866 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "af1d00128ca51f9182c7f37cdbd5ce3faac4a9cc",
          "message": "IBEX sim and fix (#369)",
          "timestamp": "2026-05-08T10:54:00+02:00",
          "tree_id": "30c067cdfc2b4f57ba952146cc9945e565a9196f",
          "url": "https://github.com/najaeda/naja/commit/af1d00128ca51f9182c7f37cdbd5ce3faac4a9cc"
        },
        "date": 1778231324730,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 51177.94448068902,
            "unit": "ns/iter",
            "extra": "iterations: 13797\ncpu: 51173.509023700804 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 131676.43695897478,
            "unit": "ns/iter",
            "extra": "iterations: 5314\ncpu: 131670.1802785096 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1224155.9982143713,
            "unit": "ns/iter",
            "extra": "iterations: 560\ncpu: 1224031.4303571428 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 13686646.098039107,
            "unit": "ns/iter",
            "extra": "iterations: 51\ncpu: 13685452.352941174 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1269.6179137060485,
            "unit": "ns/iter",
            "extra": "iterations: 557908\ncpu: 1269.4472206887162 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12427.16530375975,
            "unit": "ns/iter",
            "extra": "iterations: 53562\ncpu: 12425.907453045067 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 133388.56502751706,
            "unit": "ns/iter",
            "extra": "iterations: 5267\ncpu: 133374.10708183027 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 186813.30647750272,
            "unit": "ns/iter",
            "extra": "iterations: 3736\ncpu: 186809.20449678806 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 152899.60528613676,
            "unit": "ns/iter",
            "extra": "iterations: 4578\ncpu: 152881.3997378767 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 104743.77313838765,
            "unit": "ns/iter",
            "extra": "iterations: 6634\ncpu: 104736.21359662343 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 56542619.499998644,
            "unit": "ns/iter",
            "extra": "iterations: 10\ncpu: 56538230.80000003 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20488.46210683098,
            "unit": "ns/iter",
            "extra": "iterations: 34241\ncpu: 20486.594813235573 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 168152.67312281163,
            "unit": "ns/iter",
            "extra": "iterations: 4182\ncpu: 167970.5277378991 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "committer": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "distinct": true,
          "id": "dbbac1c86735a01104c9651dfbe2f110df50f5a7",
          "message": "more ibex sim",
          "timestamp": "2026-05-08T11:39:54+02:00",
          "tree_id": "25ebf688b233fdfc607f8a47ed075804db04fedf",
          "url": "https://github.com/najaeda/naja/commit/dbbac1c86735a01104c9651dfbe2f110df50f5a7"
        },
        "date": 1778234251363,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 60259.047533251316,
            "unit": "ns/iter",
            "extra": "iterations: 11655\ncpu: 60254.8906906907 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 155900.38852934886,
            "unit": "ns/iter",
            "extra": "iterations: 4481\ncpu: 155889.62776166038 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1518824.0865800902,
            "unit": "ns/iter",
            "extra": "iterations: 462\ncpu: 1518715.8051948044 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17825708.50000127,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17824586.19999999 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1217.8182625328773,
            "unit": "ns/iter",
            "extra": "iterations: 575539\ncpu: 1217.8016033665826 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12650.076904931122,
            "unit": "ns/iter",
            "extra": "iterations: 55107\ncpu: 12649.052298256109 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 129179.92204251229,
            "unit": "ns/iter",
            "extra": "iterations: 5503\ncpu: 129166.17790296208 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 247363.70685993772,
            "unit": "ns/iter",
            "extra": "iterations: 2828\ncpu: 247339.4614568598 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 203117.6142029018,
            "unit": "ns/iter",
            "extra": "iterations: 3450\ncpu: 203100.8857971015 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 136445.94330399827,
            "unit": "ns/iter",
            "extra": "iterations: 5115\ncpu: 136428.47585532742 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 72842500.55555376,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 72837140.33333331 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24180.369529085132,
            "unit": "ns/iter",
            "extra": "iterations: 28880\ncpu: 24177.99103185592 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 155041.6091938398,
            "unit": "ns/iter",
            "extra": "iterations: 4547\ncpu: 154736.85374973525 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "002deb8d229e62e8f0fc256f10064412b824e980",
          "message": "More SV sims (#370)",
          "timestamp": "2026-05-09T15:23:26+02:00",
          "tree_id": "e65cfbbfd53ba17ba29436d73351ee866de992e9",
          "url": "https://github.com/najaeda/naja/commit/002deb8d229e62e8f0fc256f10064412b824e980"
        },
        "date": 1778333252528,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 66270.68826815285,
            "unit": "ns/iter",
            "extra": "iterations: 10740\ncpu: 66248.83249534451 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 165382.06310336044,
            "unit": "ns/iter",
            "extra": "iterations: 4247\ncpu: 165335.11914292438 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1642182.8454332573,
            "unit": "ns/iter",
            "extra": "iterations: 427\ncpu: 1641726.0327868857 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17368797.400000345,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17368415.574999996 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1491.597666797099,
            "unit": "ns/iter",
            "extra": "iterations: 467769\ncpu: 1491.5278075289298 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14617.583565721578,
            "unit": "ns/iter",
            "extra": "iterations: 47693\ncpu: 14617.288029689891 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 211254.29985096972,
            "unit": "ns/iter",
            "extra": "iterations: 3355\ncpu: 211255.53800298058 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 296301.7428209657,
            "unit": "ns/iter",
            "extra": "iterations: 2368\ncpu: 296292.2461993246 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 244824.03155680993,
            "unit": "ns/iter",
            "extra": "iterations: 2852\ncpu: 244815.94319775593 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 172808.45458996136,
            "unit": "ns/iter",
            "extra": "iterations: 4085\ncpu: 172801.17625458984 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 71581757.24999438,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 71574267.50000018 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24104.91974906592,
            "unit": "ns/iter",
            "extra": "iterations: 29171\ncpu: 24104.538719961572 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 224354.83407907214,
            "unit": "ns/iter",
            "extra": "iterations: 3128\ncpu: 224230.05019180072 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "acd6ffaf1e6a64912b7139c8fea9a6fd736376f4",
          "message": "addinx new axi bench and towards 0.6.3 (#371)",
          "timestamp": "2026-05-14T15:32:02+02:00",
          "tree_id": "b87c48602ed81b8a72e8dc65ce9e518e7f823159",
          "url": "https://github.com/najaeda/naja/commit/acd6ffaf1e6a64912b7139c8fea9a6fd736376f4"
        },
        "date": 1778765777385,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 64203.228613296946,
            "unit": "ns/iter",
            "extra": "iterations: 10918\ncpu: 64179.14480674116 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 165270.38539697498,
            "unit": "ns/iter",
            "extra": "iterations: 4232\ncpu: 165264.91351606805 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1568808.348519298,
            "unit": "ns/iter",
            "extra": "iterations: 439\ncpu: 1568696.0751708425 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17564481.299999103,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17563827.249999996 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1622.9057842871102,
            "unit": "ns/iter",
            "extra": "iterations: 432603\ncpu: 1622.087720149883 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 16012.180478652119,
            "unit": "ns/iter",
            "extra": "iterations: 43706\ncpu: 16010.888756692453 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 173417.49754300856,
            "unit": "ns/iter",
            "extra": "iterations: 4070\ncpu: 173395.67813267803 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 240324.37339355596,
            "unit": "ns/iter",
            "extra": "iterations: 2879\ncpu: 240311.0211879123 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 195364.41085923064,
            "unit": "ns/iter",
            "extra": "iterations: 3573\ncpu: 195331.66442765167 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 135582.46952161263,
            "unit": "ns/iter",
            "extra": "iterations: 5184\ncpu: 135568.64949845657 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 74290677.00000757,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 74280224.37500003 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 26275.454650858366,
            "unit": "ns/iter",
            "extra": "iterations: 26737\ncpu: 26272.792235478948 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 216517.76255276418,
            "unit": "ns/iter",
            "extra": "iterations: 3226\ncpu: 216383.65654060172 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "17544c56bc1234953396bee2ca0c9e352771d71d",
          "message": "B0.6.4 (#372)",
          "timestamp": "2026-05-17T00:07:16+02:00",
          "tree_id": "dcdc0c35ade858a23f60f084af3504cfff98bf96",
          "url": "https://github.com/najaeda/naja/commit/17544c56bc1234953396bee2ca0c9e352771d71d"
        },
        "date": 1778969481323,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 65980.14330365245,
            "unit": "ns/iter",
            "extra": "iterations: 10558\ncpu: 65972.47253267665 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 161131.24687216157,
            "unit": "ns/iter",
            "extra": "iterations: 3277\ncpu: 161115.91547146783 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1568373.42953019,
            "unit": "ns/iter",
            "extra": "iterations: 447\ncpu: 1568320.4250559288 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17592918.125001233,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17592204.525 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1618.956286420517,
            "unit": "ns/iter",
            "extra": "iterations: 433824\ncpu: 1618.9188334439768 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 16012.45214982527,
            "unit": "ns/iter",
            "extra": "iterations: 43678\ncpu: 16011.917303905868 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 173592.59941802817,
            "unit": "ns/iter",
            "extra": "iterations: 4124\ncpu: 173587.96047526662 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 250925.2540308258,
            "unit": "ns/iter",
            "extra": "iterations: 2791\ncpu: 250919.48907201728 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 207841.0568720493,
            "unit": "ns/iter",
            "extra": "iterations: 3376\ncpu: 207835.4567535545 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 147142.5204038645,
            "unit": "ns/iter",
            "extra": "iterations: 4754\ncpu: 147135.12010938174 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 74414226.74999388,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 74408623.37500009 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 26307.43138435804,
            "unit": "ns/iter",
            "extra": "iterations: 26561\ncpu: 26306.71085426003 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 224617.72820723,
            "unit": "ns/iter",
            "extra": "iterations: 3109\ncpu: 224503.57060146885 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "7c620f8b3ab850ef20e9c9094df8b11ac8f68900",
          "message": "Tutorials readme reworked (#374)\n\n* prepare next version and simplify README\n\n* merging tutorials in repo in progress\n\n* integrate tutorials in naja\n\n* updates to README\n\n* clean licenses\n\n* setup najaeda to test tutorials\n\n* fix tutorial\n\n* don't change version",
          "timestamp": "2026-05-18T12:11:28+02:00",
          "tree_id": "d8301627ebb4c5ce4528e212606477d2fc96a7db",
          "url": "https://github.com/najaeda/naja/commit/7c620f8b3ab850ef20e9c9094df8b11ac8f68900"
        },
        "date": 1779099385419,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 67083.72104180636,
            "unit": "ns/iter",
            "extra": "iterations: 10213\ncpu: 67082.15313815726 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 175237.0410307699,
            "unit": "ns/iter",
            "extra": "iterations: 3997\ncpu: 175227.8628971729 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1679805.6043167165,
            "unit": "ns/iter",
            "extra": "iterations: 417\ncpu: 1679724.2829736213 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18819332.861110322,
            "unit": "ns/iter",
            "extra": "iterations: 36\ncpu: 18817621.77777778 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1608.8827710904682,
            "unit": "ns/iter",
            "extra": "iterations: 435814\ncpu: 1608.8127297425037 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 15808.37285643204,
            "unit": "ns/iter",
            "extra": "iterations: 44202\ncpu: 15807.994796615545 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 169542.19834104966,
            "unit": "ns/iter",
            "extra": "iterations: 4099\ncpu: 169533.78506952917 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 244892.76745808125,
            "unit": "ns/iter",
            "extra": "iterations: 2864\ncpu: 244880.21194134103 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 199584.17997720052,
            "unit": "ns/iter",
            "extra": "iterations: 3506\ncpu: 199568.9663434111 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 139869.35721392787,
            "unit": "ns/iter",
            "extra": "iterations: 5025\ncpu: 139866.0927363184 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 74420057.62500514,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 74411487.12500012 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 26898.578802370994,
            "unit": "ns/iter",
            "extra": "iterations: 26135\ncpu: 26897.48433135645 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 224688.13540191992,
            "unit": "ns/iter",
            "extra": "iterations: 3124\ncpu: 224614.22791291645 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "3634212302c1916a81bf9c8de672e66bde7e3a81",
          "message": "Fix for SV (#375)",
          "timestamp": "2026-05-19T07:33:06+02:00",
          "tree_id": "0787e8c2967eb03306e8732ce74cbfdfa5fa4d2f",
          "url": "https://github.com/najaeda/naja/commit/3634212302c1916a81bf9c8de672e66bde7e3a81"
        },
        "date": 1779169024521,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 66227.59052374613,
            "unit": "ns/iter",
            "extra": "iterations: 10616\ncpu: 66227.26525998492 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 167519.85274233096,
            "unit": "ns/iter",
            "extra": "iterations: 3993\ncpu: 167510.07162534434 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1644323.2763464863,
            "unit": "ns/iter",
            "extra": "iterations: 427\ncpu: 1644242.5011709605 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17778545.72500246,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17778292.89999999 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1499.8809501900205,
            "unit": "ns/iter",
            "extra": "iterations: 467275\ncpu: 1499.8484875073555 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14696.245271136688,
            "unit": "ns/iter",
            "extra": "iterations: 47633\ncpu: 14695.829844855445 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 207863.0689558295,
            "unit": "ns/iter",
            "extra": "iterations: 3553\ncpu: 207859.32395159034 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 295121.4587888674,
            "unit": "ns/iter",
            "extra": "iterations: 2378\ncpu: 295106.0815811606 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 244856.1006969346,
            "unit": "ns/iter",
            "extra": "iterations: 2870\ncpu: 244856.82717770006 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 169846.3176328207,
            "unit": "ns/iter",
            "extra": "iterations: 4140\ncpu: 169836.0041062799 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 73042692.12500003,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 73037226.3750001 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24189.819570697753,
            "unit": "ns/iter",
            "extra": "iterations: 28931\ncpu: 24188.581417856265 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 233304.5716638935,
            "unit": "ns/iter",
            "extra": "iterations: 3000\ncpu: 233196.91166670728 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "6d7199df68d6fb326bb3c62c65a9bdf759460c18",
          "message": "B0.6.6 (#376)\n\n* prepare next version\n\n* remove PINMISSING suppression warning in verilator linting\n\n* more SV support\n\n* edit IBEX linting\n\n* more avoided warnings\n\n* more suppression\n\n* more SV support\n\n* Fix more SV and tutorials\n\n* new support and cleaning\n\n* more SV support\n\n* next row of support\n\n* more support\n\n* improve coverage\n\n* coverage\n\n* coverage\n\n* coverage\n\n* coverage",
          "timestamp": "2026-05-21T15:52:30+02:00",
          "tree_id": "7c7012a74a21c42ed82c7a2923fb628995da0d61",
          "url": "https://github.com/najaeda/naja/commit/6d7199df68d6fb326bb3c62c65a9bdf759460c18"
        },
        "date": 1779371794416,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 62751.62666029851,
            "unit": "ns/iter",
            "extra": "iterations: 10465\ncpu: 62749.19034878165 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 154432.5795154114,
            "unit": "ns/iter",
            "extra": "iterations: 4540\ncpu: 154423.35374449336 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1559159.5501112763,
            "unit": "ns/iter",
            "extra": "iterations: 449\ncpu: 1559084.4565701562 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17319812.62500142,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17316640.1 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1608.816908904764,
            "unit": "ns/iter",
            "extra": "iterations: 434008\ncpu: 1608.6466009843145 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 15845.566482088323,
            "unit": "ns/iter",
            "extra": "iterations: 44245\ncpu: 15843.827076505822 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 169523.1939291759,
            "unit": "ns/iter",
            "extra": "iterations: 4151\ncpu: 169505.58949650684 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 242123.0606687434,
            "unit": "ns/iter",
            "extra": "iterations: 2901\ncpu: 241928.1447776628 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 198215.22115386577,
            "unit": "ns/iter",
            "extra": "iterations: 3536\ncpu: 198199.48755656095 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 137354.17583495812,
            "unit": "ns/iter",
            "extra": "iterations: 5090\ncpu: 137352.51434184695 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 74588179.74999477,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 74584053.62499999 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 26224.147117219985,
            "unit": "ns/iter",
            "extra": "iterations: 26693\ncpu: 26223.38216011685 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 225218.88949700422,
            "unit": "ns/iter",
            "extra": "iterations: 3104\ncpu: 224994.51449740244 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "8fa27fca8bfe64dd5f19229546491e3301b93024",
          "message": "test and fix for the bug (#378)\n\n* test and fix for the bug\n\n* coverage\n\n* Trigger CI\n\n* Trigger CI",
          "timestamp": "2026-05-26T16:50:48+02:00",
          "tree_id": "bafec6b8b66fe5371cca74ceb70abff5809d9243",
          "url": "https://github.com/najaeda/naja/commit/8fa27fca8bfe64dd5f19229546491e3301b93024"
        },
        "date": 1779807296320,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 66317.43007990801,
            "unit": "ns/iter",
            "extra": "iterations: 10512\ncpu: 66297.38061263318 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 155026.2799822622,
            "unit": "ns/iter",
            "extra": "iterations: 4511\ncpu: 155019.38062513858 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1560913.5666666892,
            "unit": "ns/iter",
            "extra": "iterations: 450\ncpu: 1560854.1977777774 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17645993.600000054,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17645409.724999994 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1604.4662643641816,
            "unit": "ns/iter",
            "extra": "iterations: 436156\ncpu: 1604.430942139968 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 15796.447381501412,
            "unit": "ns/iter",
            "extra": "iterations: 44262\ncpu: 15795.046315123569 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 172703.0081200691,
            "unit": "ns/iter",
            "extra": "iterations: 4064\ncpu: 172687.83686023628 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 242933.4833625119,
            "unit": "ns/iter",
            "extra": "iterations: 2855\ncpu: 242920.0739054289 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 198268.9177538287,
            "unit": "ns/iter",
            "extra": "iterations: 3526\ncpu: 198254.2532614861 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 138505.16116926546,
            "unit": "ns/iter",
            "extra": "iterations: 5063\ncpu: 138484.38336954374 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 74675650.12499477,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 74666502.24999994 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 26756.768899451396,
            "unit": "ns/iter",
            "extra": "iterations: 26932\ncpu: 26754.532080796085 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 223112.72733033373,
            "unit": "ns/iter",
            "extra": "iterations: 3154\ncpu: 222894.1122384599 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "d4bab5e687cedb120217261e42209ee53abd03f2",
          "message": "B0.6.7 (#377)\n\n* Towards 0.6.7 and more SV support\n\n* divmod prim\n\n* more SV support\n\n* support more latch cases\n\n* new latch support\n\n* new support\n\n* next row of support\n\n* next support\n\n* Trigger CI\n\n* Trigger CI\n\n* new row of support\n\n* next row\n\n* more support\n\n* new set of support\n\n* update workflow\n\n* update tutorial\n\n* clean dot output\n\n* parallel tests\n\n* precompute timing modeling as it is not thread safe\n\n* Revert \"precompute timing modeling as it is not thread safe\"\n\nThis reverts commit 8bc5be5e2ff72e62cbe455ab680369fc319f2221.\n\n* cleaning workflows and adding //\n\n* test with tsan\n\n* test cleaning\n\n* edit tsan options\n\n* coverage\n\n* tsan avoiding TBB noise\n\n* mt safety fix for NLName\n\n* revert test change\n\n* Revert \"mt safety fix for NLName\"\n\n* clean dot usage through option in tests\n\n* set -> unique vector for ll\n\n* preprocess design before multi-threading operation\n\n* clean tsan\n\n* cleaning\n\n* rerun wheels\n\n* run wheels\n\n* coverage\n\n* coverage\n\n* coverage\n\n---------\n\nCo-authored-by: Noam Cohen <noam.chn1@gmail.com>",
          "timestamp": "2026-06-01T22:02:34+02:00",
          "tree_id": "edebf00c874a20531b0e5d90ab6b5e6e7f9b2b6f",
          "url": "https://github.com/najaeda/naja/commit/d4bab5e687cedb120217261e42209ee53abd03f2"
        },
        "date": 1780344287275,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 64654.09025669152,
            "unit": "ns/iter",
            "extra": "iterations: 10869\ncpu: 64644.5771460116 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 166232.24613555634,
            "unit": "ns/iter",
            "extra": "iterations: 4205\ncpu: 166224.21569560052 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1577295.9225513358,
            "unit": "ns/iter",
            "extra": "iterations: 439\ncpu: 1577200.0842824602 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17987414.73684261,
            "unit": "ns/iter",
            "extra": "iterations: 38\ncpu: 17986966.789473683 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1611.8903845226664,
            "unit": "ns/iter",
            "extra": "iterations: 435787\ncpu: 1611.7881602709574 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 15875.0420071066,
            "unit": "ns/iter",
            "extra": "iterations: 44183\ncpu: 15874.004707692991 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 171539.0678997409,
            "unit": "ns/iter",
            "extra": "iterations: 4109\ncpu: 171528.07544414702 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 243494.7500881203,
            "unit": "ns/iter",
            "extra": "iterations: 2837\ncpu: 243477.86640817756 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 198730.4209631658,
            "unit": "ns/iter",
            "extra": "iterations: 3530\ncpu: 198718.30623229465 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 140394.7392354052,
            "unit": "ns/iter",
            "extra": "iterations: 4970\ncpu: 140380.36458752514 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 74395187.62500086,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 74396279.87499997 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 26045.12090457713,
            "unit": "ns/iter",
            "extra": "iterations: 27195\ncpu: 26043.985364956763 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 224637.9570808623,
            "unit": "ns/iter",
            "extra": "iterations: 3099\ncpu: 224580.17554049642 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "12b3e86bcb5326b06ecfe97864e865dda459402b",
          "message": "add rl8 support (#380)\n\n* add rl8 support\n\n* new attempt\n\n* adapt parser to rockylinux 8\n\n* use concurrent_hash_map from tbb\n\n* compile oneTBB\n\n* fix curl",
          "timestamp": "2026-06-03T10:44:19+02:00",
          "tree_id": "bf6aeb3d76a7bacb62986ca59bbdcc37ed6dbe59",
          "url": "https://github.com/najaeda/naja/commit/12b3e86bcb5326b06ecfe97864e865dda459402b"
        },
        "date": 1780476383835,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 50980.80970000183,
            "unit": "ns/iter",
            "extra": "iterations: 10000\ncpu: 50959.79029999999 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 131178.2512333899,
            "unit": "ns/iter",
            "extra": "iterations: 5270\ncpu: 131142.8322580645 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1263827.4405851292,
            "unit": "ns/iter",
            "extra": "iterations: 547\ncpu: 1263755.7824497255 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 14267429.755101817,
            "unit": "ns/iter",
            "extra": "iterations: 49\ncpu: 14266901.653061224 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1246.360376499326,
            "unit": "ns/iter",
            "extra": "iterations: 562126\ncpu: 1246.1646943923606 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12269.764356435553,
            "unit": "ns/iter",
            "extra": "iterations: 57065\ncpu: 12267.10092000351 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 132667.74849171014,
            "unit": "ns/iter",
            "extra": "iterations: 5304\ncpu: 132631.33163650084 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 188870.44160388454,
            "unit": "ns/iter",
            "extra": "iterations: 3716\ncpu: 188836.75242195913 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 156102.79018349186,
            "unit": "ns/iter",
            "extra": "iterations: 4523\ncpu: 156092.72009728063 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 107101.3735456145,
            "unit": "ns/iter",
            "extra": "iterations: 6532\ncpu: 107094.59522351493 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 57463293.49999541,
            "unit": "ns/iter",
            "extra": "iterations: 10\ncpu: 57457056.799999855 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 19903.02375385187,
            "unit": "ns/iter",
            "extra": "iterations: 35068\ncpu: 19902.390127751773 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 174124.1503487878,
            "unit": "ns/iter",
            "extra": "iterations: 4004\ncpu: 173978.53221775775 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "7576360808d16632847c1d02a9282531baf24143",
          "message": "Remove Codacy badge from README\n\nRemoved Codacy badge from README.md",
          "timestamp": "2026-06-03T13:23:40+02:00",
          "tree_id": "d556d8a309063e3b746c48e3a2a4d8351f9418ec",
          "url": "https://github.com/najaeda/naja/commit/7576360808d16632847c1d02a9282531baf24143"
        },
        "date": 1780485962763,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 65819.69705884489,
            "unit": "ns/iter",
            "extra": "iterations: 10540\ncpu: 65818.26034155599 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 165561.66241735657,
            "unit": "ns/iter",
            "extra": "iterations: 4236\ncpu: 165532.49834749763 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1638830.6674527165,
            "unit": "ns/iter",
            "extra": "iterations: 424\ncpu: 1638730.3396226414 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17316708.07499768,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17316250.575000003 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1493.5113322325385,
            "unit": "ns/iter",
            "extra": "iterations: 469325\ncpu: 1493.4764566132208 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14956.763678993679,
            "unit": "ns/iter",
            "extra": "iterations: 46915\ncpu: 14956.043845252052 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 206983.54161950067,
            "unit": "ns/iter",
            "extra": "iterations: 3532\ncpu: 206980.3550396375 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 293139.078562507,
            "unit": "ns/iter",
            "extra": "iterations: 2393\ncpu: 293118.12118679495 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 241254.23177442068,
            "unit": "ns/iter",
            "extra": "iterations: 2908\ncpu: 241242.31189821186 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 170549.4035884853,
            "unit": "ns/iter",
            "extra": "iterations: 4180\ncpu: 170540.63444976063 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 73294902.12498513,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 73068958.12499991 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24017.222979915907,
            "unit": "ns/iter",
            "extra": "iterations: 29182\ncpu: 24016.45510931392 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 231533.46211837113,
            "unit": "ns/iter",
            "extra": "iterations: 3049\ncpu: 230555.940964238 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "e591f54bfe52d019e5790fd374a21a9cf7379440",
          "message": "run megaboom regress on docker image (#381)",
          "timestamp": "2026-06-04T08:42:03+02:00",
          "tree_id": "019bc7cd91be5356472b139b8b33ae613a27086a",
          "url": "https://github.com/najaeda/naja/commit/e591f54bfe52d019e5790fd374a21a9cf7379440"
        },
        "date": 1780555469498,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 59651.79185172498,
            "unit": "ns/iter",
            "extra": "iterations: 11708\ncpu: 59649.48556542536 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 162587.87027954744,
            "unit": "ns/iter",
            "extra": "iterations: 4078\ncpu: 162578.20769985282 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1584511.9954955175,
            "unit": "ns/iter",
            "extra": "iterations: 444\ncpu: 1584153.4234234234 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17505475.92500027,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17481799.050000004 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1605.6964400256627,
            "unit": "ns/iter",
            "extra": "iterations: 436520\ncpu: 1605.566860624943 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 15991.551091920288,
            "unit": "ns/iter",
            "extra": "iterations: 44234\ncpu: 15990.090021250613 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 171502.60414129094,
            "unit": "ns/iter",
            "extra": "iterations: 4105\ncpu: 171494.43775883075 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 249894.59479686336,
            "unit": "ns/iter",
            "extra": "iterations: 2806\ncpu: 249868.611903065 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 205004.72291730394,
            "unit": "ns/iter",
            "extra": "iterations: 3277\ncpu: 204988.5950564541 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 145210.96969696757,
            "unit": "ns/iter",
            "extra": "iterations: 4818\ncpu: 145203.6307596514 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 72714737.12499699,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 72714247.12499997 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 25960.182537324086,
            "unit": "ns/iter",
            "extra": "iterations: 26926\ncpu: 25959.038067295554 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 224432.7730175034,
            "unit": "ns/iter",
            "extra": "iterations: 3128\ncpu: 224384.39578003366 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "eaba93f5efe0fc2f57d39466c45855ef9495a298",
          "message": "disable some parts of the workflows in forks when they can only work … (#382)\n\n* disable some parts of the workflows in forks when they can only work in central repo\n\n* protect forks against non runnable runs (upload package, ...)\n\n* add cva6 sim\n\n* clean regress\n\n* missing package\n\n* add local sim\n\n* add authorization\n\n* update sv regress\n\n* remove cva6 for the moment\n\n* add boost",
          "timestamp": "2026-06-05T12:16:08+02:00",
          "tree_id": "7b46b1614e7708772bde258c98f60553a963af87",
          "url": "https://github.com/najaeda/naja/commit/eaba93f5efe0fc2f57d39466c45855ef9495a298"
        },
        "date": 1780654717133,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 63937.58602150415,
            "unit": "ns/iter",
            "extra": "iterations: 11346\ncpu: 63925.08487572714 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 162234.6332331864,
            "unit": "ns/iter",
            "extra": "iterations: 4327\ncpu: 162199.4118326785 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1617285.693023246,
            "unit": "ns/iter",
            "extra": "iterations: 430\ncpu: 1616977.9953488375 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 19930717.60526277,
            "unit": "ns/iter",
            "extra": "iterations: 38\ncpu: 19929690.605263166 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1634.9100018599058,
            "unit": "ns/iter",
            "extra": "iterations: 430120\ncpu: 1634.8492583465072 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 16053.053307053884,
            "unit": "ns/iter",
            "extra": "iterations: 43634\ncpu: 16051.93523399184 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 171883.2315412175,
            "unit": "ns/iter",
            "extra": "iterations: 4185\ncpu: 171868.3545997612 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 250661.20490621196,
            "unit": "ns/iter",
            "extra": "iterations: 2772\ncpu: 250639.4942279944 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 206491.4841199164,
            "unit": "ns/iter",
            "extra": "iterations: 3369\ncpu: 206476.2428020186 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 146916.18685702232,
            "unit": "ns/iter",
            "extra": "iterations: 4763\ncpu: 146809.78479949638 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 73717483.12499803,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 73710155.99999997 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 25619.593191964337,
            "unit": "ns/iter",
            "extra": "iterations: 26880\ncpu: 25618.413988095264 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 225662.84896004616,
            "unit": "ns/iter",
            "extra": "iterations: 3125\ncpu: 225580.7552000175 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "122ec5103f815f248a3179a52515fcf357898823",
          "message": "primitive FFs and Latches (#383)\n\n* primitive FFs and Latches\n\n* fix tests\n\n* reorganize db0\n\n* clean test\n\n* coverage",
          "timestamp": "2026-06-05T16:49:36+02:00",
          "tree_id": "bf5930873791a19ade9b4137b5790a78d5aa758b",
          "url": "https://github.com/najaeda/naja/commit/122ec5103f815f248a3179a52515fcf357898823"
        },
        "date": 1780671128091,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 103216.31660976334,
            "unit": "ns/iter",
            "extra": "iterations: 6737\ncpu: 103203.6302508535 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 210414.06988601582,
            "unit": "ns/iter",
            "extra": "iterations: 3334\ncpu: 210385.58728254348 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1700947.0782396535,
            "unit": "ns/iter",
            "extra": "iterations: 409\ncpu: 1700855.2396088017 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 19748242.97222237,
            "unit": "ns/iter",
            "extra": "iterations: 36\ncpu: 19747350.30555556 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1627.9543117301719,
            "unit": "ns/iter",
            "extra": "iterations: 436217\ncpu: 1627.8033341204036 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 15972.83524711744,
            "unit": "ns/iter",
            "extra": "iterations: 43805\ncpu: 15972.704029220416 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 170639.73168908985,
            "unit": "ns/iter",
            "extra": "iterations: 4014\ncpu: 170639.7097658197 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 297770.41972186934,
            "unit": "ns/iter",
            "extra": "iterations: 2373\ncpu: 297776.4707121784 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 251960.64549105373,
            "unit": "ns/iter",
            "extra": "iterations: 2739\ncpu: 251959.49726177458 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 190890.78401312948,
            "unit": "ns/iter",
            "extra": "iterations: 3653\ncpu: 190892.1226389271 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 73730052.87499979,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 73721957.00000006 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 25660.032514052727,
            "unit": "ns/iter",
            "extra": "iterations: 27219\ncpu: 25660.47121496013 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 261905.02442731237,
            "unit": "ns/iter",
            "extra": "iterations: 2661\ncpu: 261810.26907180325 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "c998ec4df1cccf0dfd5c76776ec5d38670bddd1e",
          "message": "optimize cva6 regress (#384)",
          "timestamp": "2026-06-07T12:41:01+02:00",
          "tree_id": "5543380799ac32bc21eab108c0faaa644b075fa9",
          "url": "https://github.com/najaeda/naja/commit/c998ec4df1cccf0dfd5c76776ec5d38670bddd1e"
        },
        "date": 1780829007290,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 101326.1105020221,
            "unit": "ns/iter",
            "extra": "iterations: 6932\ncpu: 101303.32775533758 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 203668.20985506583,
            "unit": "ns/iter",
            "extra": "iterations: 3450\ncpu: 203543.9611594203 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1680933.4463007313,
            "unit": "ns/iter",
            "extra": "iterations: 419\ncpu: 1680688.0095465397 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18998628.8648648,
            "unit": "ns/iter",
            "extra": "iterations: 37\ncpu: 18997295.837837838 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1610.2897355099633,
            "unit": "ns/iter",
            "extra": "iterations: 435404\ncpu: 1610.2525447630246 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 16320.242798353489,
            "unit": "ns/iter",
            "extra": "iterations: 44226\ncpu: 16319.903314792211 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 169576.6309148254,
            "unit": "ns/iter",
            "extra": "iterations: 4121\ncpu: 169572.5535064306 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 297572.224152553,
            "unit": "ns/iter",
            "extra": "iterations: 2360\ncpu: 297558.84491525416 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 253597.17555313668,
            "unit": "ns/iter",
            "extra": "iterations: 2757\ncpu: 253579.92564381612 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 190491.6150068346,
            "unit": "ns/iter",
            "extra": "iterations: 3665\ncpu: 190482.42237380633 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 75159254.87500396,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 75155382.12499994 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 25651.623926919103,
            "unit": "ns/iter",
            "extra": "iterations: 27258\ncpu: 25650.01383080195 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 262737.33545718365,
            "unit": "ns/iter",
            "extra": "iterations: 2668\ncpu: 262638.4047975958 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "3a42cb4e3ecd872ae924e9920732a5dc64368e5b",
          "message": "compress registers - switch to 0.7.0 version (#385)",
          "timestamp": "2026-06-08T15:00:22+02:00",
          "tree_id": "f098713e1b828a200b79c84850ddc97166cd4589",
          "url": "https://github.com/najaeda/naja/commit/3a42cb4e3ecd872ae924e9920732a5dc64368e5b"
        },
        "date": 1780923784703,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 106270.4411983924,
            "unit": "ns/iter",
            "extra": "iterations: 6709\ncpu: 106212.71903413326 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 214272.46774192736,
            "unit": "ns/iter",
            "extra": "iterations: 3286\ncpu: 214269.2072428485 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1756650.5714284517,
            "unit": "ns/iter",
            "extra": "iterations: 399\ncpu: 1756649.2506265664 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 19002526.055557054,
            "unit": "ns/iter",
            "extra": "iterations: 36\ncpu: 19001972.22222223 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1495.1361539618217,
            "unit": "ns/iter",
            "extra": "iterations: 467390\ncpu: 1494.5212841524212 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14728.747530813032,
            "unit": "ns/iter",
            "extra": "iterations: 47384\ncpu: 14725.983791997294 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 200968.40126655647,
            "unit": "ns/iter",
            "extra": "iterations: 3474\ncpu: 200949.0374208407 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 336892.6458733549,
            "unit": "ns/iter",
            "extra": "iterations: 2084\ncpu: 336845.6684261034 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 290130.7712765723,
            "unit": "ns/iter",
            "extra": "iterations: 2444\ncpu: 290088.7565466447 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 211722.64426276943,
            "unit": "ns/iter",
            "extra": "iterations: 3303\ncpu: 211697.86527399332 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 73369667.5000033,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 73363046.37500013 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24041.81188119002,
            "unit": "ns/iter",
            "extra": "iterations: 28785\ncpu: 24037.873406288014 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 272368.869005806,
            "unit": "ns/iter",
            "extra": "iterations: 2565\ncpu: 272357.09239768254 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "71bfd75d36570d5730c705229967aeb7647f66cc",
          "message": "package as a bundle (#386)\n\n* package as a bundle\n\n* fix path\n\n* verifications and cleaning\n\n* some cleaning\n\n* Fix\n\n* new cleaning",
          "timestamp": "2026-06-12T12:19:56+02:00",
          "tree_id": "552ad0bee1d47fb00593daa79693430e31446814",
          "url": "https://github.com/najaeda/naja/commit/71bfd75d36570d5730c705229967aeb7647f66cc"
        },
        "date": 1781259768052,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 105443.60389706724,
            "unit": "ns/iter",
            "extra": "iterations: 6723\ncpu: 105435.84917447568 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 205886.99143783952,
            "unit": "ns/iter",
            "extra": "iterations: 3387\ncpu: 205875.8727487452 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1725150.6938272158,
            "unit": "ns/iter",
            "extra": "iterations: 405\ncpu: 1725086.1580246913 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17708660.449999057,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17708202.249999993 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1495.4904743190511,
            "unit": "ns/iter",
            "extra": "iterations: 468208\ncpu: 1495.4105333526982 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14614.530029873024,
            "unit": "ns/iter",
            "extra": "iterations: 47869\ncpu: 14613.61054126889 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 206558.4345612515,
            "unit": "ns/iter",
            "extra": "iterations: 3339\ncpu: 206544.38993710701 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 333310.6010485989,
            "unit": "ns/iter",
            "extra": "iterations: 2098\ncpu: 333280.6310772164 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 284007.37581170164,
            "unit": "ns/iter",
            "extra": "iterations: 2464\ncpu: 283979.4115259744 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 209685.6974789808,
            "unit": "ns/iter",
            "extra": "iterations: 3332\ncpu: 209672.78151260514 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 72250374.12499802,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 72242392.62499999 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24281.622970117187,
            "unit": "ns/iter",
            "extra": "iterations: 28881\ncpu: 24279.625947854984 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 266055.73038834735,
            "unit": "ns/iter",
            "extra": "iterations: 2626\ncpu: 265959.9485910406 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "e09c942c2c8a24406bd4f0ba949220a82a5f9fc4",
          "message": "Log cleaning (#387)",
          "timestamp": "2026-06-13T08:07:03+02:00",
          "tree_id": "a4f67d6ba6074ae331f894bd9b8ddce2c03c0335",
          "url": "https://github.com/najaeda/naja/commit/e09c942c2c8a24406bd4f0ba949220a82a5f9fc4"
        },
        "date": 1781330976891,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 102215.39283625752,
            "unit": "ns/iter",
            "extra": "iterations: 6840\ncpu: 102209.68435672516 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 205233.23990637698,
            "unit": "ns/iter",
            "extra": "iterations: 3418\ncpu: 205214.48303101235 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1669385.070388357,
            "unit": "ns/iter",
            "extra": "iterations: 412\ncpu: 1669258.2791262127 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18447750.499999557,
            "unit": "ns/iter",
            "extra": "iterations: 38\ncpu: 18445404.02631578 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1609.7697850394186,
            "unit": "ns/iter",
            "extra": "iterations: 431614\ncpu: 1609.508454313346 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 15840.817178024785,
            "unit": "ns/iter",
            "extra": "iterations: 44196\ncpu: 15835.777604308072 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 169964.80919426825,
            "unit": "ns/iter",
            "extra": "iterations: 4046\ncpu: 169930.25160652498 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 292459.74070981244,
            "unit": "ns/iter",
            "extra": "iterations: 2395\ncpu: 292419.99248434225 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 250813.52712049257,
            "unit": "ns/iter",
            "extra": "iterations: 2747\ncpu: 250793.5547870408 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 189070.80735930585,
            "unit": "ns/iter",
            "extra": "iterations: 3696\ncpu: 189052.56331168834 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 74608974.3749998,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 74610041.99999999 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 25818.194519036024,
            "unit": "ns/iter",
            "extra": "iterations: 27185\ncpu: 25815.530513150603 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 264740.62386706774,
            "unit": "ns/iter",
            "extra": "iterations: 2648\ncpu: 264650.8674470895 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "7a6775acacdded3892cb0628d4cb157074173e0e",
          "message": "Liberty filtering (#388)",
          "timestamp": "2026-06-13T11:14:11+02:00",
          "tree_id": "2fa4a6a1e6264ec93fe2935c98c1827f7aca43e3",
          "url": "https://github.com/najaeda/naja/commit/7a6775acacdded3892cb0628d4cb157074173e0e"
        },
        "date": 1781342186691,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 104100.77748650937,
            "unit": "ns/iter",
            "extra": "iterations: 6485\ncpu: 104084.47124132614 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 207708.4802962485,
            "unit": "ns/iter",
            "extra": "iterations: 3375\ncpu: 207665.48681481488 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1745754.321782082,
            "unit": "ns/iter",
            "extra": "iterations: 404\ncpu: 1743277.8638613871 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18001762.12820285,
            "unit": "ns/iter",
            "extra": "iterations: 39\ncpu: 18001490.307692308 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1500.7030678155277,
            "unit": "ns/iter",
            "extra": "iterations: 478712\ncpu: 1500.6601359481263 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14607.765352908094,
            "unit": "ns/iter",
            "extra": "iterations: 48639\ncpu: 14606.225518616746 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 204653.49264703714,
            "unit": "ns/iter",
            "extra": "iterations: 3536\ncpu: 204634.3885746608 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 335571.278618688,
            "unit": "ns/iter",
            "extra": "iterations: 2114\ncpu: 335569.29328287597 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 294091.53758923896,
            "unit": "ns/iter",
            "extra": "iterations: 2381\ncpu: 294083.27005459863 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 213828.99726692637,
            "unit": "ns/iter",
            "extra": "iterations: 3293\ncpu: 213827.7102945638 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 69895318.3749893,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 69891582.25000013 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24156.83827657623,
            "unit": "ns/iter",
            "extra": "iterations: 29569\ncpu: 24156.056951537103 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 270505.2868672522,
            "unit": "ns/iter",
            "extra": "iterations: 2604\ncpu: 270411.6524577748 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "a78da55e85263eb2391e01def2f5cc9629854ee9",
          "message": "rework RTL source infos (#389)\n\n* rework RTL source infos\n\n* coverage\n\n* fix git-hash information.\n\n* Clean Logic Optimization logging\n\n* apply all optim in megaboom regress run and add infos in naja_edit log\n\n* fix for docker build wf\n\n* fix for log\n\n* cleaning\n\n* improve instance optmization count logging\n\n* coverage",
          "timestamp": "2026-06-15T17:52:01+02:00",
          "tree_id": "f51f3d10f07176fcdc18967c78840735370bed7e",
          "url": "https://github.com/najaeda/naja/commit/a78da55e85263eb2391e01def2f5cc9629854ee9"
        },
        "date": 1781538867975,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 101024.62666268313,
            "unit": "ns/iter",
            "extra": "iterations: 6691\ncpu: 101016.97877746227 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 200538.54368083333,
            "unit": "ns/iter",
            "extra": "iterations: 3434\ncpu: 200494.3328479907 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1674873.5268292627,
            "unit": "ns/iter",
            "extra": "iterations: 410\ncpu: 1674542.9292682935 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18993103.8055554,
            "unit": "ns/iter",
            "extra": "iterations: 36\ncpu: 18992095.361111104 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1622.9870345845259,
            "unit": "ns/iter",
            "extra": "iterations: 431147\ncpu: 1622.9025459993911 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 15981.22460272073,
            "unit": "ns/iter",
            "extra": "iterations: 43735\ncpu: 15981.034091688576 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 171581.58873720645,
            "unit": "ns/iter",
            "extra": "iterations: 4102\ncpu: 171564.0285226717 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 294420.64258714573,
            "unit": "ns/iter",
            "extra": "iterations: 2381\ncpu: 294413.48635027296 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 260097.17311828106,
            "unit": "ns/iter",
            "extra": "iterations: 2790\ncpu: 260070.59892473114 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 190071.5036674828,
            "unit": "ns/iter",
            "extra": "iterations: 3681\ncpu: 190062.4479760933 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 75168352.3749982,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 75156383.49999999 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 26036.181620649702,
            "unit": "ns/iter",
            "extra": "iterations: 26693\ncpu: 26034.82103922375 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 265547.6550151457,
            "unit": "ns/iter",
            "extra": "iterations: 2632\ncpu: 265417.3685410217 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "8a038d7c32d26438178f2cfae4459f34a65c32c7",
          "message": "wrap source access in Python (#390)\n\n* wrap source access in Python\n\n* coverage\n\n* fix",
          "timestamp": "2026-06-15T22:25:02+02:00",
          "tree_id": "8e30e68816127cb1ab910efd6ce0ebfbfffe311e",
          "url": "https://github.com/najaeda/naja/commit/8a038d7c32d26438178f2cfae4459f34a65c32c7"
        },
        "date": 1781555745511,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 96679.94049986177,
            "unit": "ns/iter",
            "extra": "iterations: 7042\ncpu: 96655.04231752343 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 192891.45023435127,
            "unit": "ns/iter",
            "extra": "iterations: 3627\ncpu: 192849.2594430659 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1650230.6188234638,
            "unit": "ns/iter",
            "extra": "iterations: 425\ncpu: 1649732.8541176473 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18317645.368420854,
            "unit": "ns/iter",
            "extra": "iterations: 38\ncpu: 18315360.02631579 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1629.1025870006945,
            "unit": "ns/iter",
            "extra": "iterations: 431117\ncpu: 1627.7575646518228 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 15963.848188206783,
            "unit": "ns/iter",
            "extra": "iterations: 43824\ncpu: 15962.340680905445 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 167789.4245306553,
            "unit": "ns/iter",
            "extra": "iterations: 3995\ncpu: 167778.7591989989 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 295991.71313899424,
            "unit": "ns/iter",
            "extra": "iterations: 2367\ncpu: 295990.8926911704 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 253978.83430231683,
            "unit": "ns/iter",
            "extra": "iterations: 2752\ncpu: 253971.57085755837 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 192992.68569065162,
            "unit": "ns/iter",
            "extra": "iterations: 3627\ncpu: 192992.17038875114 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 75218120.62500288,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 75213946.87500016 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 26187.769502346255,
            "unit": "ns/iter",
            "extra": "iterations: 26625\ncpu: 26185.535661971844 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 261855.5537320142,
            "unit": "ns/iter",
            "extra": "iterations: 2680\ncpu: 261712.63955223956 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "b4f66dd0f7625bca763dff567805d32dd7b31950",
          "message": "B0.7.3 (#391)",
          "timestamp": "2026-06-18T01:03:28+02:00",
          "tree_id": "6e4bbdfda63aafb4ba8e3a239693e6e0b40ca46a",
          "url": "https://github.com/najaeda/naja/commit/b4f66dd0f7625bca763dff567805d32dd7b31950"
        },
        "date": 1781737546572,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 118143.24519229967,
            "unit": "ns/iter",
            "extra": "iterations: 6032\ncpu: 118123.41826923077 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 218335.23815464156,
            "unit": "ns/iter",
            "extra": "iterations: 3208\ncpu: 218287.02961346638 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1716822.9874999952,
            "unit": "ns/iter",
            "extra": "iterations: 400\ncpu: 1716412.4275000002 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17621050.34999877,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17619641.025 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1510.0958500237987,
            "unit": "ns/iter",
            "extra": "iterations: 464340\ncpu: 1510.0856139897494 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14833.751594380052,
            "unit": "ns/iter",
            "extra": "iterations: 47197\ncpu: 14833.360489014134 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 203669.44275256136,
            "unit": "ns/iter",
            "extra": "iterations: 3415\ncpu: 203669.6412884333 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 350422.0961923796,
            "unit": "ns/iter",
            "extra": "iterations: 1996\ncpu: 350403.7419839677 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 298998.580976858,
            "unit": "ns/iter",
            "extra": "iterations: 2334\ncpu: 298988.3423307629 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 224473.55861625852,
            "unit": "ns/iter",
            "extra": "iterations: 3122\ncpu: 224462.94042280584 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 72426478.49999173,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 72425517.74999994 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 25445.159463649838,
            "unit": "ns/iter",
            "extra": "iterations: 27743\ncpu: 25444.327794398607 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 282085.2756860877,
            "unit": "ns/iter",
            "extra": "iterations: 2492\ncpu: 281663.5397271177 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "7d9ccd4dcaf10673f3866b460b0a61a676b6be8c",
          "message": "B0.7.4 (#392)",
          "timestamp": "2026-06-20T08:12:43+02:00",
          "tree_id": "2b9370c5d34dc457b729e8da11ea41698917c12e",
          "url": "https://github.com/najaeda/naja/commit/7d9ccd4dcaf10673f3866b460b0a61a676b6be8c"
        },
        "date": 1781936108985,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 118457.1429054759,
            "unit": "ns/iter",
            "extra": "iterations: 5913\ncpu: 118456.29223744292 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 223649.44302179298,
            "unit": "ns/iter",
            "extra": "iterations: 3124\ncpu: 223618.98303457105 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1780152.2115867152,
            "unit": "ns/iter",
            "extra": "iterations: 397\ncpu: 1779695.3652392945 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18866606.86486229,
            "unit": "ns/iter",
            "extra": "iterations: 37\ncpu: 18865753.243243244 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1481.5570520682388,
            "unit": "ns/iter",
            "extra": "iterations: 472209\ncpu: 1481.34549955634 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14518.33789574467,
            "unit": "ns/iter",
            "extra": "iterations: 48074\ncpu: 14516.0373174689 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 208554.69284626446,
            "unit": "ns/iter",
            "extra": "iterations: 3285\ncpu: 208517.2340943683 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 372105.18394469906,
            "unit": "ns/iter",
            "extra": "iterations: 1881\ncpu: 372080.58639021806 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 319837.7877401835,
            "unit": "ns/iter",
            "extra": "iterations: 2186\ncpu: 319818.3824336687 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 244750.98243145237,
            "unit": "ns/iter",
            "extra": "iterations: 2846\ncpu: 244743.33591004898 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 73222550.50000592,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 73215587.12499999 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 22242.9000222949,
            "unit": "ns/iter",
            "extra": "iterations: 31397\ncpu: 22242.604803006616 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 289158.068438403,
            "unit": "ns/iter",
            "extra": "iterations: 2411\ncpu: 288878.2401492927 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "90b4128a092f6cbab9f670a5c32be81c935a91e4",
          "message": "adding logical cone (#393)",
          "timestamp": "2026-06-22T06:36:14+02:00",
          "tree_id": "014628bac7d2e0768b26ae532a87435726bdd16f",
          "url": "https://github.com/najaeda/naja/commit/90b4128a092f6cbab9f670a5c32be81c935a91e4"
        },
        "date": 1782103116053,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 113635.54460478348,
            "unit": "ns/iter",
            "extra": "iterations: 6098\ncpu: 113627.4609708101 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 216170.88529684063,
            "unit": "ns/iter",
            "extra": "iterations: 3217\ncpu: 216133.44762200807 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1627625.8503561546,
            "unit": "ns/iter",
            "extra": "iterations: 421\ncpu: 1627277.2636579573 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18099025.410256565,
            "unit": "ns/iter",
            "extra": "iterations: 39\ncpu: 18098391.12820514 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1597.151210298273,
            "unit": "ns/iter",
            "extra": "iterations: 438363\ncpu: 1597.090536838192 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 15724.76112196597,
            "unit": "ns/iter",
            "extra": "iterations: 44529\ncpu: 15723.85793527813 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 167521.6707434088,
            "unit": "ns/iter",
            "extra": "iterations: 4170\ncpu: 167511.05947242194 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 315375.5516605377,
            "unit": "ns/iter",
            "extra": "iterations: 2168\ncpu: 315360.2200184502 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 271020.44237482164,
            "unit": "ns/iter",
            "extra": "iterations: 2577\ncpu: 271016.2169188981 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 207256.4371115954,
            "unit": "ns/iter",
            "extra": "iterations: 3379\ncpu: 207253.46285883366 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 74272796.00000247,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 74268445.5 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 23002.30610607044,
            "unit": "ns/iter",
            "extra": "iterations: 28660\ncpu: 23000.932728541502 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 275943.00235617073,
            "unit": "ns/iter",
            "extra": "iterations: 2546\ncpu: 275772.88727412873 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "7f5984ba6267bfbcc2a40f15192bc7173e740216",
          "message": "Add Combinatorial Dependencies to NLDB0 and, or, xor, ... gates (#394)\n\n* Add Combinatorial Dependencies to NLDB0 and, or, xor, ... gates\n\n* Add API for Instance Occurrence\n\n* add getAssign/NonAssign Instances",
          "timestamp": "2026-06-23T08:54:38+02:00",
          "tree_id": "9116f8d5dcc6f61ceb51ab8121f1e4d8f78bfe7e",
          "url": "https://github.com/najaeda/naja/commit/7f5984ba6267bfbcc2a40f15192bc7173e740216"
        },
        "date": 1782197830217,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 121501.80359845051,
            "unit": "ns/iter",
            "extra": "iterations: 5947\ncpu: 121469.36119051624 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 221364.6444518464,
            "unit": "ns/iter",
            "extra": "iterations: 3001\ncpu: 221298.0866377874 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1720255.65441175,
            "unit": "ns/iter",
            "extra": "iterations: 408\ncpu: 1719719.2794117648 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18430534.05405413,
            "unit": "ns/iter",
            "extra": "iterations: 37\ncpu: 18430612.29729729 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1465.6350113034637,
            "unit": "ns/iter",
            "extra": "iterations: 480382\ncpu: 1465.6100062033963 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14704.237978080864,
            "unit": "ns/iter",
            "extra": "iterations: 49181\ncpu: 14701.968016103769 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 211869.51919132532,
            "unit": "ns/iter",
            "extra": "iterations: 3413\ncpu: 211866.35159683545 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 378142.71273712465,
            "unit": "ns/iter",
            "extra": "iterations: 1845\ncpu: 378138.0504065041 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 324982.9147358612,
            "unit": "ns/iter",
            "extra": "iterations: 2158\ncpu: 324973.85078776634 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 245737.8543348583,
            "unit": "ns/iter",
            "extra": "iterations: 2849\ncpu: 245739.13162513153 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 73741011.99999927,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 73735988.37499996 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 21827.623973418038,
            "unit": "ns/iter",
            "extra": "iterations: 31902\ncpu: 21827.367469124183 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 288331.761553432,
            "unit": "ns/iter",
            "extra": "iterations: 2445\ncpu: 287467.06912064657 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "017a1c852a0d2630340619e932bb1b777eafe502",
          "message": "B0.7.7 (#395)",
          "timestamp": "2026-06-25T07:51:28+02:00",
          "tree_id": "ef5025a66baf9cf5f68e085c19009d42b010b25a",
          "url": "https://github.com/najaeda/naja/commit/017a1c852a0d2630340619e932bb1b777eafe502"
        },
        "date": 1782366887623,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 118458.70472642058,
            "unit": "ns/iter",
            "extra": "iterations: 5903\ncpu: 118452.71048619346 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 227254.00388225523,
            "unit": "ns/iter",
            "extra": "iterations: 3091\ncpu: 227245.77418311225 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1784997.5113351035,
            "unit": "ns/iter",
            "extra": "iterations: 397\ncpu: 1784959.8765743072 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18349257.66666768,
            "unit": "ns/iter",
            "extra": "iterations: 36\ncpu: 18346957.944444444 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1469.534564834914,
            "unit": "ns/iter",
            "extra": "iterations: 477899\ncpu: 1469.4587642995696 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14368.977432656002,
            "unit": "ns/iter",
            "extra": "iterations: 48743\ncpu: 14368.70461399585 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 216969.36664605376,
            "unit": "ns/iter",
            "extra": "iterations: 3232\ncpu: 216952.6491336634 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 382053.12459015416,
            "unit": "ns/iter",
            "extra": "iterations: 1830\ncpu: 382044.38797814184 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 327430.9200560714,
            "unit": "ns/iter",
            "extra": "iterations: 2139\ncpu: 327403.9289387566 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 247035.89222613454,
            "unit": "ns/iter",
            "extra": "iterations: 2830\ncpu: 247027.7819787985 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 73953594.62499584,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 73717765.12499984 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 21760.017088726836,
            "unit": "ns/iter",
            "extra": "iterations: 32302\ncpu: 21759.428487400157 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 290194.178438647,
            "unit": "ns/iter",
            "extra": "iterations: 2421\ncpu: 289984.1404378221 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "883b1c36f696fa7fa1e1e71123d8a0d8039668ae",
          "message": "B0.7.8 (#396)\n\n* initiate slang <-> naja link in memory\n\n* adding SV Intent\n\n* SVIntent evolution\n\n* coverage\n\n* more coverage",
          "timestamp": "2026-06-28T20:29:17+02:00",
          "tree_id": "62b134b2ac1ff4d0fe22bf28f71a9144157b59d4",
          "url": "https://github.com/najaeda/naja/commit/883b1c36f696fa7fa1e1e71123d8a0d8039668ae"
        },
        "date": 1782671497787,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 121993.99209758546,
            "unit": "ns/iter",
            "extra": "iterations: 5821\ncpu: 121988.35234495791 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 224380.9062600378,
            "unit": "ns/iter",
            "extra": "iterations: 3115\ncpu: 224381.3540930979 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1776848.111662431,
            "unit": "ns/iter",
            "extra": "iterations: 403\ncpu: 1776783.516129031 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18759458.351352092,
            "unit": "ns/iter",
            "extra": "iterations: 37\ncpu: 18759209.216216218 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1454.6673053187644,
            "unit": "ns/iter",
            "extra": "iterations: 479656\ncpu: 1454.5693913971681 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14324.257105495059,
            "unit": "ns/iter",
            "extra": "iterations: 48941\ncpu: 14322.61578226844 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 212570.36283184847,
            "unit": "ns/iter",
            "extra": "iterations: 3277\ncpu: 212563.15471467804 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 377773.98216213763,
            "unit": "ns/iter",
            "extra": "iterations: 1850\ncpu: 377744.53027027036 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 324243.7777262207,
            "unit": "ns/iter",
            "extra": "iterations: 2155\ncpu: 324236.9243619488 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 245870.59261861778,
            "unit": "ns/iter",
            "extra": "iterations: 2845\ncpu: 245862.62917398912 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 74134077.87500148,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 74129369.62500006 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 21888.575472875775,
            "unit": "ns/iter",
            "extra": "iterations: 32038\ncpu: 21887.548598539248 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 293000.08257664496,
            "unit": "ns/iter",
            "extra": "iterations: 2422\ncpu: 290003.71180842107 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "4d6b8108c7d0680020c1d793372130a637d8c066",
          "message": "support and differentiate blocking and non blocking assignments (#397)\n\n* support and differentiate blocking and non blocking assignments\n\n* coverage",
          "timestamp": "2026-06-30T23:53:43+02:00",
          "tree_id": "183385a6f0423f77a3a592e22b1870704b15e65e",
          "url": "https://github.com/najaeda/naja/commit/4d6b8108c7d0680020c1d793372130a637d8c066"
        },
        "date": 1782856582314,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 120529.5641336369,
            "unit": "ns/iter",
            "extra": "iterations: 5777\ncpu: 120513.73584905663 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 222198.61289298406,
            "unit": "ns/iter",
            "extra": "iterations: 3149\ncpu: 222181.9437916799 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1745486.7885571818,
            "unit": "ns/iter",
            "extra": "iterations: 402\ncpu: 1745323.4975124379 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18014538.43589739,
            "unit": "ns/iter",
            "extra": "iterations: 39\ncpu: 18012850.846153844 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1461.6111016139973,
            "unit": "ns/iter",
            "extra": "iterations: 479678\ncpu: 1461.5138509583508 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14327.756650649337,
            "unit": "ns/iter",
            "extra": "iterations: 48905\ncpu: 14326.605684490336 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 211715.61899001693,
            "unit": "ns/iter",
            "extra": "iterations: 3307\ncpu: 211704.66495312986 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 382002.7814207677,
            "unit": "ns/iter",
            "extra": "iterations: 1830\ncpu: 381955.6863387977 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 325763.66542923526,
            "unit": "ns/iter",
            "extra": "iterations: 2155\ncpu: 325732.23248259857 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 245860.3442680733,
            "unit": "ns/iter",
            "extra": "iterations: 2835\ncpu: 245828.5421516759 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 74042212.87500334,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 74039339.12500004 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 21883.79281596706,
            "unit": "ns/iter",
            "extra": "iterations: 32266\ncpu: 21834.047108411327 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 292174.91577261646,
            "unit": "ns/iter",
            "extra": "iterations: 2422\ncpu: 291939.66969447327 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "noam.chn1@gmail.com",
            "name": "Noam Cohen",
            "username": "nanocoh"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "77cb2b1796b798572162fedda62539ef3054cc3c",
          "message": "DNL based logical cone compute api (#399)\n\n* DNL logic cone compute api",
          "timestamp": "2026-07-02T23:03:37+02:00",
          "tree_id": "2f36028ad3b0a2faf0f696f3e2af8b81a24ee1c2",
          "url": "https://github.com/najaeda/naja/commit/77cb2b1796b798572162fedda62539ef3054cc3c"
        },
        "date": 1783026391343,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 121570.65012148595,
            "unit": "ns/iter",
            "extra": "iterations: 5762\ncpu: 121559.24626865672 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 226529.31649484756,
            "unit": "ns/iter",
            "extra": "iterations: 2910\ncpu: 226529.91546391757 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1787286.9057072403,
            "unit": "ns/iter",
            "extra": "iterations: 403\ncpu: 1786862.3771712168 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18156086.56410206,
            "unit": "ns/iter",
            "extra": "iterations: 39\ncpu: 18155883.205128208 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1455.1289961546252,
            "unit": "ns/iter",
            "extra": "iterations: 479805\ncpu: 1454.8815539646316 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14291.01344798096,
            "unit": "ns/iter",
            "extra": "iterations: 49078\ncpu: 14290.09197603814 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 209985.55515371656,
            "unit": "ns/iter",
            "extra": "iterations: 3318\ncpu: 209968.1769138035 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 378301.90835141507,
            "unit": "ns/iter",
            "extra": "iterations: 1844\ncpu: 378297.8373101954 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 326667.57808090025,
            "unit": "ns/iter",
            "extra": "iterations: 2126\ncpu: 326617.76952022547 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 245920.35236082828,
            "unit": "ns/iter",
            "extra": "iterations: 2838\ncpu: 245913.72727272694 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 72377138.2500047,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 72372447.25000003 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 21792.42458690464,
            "unit": "ns/iter",
            "extra": "iterations: 32196\ncpu: 21791.918343893627 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 291252.56368977675,
            "unit": "ns/iter",
            "extra": "iterations: 2418\ncpu: 291092.7770885096 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "noam.chn1@gmail.com",
            "name": "Noam Cohen",
            "username": "nanocoh"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "618c6b54273caa3f6fd51372c80e6486af5213de",
          "message": "partial path support in logic cone (#400)",
          "timestamp": "2026-07-03T10:43:07+02:00",
          "tree_id": "9780b20ef6437ae57541e8b6252edc4ca6a10685",
          "url": "https://github.com/najaeda/naja/commit/618c6b54273caa3f6fd51372c80e6486af5213de"
        },
        "date": 1783068339688,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 120819.51145564571,
            "unit": "ns/iter",
            "extra": "iterations: 5805\ncpu: 120809.64892334197 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 224848.78225806105,
            "unit": "ns/iter",
            "extra": "iterations: 3100\ncpu: 224841.05516129037 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1795990.9407217586,
            "unit": "ns/iter",
            "extra": "iterations: 388\ncpu: 1795894.6546391752 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 19084760.31578899,
            "unit": "ns/iter",
            "extra": "iterations: 38\ncpu: 19084410.684210517 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1465.7872830746503,
            "unit": "ns/iter",
            "extra": "iterations: 476892\ncpu: 1465.454306216082 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14369.505669679513,
            "unit": "ns/iter",
            "extra": "iterations: 48680\ncpu: 14366.473705834016 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 201196.72573100246,
            "unit": "ns/iter",
            "extra": "iterations: 3420\ncpu: 201161.36842105267 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 378823.98807585996,
            "unit": "ns/iter",
            "extra": "iterations: 1845\ncpu: 378748.5040650407 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 326836.9330232396,
            "unit": "ns/iter",
            "extra": "iterations: 2150\ncpu: 326819.77860465116 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 246170.74196963894,
            "unit": "ns/iter",
            "extra": "iterations: 2833\ncpu: 246158.25697140794 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 76881935.25000031,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 76598766.00000004 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 21085.04729953402,
            "unit": "ns/iter",
            "extra": "iterations: 33235\ncpu: 21084.335248984462 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 290606.25251214165,
            "unit": "ns/iter",
            "extra": "iterations: 2392\ncpu: 290370.9782608661 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "532014c15d95b305837d421398f07e54586c453c",
          "message": "B0.7.9 (#398)",
          "timestamp": "2026-07-03T15:27:54+02:00",
          "tree_id": "ce0399a02032b4721aeb9a8e4e7039ddcdbbc2e2",
          "url": "https://github.com/najaeda/naja/commit/532014c15d95b305837d421398f07e54586c453c"
        },
        "date": 1783085436879,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 117208.79460758991,
            "unit": "ns/iter",
            "extra": "iterations: 5823\ncpu: 117194.18770393268 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 225280.75604254074,
            "unit": "ns/iter",
            "extra": "iterations: 3103\ncpu: 225227.34676119883 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1773250.1818181314,
            "unit": "ns/iter",
            "extra": "iterations: 407\ncpu: 1772735.230958231 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18300787.789473966,
            "unit": "ns/iter",
            "extra": "iterations: 38\ncpu: 18300307.63157894 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1468.6489565275724,
            "unit": "ns/iter",
            "extra": "iterations: 477636\ncpu: 1468.623152358699 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14384.452456467247,
            "unit": "ns/iter",
            "extra": "iterations: 48871\ncpu: 14383.98481717174 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 205416.3015301502,
            "unit": "ns/iter",
            "extra": "iterations: 3333\ncpu: 205407.1164116413 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 374550.1437733885,
            "unit": "ns/iter",
            "extra": "iterations: 1871\ncpu: 374538.6274719406 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 320947.56900504004,
            "unit": "ns/iter",
            "extra": "iterations: 2181\ncpu: 320919.6382393395 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 241863.6974376958,
            "unit": "ns/iter",
            "extra": "iterations: 2849\ncpu: 241844.57213057237 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 71697304.62500112,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 71689224.125 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 21353.155178226727,
            "unit": "ns/iter",
            "extra": "iterations: 32627\ncpu: 21351.805590461885 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 286387.8845365793,
            "unit": "ns/iter",
            "extra": "iterations: 2451\ncpu: 286212.6919624984 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "noam.chn1@gmail.com",
            "name": "Noam Cohen",
            "username": "nanocoh"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "63031addd9a4f20b1ba252050a75b1c2d13b321e",
          "message": "table select generic tt (#402)",
          "timestamp": "2026-07-05T18:52:00+02:00",
          "tree_id": "5b1f69a0bdca89b8ea12390a087cb9edf9f3d1a3",
          "url": "https://github.com/najaeda/naja/commit/63031addd9a4f20b1ba252050a75b1c2d13b321e"
        },
        "date": 1783270474241,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 125689.23242769488,
            "unit": "ns/iter",
            "extra": "iterations: 5705\ncpu: 125684.3745836985 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 229475.80158207015,
            "unit": "ns/iter",
            "extra": "iterations: 3034\ncpu: 229464.0830586684 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1774191.9130433549,
            "unit": "ns/iter",
            "extra": "iterations: 391\ncpu: 1774111.2020460356 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18159544.000001445,
            "unit": "ns/iter",
            "extra": "iterations: 37\ncpu: 18158378.243243236 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1473.8761442460573,
            "unit": "ns/iter",
            "extra": "iterations: 474439\ncpu: 1473.5032912555669 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14442.22161950334,
            "unit": "ns/iter",
            "extra": "iterations: 48484\ncpu: 14441.439113934486 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 208875.68876779155,
            "unit": "ns/iter",
            "extra": "iterations: 3303\ncpu: 208866.1283681501 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 384660.81373621564,
            "unit": "ns/iter",
            "extra": "iterations: 1820\ncpu: 384646.406593407 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 330123.33050844405,
            "unit": "ns/iter",
            "extra": "iterations: 2124\ncpu: 330110.8983050852 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 249315.16417376723,
            "unit": "ns/iter",
            "extra": "iterations: 2808\ncpu: 249300.69764957274 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 72355480.62500641,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 72352732.37500018 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 21053.424817915005,
            "unit": "ns/iter",
            "extra": "iterations: 33226\ncpu: 21052.74258111117 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 293261.64999947615,
            "unit": "ns/iter",
            "extra": "iterations: 2380\ncpu: 293028.4449579769 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "bac272e9b8bb6f2f032d4831f53a9304c10c7a3c",
          "message": "B0.7.10 (#401)\n\n* more memory inferring support\n\n* more SV support\n\n* adding new regress\n\n* new regress\n\n* fix test\n\n* memory inference coverage\n\n* new support\n\n* redesign SV warning messages\n\n* update tests\n\n* clean logging\n\n* more coverage\n\n* licensing",
          "timestamp": "2026-07-06T08:56:30+02:00",
          "tree_id": "fdf6e9977b2bae0087a3677b9560d73a69306de9",
          "url": "https://github.com/najaeda/naja/commit/bac272e9b8bb6f2f032d4831f53a9304c10c7a3c"
        },
        "date": 1783321131536,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 116924.97088691701,
            "unit": "ns/iter",
            "extra": "iterations: 5908\ncpu: 116911.42027758971 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 220736.41354342445,
            "unit": "ns/iter",
            "extra": "iterations: 3175\ncpu: 220674.06614173233 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1763654.802030702,
            "unit": "ns/iter",
            "extra": "iterations: 394\ncpu: 1763496.644670051 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18209908.921048015,
            "unit": "ns/iter",
            "extra": "iterations: 38\ncpu: 18207904.894736838 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1466.7634254209834,
            "unit": "ns/iter",
            "extra": "iterations: 477080\ncpu: 1466.2346189318353 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14442.677860050555,
            "unit": "ns/iter",
            "extra": "iterations: 48749\ncpu: 14438.91148536381 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 200810.22239162997,
            "unit": "ns/iter",
            "extra": "iterations: 3278\ncpu: 200797.71446003678 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 377898.2077502739,
            "unit": "ns/iter",
            "extra": "iterations: 1858\ncpu: 377881.5236813775 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 324740.8364312305,
            "unit": "ns/iter",
            "extra": "iterations: 2152\ncpu: 324725.1412639407 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 246205.22370736394,
            "unit": "ns/iter",
            "extra": "iterations: 2843\ncpu: 246194.3218431237 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 72873310.50002876,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 72866315.87500003 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 21248.090875951602,
            "unit": "ns/iter",
            "extra": "iterations: 32924\ncpu: 21246.98836714863 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 290524.59400105936,
            "unit": "ns/iter",
            "extra": "iterations: 2399\ncpu: 289542.05627346755 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "committer": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "distinct": true,
          "id": "dd0c818b2f0b0d785d931456835f9f2325b66125",
          "message": "naja FFs with INIT - defaulted to X",
          "timestamp": "2026-07-07T13:17:51+02:00",
          "tree_id": "292211f91128c16edc8b7f0fc7a5bfa1191e7bef",
          "url": "https://github.com/najaeda/naja/commit/dd0c818b2f0b0d785d931456835f9f2325b66125"
        },
        "date": 1783423214867,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 117884.2379257706,
            "unit": "ns/iter",
            "extra": "iterations: 5901\ncpu: 117846.35027961363 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 219663.6559575926,
            "unit": "ns/iter",
            "extra": "iterations: 3206\ncpu: 219606.50592638802 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1755309.8935643514,
            "unit": "ns/iter",
            "extra": "iterations: 404\ncpu: 1755212.4925742561 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17886119.794872146,
            "unit": "ns/iter",
            "extra": "iterations: 39\ncpu: 17885111.948717944 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1469.6305242546157,
            "unit": "ns/iter",
            "extra": "iterations: 476467\ncpu: 1469.5934282961873 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14397.356171697882,
            "unit": "ns/iter",
            "extra": "iterations: 48690\ncpu: 14394.628630108858 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 211593.48035713914,
            "unit": "ns/iter",
            "extra": "iterations: 3360\ncpu: 211557.87916666677 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 379410.00435492647,
            "unit": "ns/iter",
            "extra": "iterations: 1837\ncpu: 379402.63908546563 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 323632.1548774794,
            "unit": "ns/iter",
            "extra": "iterations: 2163\ncpu: 323626.2750809064 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 244637.89947644228,
            "unit": "ns/iter",
            "extra": "iterations: 2865\ncpu: 244629.4565445028 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 72907397.62500209,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 72903699.62499988 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 21272.219438209824,
            "unit": "ns/iter",
            "extra": "iterations: 32966\ncpu: 21271.78823636474 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 287329.1699344386,
            "unit": "ns/iter",
            "extra": "iterations: 2448\ncpu: 287079.696078434 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "b9d5443e875fa167ce5ab69d31298b5a7878c6ae",
          "message": "package najaeda sources (#404)",
          "timestamp": "2026-07-07T23:46:51+02:00",
          "tree_id": "6dc25416b8f18f7a6fd87d032321abaf27e2d721",
          "url": "https://github.com/najaeda/naja/commit/b9d5443e875fa167ce5ab69d31298b5a7878c6ae"
        },
        "date": 1783460953772,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 118525.65388513547,
            "unit": "ns/iter",
            "extra": "iterations: 5920\ncpu: 118517.65675675675 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 225543.60103627248,
            "unit": "ns/iter",
            "extra": "iterations: 3088\ncpu: 225540.32480569952 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1771662.7475247062,
            "unit": "ns/iter",
            "extra": "iterations: 404\ncpu: 1771614.54950495 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18142623.461538203,
            "unit": "ns/iter",
            "extra": "iterations: 39\ncpu: 18141871.435897443 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1476.4150851438285,
            "unit": "ns/iter",
            "extra": "iterations: 476018\ncpu: 1476.0882613682666 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14360.705965763254,
            "unit": "ns/iter",
            "extra": "iterations: 48661\ncpu: 14358.69176547954 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 211150.7578852455,
            "unit": "ns/iter",
            "extra": "iterations: 3329\ncpu: 211132.4701111444 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 375692.2397003719,
            "unit": "ns/iter",
            "extra": "iterations: 1869\ncpu: 375662.6490101661 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 322991.09879406594,
            "unit": "ns/iter",
            "extra": "iterations: 2156\ncpu: 322978.695269017 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 244051.2422554836,
            "unit": "ns/iter",
            "extra": "iterations: 2873\ncpu: 244024.25931082468 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 72474973.74999768,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 72467588.74999993 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 21358.78328201684,
            "unit": "ns/iter",
            "extra": "iterations: 32803\ncpu: 21358.171539188515 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 287378.5959810275,
            "unit": "ns/iter",
            "extra": "iterations: 2438\ncpu: 287217.95447090484 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "af7695a1326727cc6efd808f523291f6c0f9fc88",
          "message": "Allow unknown blackbox refactor (#403)\n\n* add allow_unknown_designs in SV frontend and refactor verilog option\n\n* add standard __version__\n\n* add support in SV\n\n* Add API for Term characterization on primitives, currently only used in NLDB0 prims, to extend later to liberty and python loading.\n\n* Fix Ibex top ports issue\n\n* add INIT parameter on FFs\n\n* accept FF initialization at declaration time\n\n* more testing and coverage\n\n* more coverage\n\n* more coverage\n\n* coverage",
          "timestamp": "2026-07-08T00:07:43+02:00",
          "tree_id": "a15ac061511b55d685319e5a81a270a738daac9f",
          "url": "https://github.com/najaeda/naja/commit/af7695a1326727cc6efd808f523291f6c0f9fc88"
        },
        "date": 1783462799862,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 115904.07238367095,
            "unit": "ns/iter",
            "extra": "iterations: 5609\ncpu: 115894.65769299342 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 217969.90433157393,
            "unit": "ns/iter",
            "extra": "iterations: 3209\ncpu: 217940.71019009038 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1710549.3698297301,
            "unit": "ns/iter",
            "extra": "iterations: 411\ncpu: 1710369.958637469 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18193268.71794858,
            "unit": "ns/iter",
            "extra": "iterations: 39\ncpu: 18192678.05128205 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1559.44453007722,
            "unit": "ns/iter",
            "extra": "iterations: 447648\ncpu: 1559.3961728858394 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 15396.577279022396,
            "unit": "ns/iter",
            "extra": "iterations: 45491\ncpu: 15396.248642588647 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 164022.632135308,
            "unit": "ns/iter",
            "extra": "iterations: 4257\ncpu: 164022.89687573386 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 335566.3426103648,
            "unit": "ns/iter",
            "extra": "iterations: 2084\ncpu: 335561.27015355066 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 288251.55879934266,
            "unit": "ns/iter",
            "extra": "iterations: 2432\ncpu: 288236.652138158 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 224847.7345302969,
            "unit": "ns/iter",
            "extra": "iterations: 3119\ncpu: 224842.12824623295 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 74408544.99999717,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 74404339.37500001 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 22881.51559081857,
            "unit": "ns/iter",
            "extra": "iterations: 30627\ncpu: 22879.493290234124 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 289291.3199348847,
            "unit": "ns/iter",
            "extra": "iterations: 2438\ncpu: 289090.4245282994 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "eb1dde036f924ecf3a181d73776bb5a9918509b3",
          "message": "B0.7.12 (#405)\n\n* prepare v0.7.12 version\n\n* rework documentation\n\n* description\n\n* clean hash functions in Py objects\n\n* has attributes\n\n* adding gate recognition primitives\n\n* new NLDB0 primitive\n\n* update testing\n\n* fixing tests\n\n* coverage\n\n* coverage\n\n* coverage\n\n* exclude line",
          "timestamp": "2026-07-09T11:44:22+02:00",
          "tree_id": "071ba85265bf6f6c4f02630ec00468bd6743fcd1",
          "url": "https://github.com/najaeda/naja/commit/eb1dde036f924ecf3a181d73776bb5a9918509b3"
        },
        "date": 1783590401891,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 172463.5815602808,
            "unit": "ns/iter",
            "extra": "iterations: 3807\ncpu: 172459.39374835833 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 276070.2196669291,
            "unit": "ns/iter",
            "extra": "iterations: 2522\ncpu: 276071.49048374314 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1805002.3608247833,
            "unit": "ns/iter",
            "extra": "iterations: 388\ncpu: 1805010.631443299 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18463949.36841995,
            "unit": "ns/iter",
            "extra": "iterations: 38\ncpu: 18463824.57894737 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1562.1607637627812,
            "unit": "ns/iter",
            "extra": "iterations: 448621\ncpu: 1562.1057016947495 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 15368.904351071913,
            "unit": "ns/iter",
            "extra": "iterations: 45552\ncpu: 15368.27772655427 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 161815.2581417619,
            "unit": "ns/iter",
            "extra": "iterations: 4176\ncpu: 161809.26412835243 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 403605.55103146454,
            "unit": "ns/iter",
            "extra": "iterations: 1842\ncpu: 403581.73344191065 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 360829.4615384443,
            "unit": "ns/iter",
            "extra": "iterations: 1937\ncpu: 360818.7160557565 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 301338.1596283639,
            "unit": "ns/iter",
            "extra": "iterations: 2368\ncpu: 301335.1718749994 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 75050780.87500294,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 75044488.99999994 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 22262.942750031343,
            "unit": "ns/iter",
            "extra": "iterations: 31476\ncpu: 22262.758768585572 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 350821.61472902447,
            "unit": "ns/iter",
            "extra": "iterations: 1996\ncpu: 350678.01603207685 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "michael@rogenmoser.us",
            "name": "Michael Rogenmoser",
            "username": "micprog"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "2fa82d9e89b11d7f271356e9a75401ce5008be51",
          "message": "sv frontend: lower packed-array assignment patterns (#406)\n\nresolveUnbasedOrStructuredPatternBits handled '{...} assignment patterns for\nunpacked arrays and packed structs, but not packed arrays (e.g.\n`logic [1:0][63:0] v = '{a, b}`). Such a pattern fell through to nullopt, so any\ncontinuous-assign RHS containing one — directly, or nested inside a '{...}\nstruct pattern via a member setter — was rejected with\n\"Unsupported RHS in continuous assign ... StructuredAssignmentPattern\".\n\nAdd a packed-array case that mirrors the existing unpacked-array element\nplacement, deriving element width from the packed element type and mapping each\npattern element to its bit offset via the array's fixed range\n(translateIndex(index) * elementWidth).\n\nTests: parsePackedArrayAssignmentPatternInContinuousAssign (bare packed array)\nand parseStructPatternWithPackedArrayMember (packed array nested in a packed\nstruct pattern), both previously threw SNLSVUnsupportedConstructError.\n\nCo-authored-by: Claude Opus 4.8 (1M context) <noreply@anthropic.com>",
          "timestamp": "2026-07-10T17:06:35+02:00",
          "tree_id": "6b4ea1753a396f0eed74f2e6e18ccd47fe6831c3",
          "url": "https://github.com/najaeda/naja/commit/2fa82d9e89b11d7f271356e9a75401ce5008be51"
        },
        "date": 1783696739296,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 194312.7004137907,
            "unit": "ns/iter",
            "extra": "iterations: 3625\ncpu: 194287.95448275865 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 297123.27896081784,
            "unit": "ns/iter",
            "extra": "iterations: 2348\ncpu: 297094.0374787054 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1892163.1769437492,
            "unit": "ns/iter",
            "extra": "iterations: 373\ncpu: 1892081.0402144778 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18242233.605263893,
            "unit": "ns/iter",
            "extra": "iterations: 38\ncpu: 18242156.973684203 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1455.455376539079,
            "unit": "ns/iter",
            "extra": "iterations: 479658\ncpu: 1455.3918187541951 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14350.337894780147,
            "unit": "ns/iter",
            "extra": "iterations: 48888\ncpu: 14349.163352970056 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 214345.158484858,
            "unit": "ns/iter",
            "extra": "iterations: 3300\ncpu: 214326.97999999995 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 465596.244337678,
            "unit": "ns/iter",
            "extra": "iterations: 1457\ncpu: 465589.7158544957 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 412221.3936358318,
            "unit": "ns/iter",
            "extra": "iterations: 1697\ncpu: 412207.9770182677 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 333098.7728136977,
            "unit": "ns/iter",
            "extra": "iterations: 2104\ncpu: 333097.0128326994 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 72805145.25000115,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 72802086.50000009 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 21641.36063648924,
            "unit": "ns/iter",
            "extra": "iterations: 32365\ncpu: 21640.459137957696 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 359229.69301837496,
            "unit": "ns/iter",
            "extra": "iterations: 1948\ncpu: 359103.5349076325 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "michael@rogenmoser.us",
            "name": "Michael Rogenmoser",
            "username": "micprog"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "30f65197f51c04dc3ec0ddd0143bb3b8f5e6db90",
          "message": "sv frontend: lower $countones as an adder-fold popcount (#407)\n\n$countones(x) on a runtime operand is not constant-foldable, so it reached\nresolveExpressionBits as an unresolved system Call and was rejected with\n\"Unsupported RHS in continuous assign ... Call\". It appears throughout\nissue-queue / freelist style RTL (population counts of valid/request vectors),\nincluding as an operand of relational comparisons and inside struct/sequential\nassignments.\n\nLower it structurally: resolve the operand's bits, then sum them with an\nadder-fold over addBitVectors into a bit_width(N)-wide accumulator (so the\nrunning sum cannot overflow), and resize to the requested target width.\n\nTests:\n- parseCountOnesInContinuousAssign (new) — previously threw\n  SNLSVUnsupportedConstructError.\n- parseAlwaysCombStructuredAssignmentPatternMemberSetterExprResolveFailureUnsupported\n  used $countones purely as a stand-in \"unresolvable\" member-setter expression;\n  since it is now supported, switch that negative test to the existing\n  bad_cond() case-inside-with-x-range idiom so it still exercises the\n  member-setter-resolve-failure path.\n\nCo-authored-by: Claude Opus 4.8 (1M context) <noreply@anthropic.com>",
          "timestamp": "2026-07-10T19:47:04+02:00",
          "tree_id": "44cd46ec235efa6a8b55dac8f4d775f96b93716a",
          "url": "https://github.com/najaeda/naja/commit/30f65197f51c04dc3ec0ddd0143bb3b8f5e6db90"
        },
        "date": 1783705776394,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 200664.17986424136,
            "unit": "ns/iter",
            "extra": "iterations: 3536\ncpu: 200659.27121040729 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 303012.29526341317,
            "unit": "ns/iter",
            "extra": "iterations: 2259\ncpu: 302992.9362549801 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1887013.3526315899,
            "unit": "ns/iter",
            "extra": "iterations: 380\ncpu: 1886943.8657894747 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18462727.05263106,
            "unit": "ns/iter",
            "extra": "iterations: 38\ncpu: 18461614.921052627 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1457.6421812768824,
            "unit": "ns/iter",
            "extra": "iterations: 481076\ncpu: 1457.5029143004426 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14369.58351467934,
            "unit": "ns/iter",
            "extra": "iterations: 48710\ncpu: 14367.558550605623 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 211383.60158151074,
            "unit": "ns/iter",
            "extra": "iterations: 3288\ncpu: 211366.37317518253 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 465497.7785904407,
            "unit": "ns/iter",
            "extra": "iterations: 1504\ncpu: 465449.90558510646 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 408612.5657277056,
            "unit": "ns/iter",
            "extra": "iterations: 1704\ncpu: 408611.90669014136 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 331299.8508522695,
            "unit": "ns/iter",
            "extra": "iterations: 2112\ncpu: 331278.1273674244 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 72983426.24999776,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 72980845.24999982 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 21870.60668284428,
            "unit": "ns/iter",
            "extra": "iterations: 32142\ncpu: 21868.518542716698 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 362147.2152563006,
            "unit": "ns/iter",
            "extra": "iterations: 1914\ncpu: 361951.0292580756 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "committer": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "distinct": true,
          "id": "003f954cca1311b9d78ee693fea4b2f1b426d558",
          "message": "more coverage and testing",
          "timestamp": "2026-07-11T13:52:17+02:00",
          "tree_id": "161d7342bfb47326fed4f186f210a7eb77d75521",
          "url": "https://github.com/najaeda/naja/commit/003f954cca1311b9d78ee693fea4b2f1b426d558"
        },
        "date": 1783770909650,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 179366.79522709732,
            "unit": "ns/iter",
            "extra": "iterations: 3897\ncpu: 179276.21349756222 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 288014.8373245146,
            "unit": "ns/iter",
            "extra": "iterations: 2422\ncpu: 287941.4867877787 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1736069.1439588193,
            "unit": "ns/iter",
            "extra": "iterations: 389\ncpu: 1735947.6863753211 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18993063.054054078,
            "unit": "ns/iter",
            "extra": "iterations: 37\ncpu: 18991919.405405406 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1564.7410677045712,
            "unit": "ns/iter",
            "extra": "iterations: 448345\ncpu: 1564.603383555075 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 15362.625295662927,
            "unit": "ns/iter",
            "extra": "iterations: 45660\ncpu: 15360.950197109052 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 165754.74994091428,
            "unit": "ns/iter",
            "extra": "iterations: 4231\ncpu: 165734.22075159533 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 414014.5334913923,
            "unit": "ns/iter",
            "extra": "iterations: 1687\ncpu: 413994.803793717 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 365312.5703858246,
            "unit": "ns/iter",
            "extra": "iterations: 1918\ncpu: 365306.43013555784 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 308564.41913340054,
            "unit": "ns/iter",
            "extra": "iterations: 2331\ncpu: 308564.8335478333 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 75385152.14285521,
            "unit": "ns/iter",
            "extra": "iterations: 7\ncpu: 75385538.14285721 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 22332.445308499497,
            "unit": "ns/iter",
            "extra": "iterations: 31248\ncpu: 22331.20327700972 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 350833.34279935417,
            "unit": "ns/iter",
            "extra": "iterations: 2007\ncpu: 350700.86198307556 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "bfc5649c27ebbef9459da87da995aca950610487",
          "message": "B0.7.13 (#408)\n\n* start 0.7.13 version\n\n* doc and version check in naja-if in progress\n\n* more mem inference\n\n* fix test\n\n* update cva6 regress\n\n* coverage\n\n* coverage\n\n* license cleaning\n\n* Fix the versioning checkes in naja-if dumps\n\n* coverage",
          "timestamp": "2026-07-13T19:34:22+02:00",
          "tree_id": "deda569a67390f839f1d2c2acc94687f01d5e88f",
          "url": "https://github.com/najaeda/naja/commit/bfc5649c27ebbef9459da87da995aca950610487"
        },
        "date": 1783964224466,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 187517.47439354417,
            "unit": "ns/iter",
            "extra": "iterations: 3710\ncpu: 187510.20377358492 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 293310.91399231134,
            "unit": "ns/iter",
            "extra": "iterations: 2337\ncpu: 293311.57466837834 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1872301.4095743787,
            "unit": "ns/iter",
            "extra": "iterations: 376\ncpu: 1871900.5877659584 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17931087.56410278,
            "unit": "ns/iter",
            "extra": "iterations: 39\ncpu: 17930858.82051282 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1480.0590450110246,
            "unit": "ns/iter",
            "extra": "iterations: 473283\ncpu: 1479.823006530977 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14527.14837570879,
            "unit": "ns/iter",
            "extra": "iterations: 44758\ncpu: 14524.093123017108 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 204211.51530907466,
            "unit": "ns/iter",
            "extra": "iterations: 3462\ncpu: 204176.25909878683 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 454958.9311688413,
            "unit": "ns/iter",
            "extra": "iterations: 1540\ncpu: 454922.1064935068 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 402134.68635320914,
            "unit": "ns/iter",
            "extra": "iterations: 1744\ncpu: 402109.9587155958 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 322412.4497695862,
            "unit": "ns/iter",
            "extra": "iterations: 2170\ncpu: 322408.6898617512 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 72978076.24999563,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 72973978.00000004 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 22754.202218484552,
            "unit": "ns/iter",
            "extra": "iterations: 30922\ncpu: 22753.86375396157 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 374932.95891103556,
            "unit": "ns/iter",
            "extra": "iterations: 1874\ncpu: 375003.248132351 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "6d033c316a3c86295ed8fb091a0bbb77d5aad6f2",
          "message": "B0.7.14 (#409)\n\n* start next v0.7.14\n\n* new testing\n\n* add 3.15 python support, remove 3.9",
          "timestamp": "2026-07-15T13:09:01+02:00",
          "tree_id": "15f4d3d0118305e29afe5e2c643ba8b5ccbbdf04",
          "url": "https://github.com/najaeda/naja/commit/6d033c316a3c86295ed8fb091a0bbb77d5aad6f2"
        },
        "date": 1784113891966,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 170319.81813602216,
            "unit": "ns/iter",
            "extra": "iterations: 3970\ncpu: 170311.7186397985 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 280487.2426992996,
            "unit": "ns/iter",
            "extra": "iterations: 2534\ncpu: 280480.9925019731 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1750703.1695760514,
            "unit": "ns/iter",
            "extra": "iterations: 401\ncpu: 1750659.6758104737 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 19602405.897435695,
            "unit": "ns/iter",
            "extra": "iterations: 39\ncpu: 19601865.512820523 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1622.6160586108492,
            "unit": "ns/iter",
            "extra": "iterations: 431183\ncpu: 1622.5769684797415 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 16079.12990743695,
            "unit": "ns/iter",
            "extra": "iterations: 43862\ncpu: 16078.159044275235 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 171010.03947046172,
            "unit": "ns/iter",
            "extra": "iterations: 4079\ncpu: 171003.01887717584 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 408174.8572261091,
            "unit": "ns/iter",
            "extra": "iterations: 1716\ncpu: 408168.1025641025 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 360574.44061696396,
            "unit": "ns/iter",
            "extra": "iterations: 1945\ncpu: 360558.40719794395 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 293526.84431389533,
            "unit": "ns/iter",
            "extra": "iterations: 2383\ncpu: 293526.41082668904 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 75320732.12500024,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 75318137.99999987 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 22869.383702086918,
            "unit": "ns/iter",
            "extra": "iterations: 30237\ncpu: 22867.897245097105 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 372894.7282895175,
            "unit": "ns/iter",
            "extra": "iterations: 1877\ncpu: 372748.3409695922 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "e1a649e2fce182d8b7ca5c5c80ab5d04aad3ffa3",
          "message": "B0.7.15 (#411)",
          "timestamp": "2026-07-16T18:32:26+02:00",
          "tree_id": "8461cda77903e998ff269e7a34ef9c525afe6a1d",
          "url": "https://github.com/najaeda/naja/commit/e1a649e2fce182d8b7ca5c5c80ab5d04aad3ffa3"
        },
        "date": 1784219699062,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 138527.54704913194,
            "unit": "ns/iter",
            "extra": "iterations: 4846\ncpu: 138514.47172926128 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 212949.45524455007,
            "unit": "ns/iter",
            "extra": "iterations: 3251\ncpu: 212944.57766840968 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1409015.2335329857,
            "unit": "ns/iter",
            "extra": "iterations: 501\ncpu: 1408637.526946108 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 14801412.693876566,
            "unit": "ns/iter",
            "extra": "iterations: 49\ncpu: 14800368.632653056 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1234.7618040289574,
            "unit": "ns/iter",
            "extra": "iterations: 566798\ncpu: 1234.5219707902993 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12140.599542571945,
            "unit": "ns/iter",
            "extra": "iterations: 57714\ncpu: 12139.725889732137 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 129658.87173875431,
            "unit": "ns/iter",
            "extra": "iterations: 5481\ncpu: 129519.65955117685 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 317892.46501125133,
            "unit": "ns/iter",
            "extra": "iterations: 2215\ncpu: 317845.6528216706 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 277159.2948159825,
            "unit": "ns/iter",
            "extra": "iterations: 2527\ncpu: 277141.62326869776 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 226007.89293777096,
            "unit": "ns/iter",
            "extra": "iterations: 3101\ncpu: 225983.68042566936 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 58509880.89999874,
            "unit": "ns/iter",
            "extra": "iterations: 10\ncpu: 58506739.99999998 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 17754.15119760485,
            "unit": "ns/iter",
            "extra": "iterations: 39412\ncpu: 17751.49781792349 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 286591.9288646777,
            "unit": "ns/iter",
            "extra": "iterations: 2460\ncpu: 286468.79552843864 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "feb8a09303cb6139145f170b7fa9da26691d2659",
          "message": "naja perf disabled by default in najaeda (#412)",
          "timestamp": "2026-07-17T01:44:49+02:00",
          "tree_id": "492dbfab6077ec945d88b8f497d12fa7b92c2160",
          "url": "https://github.com/najaeda/naja/commit/feb8a09303cb6139145f170b7fa9da26691d2659"
        },
        "date": 1784245637393,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 201920.36975035878,
            "unit": "ns/iter",
            "extra": "iterations: 3405\ncpu: 201911.80851688693 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 301361.57117826305,
            "unit": "ns/iter",
            "extra": "iterations: 2283\ncpu: 301348.55190538766 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1911658.3827494318,
            "unit": "ns/iter",
            "extra": "iterations: 371\ncpu: 1911570.6469002694 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18953482.02777806,
            "unit": "ns/iter",
            "extra": "iterations: 36\ncpu: 18952405.222222216 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1544.1975817967182,
            "unit": "ns/iter",
            "extra": "iterations: 448184\ncpu: 1544.1340900165992 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 15205.013157319869,
            "unit": "ns/iter",
            "extra": "iterations: 45754\ncpu: 15205.035603444521 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 214628.97676445876,
            "unit": "ns/iter",
            "extra": "iterations: 3443\ncpu: 214628.7522509438 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 475988.14557824086,
            "unit": "ns/iter",
            "extra": "iterations: 1470\ncpu: 475952.63129251724 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 418083.4186747031,
            "unit": "ns/iter",
            "extra": "iterations: 1660\ncpu: 418062.2716867469 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 338391.8528159116,
            "unit": "ns/iter",
            "extra": "iterations: 2113\ncpu: 338371.69947941357 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 74360617.75000268,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 74356433.49999999 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 23300.737226037567,
            "unit": "ns/iter",
            "extra": "iterations: 30570\ncpu: 23299.891298658797 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 384372.4803072965,
            "unit": "ns/iter",
            "extra": "iterations: 1828\ncpu: 384164.89277897315 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "d67342020f295c2563d85a6c051f6adf02843985",
          "message": "Adding X and Z types (#414)\n\n* Adding X and Z types\n\n* fix naja-if\n\n* improve coverage\n\n* coverage",
          "timestamp": "2026-07-21T09:19:33+02:00",
          "tree_id": "ea55e6bbc67ac8706c76d4c130679788092d33ab",
          "url": "https://github.com/najaeda/naja/commit/d67342020f295c2563d85a6c051f6adf02843985"
        },
        "date": 1784618530697,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 189705.4120864107,
            "unit": "ns/iter",
            "extra": "iterations: 3657\ncpu: 189575.81405523655 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 290343.3284762703,
            "unit": "ns/iter",
            "extra": "iterations: 2402\ncpu: 290206.0728559534 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1863341.2414697711,
            "unit": "ns/iter",
            "extra": "iterations: 381\ncpu: 1862884.8241469816 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17686589.649999715,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17684799.05 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1479.520783446135,
            "unit": "ns/iter",
            "extra": "iterations: 477327\ncpu: 1479.3255147938426 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14370.0522428094,
            "unit": "ns/iter",
            "extra": "iterations: 48466\ncpu: 14367.900033012838 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 198385.15912541957,
            "unit": "ns/iter",
            "extra": "iterations: 3293\ncpu: 198357.75766778007 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 459000.8983606568,
            "unit": "ns/iter",
            "extra": "iterations: 1525\ncpu: 458941.7586885248 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 405126.1397911903,
            "unit": "ns/iter",
            "extra": "iterations: 1724\ncpu: 405095.93039443146 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 325824.6917712682,
            "unit": "ns/iter",
            "extra": "iterations: 2151\ncpu: 325812.0032542999 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 73384598.24999788,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 73374982.50000007 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 21450.413946088272,
            "unit": "ns/iter",
            "extra": "iterations: 32683\ncpu: 21449.158645167197 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 377253.8661630841,
            "unit": "ns/iter",
            "extra": "iterations: 1853\ncpu: 377066.467889876 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "committer": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "distinct": true,
          "id": "dd16e9310e6b967910ff9ac5ac0285faf469b78a",
          "message": "bump naja-if version",
          "timestamp": "2026-07-21T10:11:47+02:00",
          "tree_id": "aca5febe17afa9bfe50c69e4f3e2179dbb4f201e",
          "url": "https://github.com/najaeda/naja/commit/dd16e9310e6b967910ff9ac5ac0285faf469b78a"
        },
        "date": 1784621673058,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 156803.77621908998,
            "unit": "ns/iter",
            "extra": "iterations: 4491\ncpu: 156803.45446448453 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 252291.2546099062,
            "unit": "ns/iter",
            "extra": "iterations: 2820\ncpu: 252236.96843971632 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1690170.8240963551,
            "unit": "ns/iter",
            "extra": "iterations: 415\ncpu: 1690182.3542168674 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17195055.34146176,
            "unit": "ns/iter",
            "extra": "iterations: 41\ncpu: 17195894.317073163 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1256.4882399691207,
            "unit": "ns/iter",
            "extra": "iterations: 499531\ncpu: 1256.4891868572715 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12164.017558733794,
            "unit": "ns/iter",
            "extra": "iterations: 56610\ncpu: 12164.108797032331 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 119943.5840648123,
            "unit": "ns/iter",
            "extra": "iterations: 5924\ncpu: 119942.35955435512 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 357376.83223178826,
            "unit": "ns/iter",
            "extra": "iterations: 1967\ncpu: 357378.16929333983 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 312887.853469155,
            "unit": "ns/iter",
            "extra": "iterations: 2061\ncpu: 312887.2163998063 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 251704.17579054856,
            "unit": "ns/iter",
            "extra": "iterations: 2941\ncpu: 251699.02754165287 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 65895448.00001477,
            "unit": "ns/iter",
            "extra": "iterations: 10\ncpu: 65895641.499999866 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 21238.156104533762,
            "unit": "ns/iter",
            "extra": "iterations: 30076\ncpu: 21237.558618167313 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 265533.7547128513,
            "unit": "ns/iter",
            "extra": "iterations: 2813\ncpu: 253437.19694276573 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "committer": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "distinct": true,
          "id": "32d76288feb571db867d950ca7c657182e690bac",
          "message": "prepare for 0.7.17 version",
          "timestamp": "2026-07-22T12:25:39+02:00",
          "tree_id": "5d579a82ed4b4a01b86feceb796d026f1ef1966c",
          "url": "https://github.com/najaeda/naja/commit/32d76288feb571db867d950ca7c657182e690bac"
        },
        "date": 1784716096365,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 204512.02564883357,
            "unit": "ns/iter",
            "extra": "iterations: 3275\ncpu: 204471.54900763358 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 314026.2719609915,
            "unit": "ns/iter",
            "extra": "iterations: 2254\ncpu: 313959.31677018636 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1987005.6103153282,
            "unit": "ns/iter",
            "extra": "iterations: 349\ncpu: 1986206.3925501439 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 21964801.878790706,
            "unit": "ns/iter",
            "extra": "iterations: 33\ncpu: 21963323.21212121 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1527.6428208435957,
            "unit": "ns/iter",
            "extra": "iterations: 450617\ncpu: 1527.5843277106724 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14982.139062194663,
            "unit": "ns/iter",
            "extra": "iterations: 46001\ncpu: 14981.886024216872 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 206848.6495177108,
            "unit": "ns/iter",
            "extra": "iterations: 3110\ncpu: 206849.47073954975 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 502015.3547445135,
            "unit": "ns/iter",
            "extra": "iterations: 1370\ncpu: 501974.41167883214 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 445895.65150543494,
            "unit": "ns/iter",
            "extra": "iterations: 1561\ncpu: 445869.9903907759 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 355634.8096202483,
            "unit": "ns/iter",
            "extra": "iterations: 1975\ncpu: 355630.95898734214 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 81068344.99999748,
            "unit": "ns/iter",
            "extra": "iterations: 7\ncpu: 81065902.28571443 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 23009.758593469804,
            "unit": "ns/iter",
            "extra": "iterations: 30401\ncpu: 23007.647182658446 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 422871.423379583,
            "unit": "ns/iter",
            "extra": "iterations: 1651\ncpu: 422698.8394912211 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "64744cef1f86864efa18c382b1988bd32e1d0b18",
          "message": "Allow multiple definitions of a primitive in Liberty (#416)",
          "timestamp": "2026-07-27T02:51:42+02:00",
          "tree_id": "7b172fd09d5f4d1f91a0cd4166799c66d6713c47",
          "url": "https://github.com/najaeda/naja/commit/64744cef1f86864efa18c382b1988bd32e1d0b18"
        },
        "date": 1785113649540,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 192384.867232398,
            "unit": "ns/iter",
            "extra": "iterations: 3653\ncpu: 192381.91842321382 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 294843.0869017533,
            "unit": "ns/iter",
            "extra": "iterations: 2382\ncpu: 294838.94584382884 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1873013.2789473052,
            "unit": "ns/iter",
            "extra": "iterations: 380\ncpu: 1872965.736842105 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17892641.615384117,
            "unit": "ns/iter",
            "extra": "iterations: 39\ncpu: 17891877.74358974 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1468.7671666572667,
            "unit": "ns/iter",
            "extra": "iterations: 478209\ncpu: 1468.6529760000333 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14557.776764668768,
            "unit": "ns/iter",
            "extra": "iterations: 48366\ncpu: 14556.199975189187 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 218717.81029322764,
            "unit": "ns/iter",
            "extra": "iterations: 3342\ncpu: 218701.64991023348 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 468713.4483914397,
            "unit": "ns/iter",
            "extra": "iterations: 1492\ncpu: 468687.71581769426 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 411758.58515025827,
            "unit": "ns/iter",
            "extra": "iterations: 1697\ncpu: 411763.1290512676 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 330482.2062293527,
            "unit": "ns/iter",
            "extra": "iterations: 2119\ncpu: 330450.75176970306 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 73046811.37500069,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 73042832.5000001 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 21337.249847096,
            "unit": "ns/iter",
            "extra": "iterations: 32700\ncpu: 21335.299418960207 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 383396.3012048198,
            "unit": "ns/iter",
            "extra": "iterations: 1826\ncpu: 383175.1851040682 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "committer": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "distinct": true,
          "id": "03c31df751c2e5f039098560d075fc00578d0a4d",
          "message": "bump version number",
          "timestamp": "2026-07-27T23:04:48+02:00",
          "tree_id": "2c30d26bf8882933dd1ac685d82a6b40abdbafdf",
          "url": "https://github.com/najaeda/naja/commit/03c31df751c2e5f039098560d075fc00578d0a4d"
        },
        "date": 1785186436575,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 179835.98707940584,
            "unit": "ns/iter",
            "extra": "iterations: 3715\ncpu: 179828.80107671602 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 284046.86116700154,
            "unit": "ns/iter",
            "extra": "iterations: 2485\ncpu: 284033.8893360161 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1817730.5038961237,
            "unit": "ns/iter",
            "extra": "iterations: 385\ncpu: 1817596.932467532 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18223615.657894835,
            "unit": "ns/iter",
            "extra": "iterations: 38\ncpu: 18222418.447368424 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1573.4485589597934,
            "unit": "ns/iter",
            "extra": "iterations: 445685\ncpu: 1573.3374019767323 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 15486.6235320139,
            "unit": "ns/iter",
            "extra": "iterations: 45215\ncpu: 15485.676191529365 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 164491.27322534317,
            "unit": "ns/iter",
            "extra": "iterations: 4198\ncpu: 164479.1953311101 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 413715.50351699995,
            "unit": "ns/iter",
            "extra": "iterations: 1706\ncpu: 413684.52227432554 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 363190.40953862405,
            "unit": "ns/iter",
            "extra": "iterations: 1929\ncpu: 363161.81285640213 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 297349.1472967287,
            "unit": "ns/iter",
            "extra": "iterations: 2349\ncpu: 297328.6973180079 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 74179119.5714297,
            "unit": "ns/iter",
            "extra": "iterations: 7\ncpu: 74172282.57142864 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 21739.94926548094,
            "unit": "ns/iter",
            "extra": "iterations: 32266\ncpu: 21738.71939502883 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 374046.20700627484,
            "unit": "ns/iter",
            "extra": "iterations: 1884\ncpu: 373900.9819532801 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "committer": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "distinct": true,
          "id": "ad8f1d71c5b7c870f91d4a2c8f1625d3abbdaa57",
          "message": "Merge branch 'main' of https://github.com/najaeda/naja",
          "timestamp": "2026-07-29T22:56:06+02:00",
          "tree_id": "d1e0786b668d9e988d4d0c167b76b0ab0495eac4",
          "url": "https://github.com/najaeda/naja/commit/ad8f1d71c5b7c870f91d4a2c8f1625d3abbdaa57"
        },
        "date": 1785358965096,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 173241.1050470896,
            "unit": "ns/iter",
            "extra": "iterations: 4141\ncpu: 173205.69427674476 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 268589.70264064614,
            "unit": "ns/iter",
            "extra": "iterations: 2613\ncpu: 268540.4003061616 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1718592.9803440028,
            "unit": "ns/iter",
            "extra": "iterations: 407\ncpu: 1718429.32923833 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18890326.13513564,
            "unit": "ns/iter",
            "extra": "iterations: 37\ncpu: 18889194.216216225 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1273.3148947299521,
            "unit": "ns/iter",
            "extra": "iterations: 573525\ncpu: 1273.2236380279842 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12897.02529182917,
            "unit": "ns/iter",
            "extra": "iterations: 54484\ncpu: 12895.954683943912 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 125475.85901762005,
            "unit": "ns/iter",
            "extra": "iterations: 5334\ncpu: 125212.34458192722 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 399280.14025084674,
            "unit": "ns/iter",
            "extra": "iterations: 1754\ncpu: 399230.2554161917 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 350699.5122683928,
            "unit": "ns/iter",
            "extra": "iterations: 1997\ncpu: 350674.3079619429 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 278769.67860049126,
            "unit": "ns/iter",
            "extra": "iterations: 2458\ncpu: 278756.89259560645 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 74592264.87500103,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 74589231.87499988 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 21543.724168746714,
            "unit": "ns/iter",
            "extra": "iterations: 32451\ncpu: 21541.43967828422 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 284716.71915606456,
            "unit": "ns/iter",
            "extra": "iterations: 2464\ncpu: 284590.6834415185 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "c70af6aacef5d0b18f1a39dbd4ff1661af1629b9",
          "message": "Bazel build (#413)\n\n* Bazel build\n\n* ubuntu and mac os builds fixes\n\n* build and license cleaning\n\n* ubuntu build fixes\n\n* build again\n\n* force static\n\n* porting workflows\n\n* clang setup\n\n* new attempt\n\n* keep isolated tests for the moment\n\n* property to keep link with AST Tree\n\n* push asan testing\n\n* clean NLDB\n\n* update naja-if\n\n* specific option for tests\n\n* code cleaning\n\n* disable ccache for slang\n\n* clean code warnings\n\n* bazel: scope @tbb repo rule symlinks to tbb/oneapi subdirs only\n\nSymlinking the whole pkg-config includedir (e.g. /usr/include) put\nevery unrelated system header on the -isystem search path for any\ntarget depending on @tbb, including a stale libgtest-dev gtest.h that\nshadowed Bazel's own fetched @googletest headers -- header/library\nversion mismatch caused undefined MakeAndRegisterTestInfo references\nat link time for cc_test targets. Symlink only tbb/ and oneapi/ (the\ntwo subdirs oneTBB actually ships) instead.\n\nCo-Authored-By: Claude Sonnet 5 <noreply@anthropic.com>\n\n* bazel: vendor Boost via http_archive instead of wrapping system install\n\nReplaces the find_path-style boost_repository rule (which searched\ncommon install prefixes and symlinked the whole discovered directory,\ne.g. /usr/local/include or /usr/include, into the include path) with a\npinned http_archive fetch of the official 1.89.0 release tarball,\nfollowing the same vendoring pattern already used for fmt and\ntomlplusplus in MODULE.bazel.\n\nThis removes two problems with the system-wrapping approach: the exact\nBoost version (and which of several installed copies got picked, by\nsearch-order priority) varied by machine, and the broad directory\nsymlink was found to leak unrelated system headers -- including a\nstale libgtest-dev gtest.h that shadowed Bazel's own fetched\n@googletest headers on one dev machine, causing gtest ABI mismatches\nat link time.\n\nCo-Authored-By: Claude Sonnet 5 <noreply@anthropic.com>\n\n* slang building\n\n* bump setup-bazel version\n\n* silence lef def warnings\n\n* more details\n\n* global flags\n\n* tsan fixes\n\n* switch more to bazel\n\n* fix workflow\n\n* less warnings\n\n* fix workflows\n\n* more translation\n\n* come back to bazel as second build system\n\n* coverage\n\n* cleaning\n\n* bump naja-verilog to main\n\n* update bazel targeted modules\n\n* bump version number\n\n---------\n\nCo-authored-by: Claude Sonnet 5 <noreply@anthropic.com>",
          "timestamp": "2026-07-30T00:13:44+02:00",
          "tree_id": "7eb0cc7e33ed563c6f332c3669d4bbb63872af91",
          "url": "https://github.com/najaeda/naja/commit/c70af6aacef5d0b18f1a39dbd4ff1661af1629b9"
        },
        "date": 1785364150306,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 134169.4033170736,
            "unit": "ns/iter",
            "extra": "iterations: 5125\ncpu: 134147.5943414634 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 207802.1624064804,
            "unit": "ns/iter",
            "extra": "iterations: 3208\ncpu: 207778.20355361604 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1266201.987632501,
            "unit": "ns/iter",
            "extra": "iterations: 566\ncpu: 1266056.5689045934 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 12852419.425926158,
            "unit": "ns/iter",
            "extra": "iterations: 54\ncpu: 12851421.703703705 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 989.5529581129916,
            "unit": "ns/iter",
            "extra": "iterations: 707833\ncpu: 989.2856210433807 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 9791.290190165124,
            "unit": "ns/iter",
            "extra": "iterations: 71622\ncpu: 9788.748806232721 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 102916.25711053415,
            "unit": "ns/iter",
            "extra": "iterations: 6188\ncpu: 102897.30365223008 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 298674.2916842597,
            "unit": "ns/iter",
            "extra": "iterations: 2369\ncpu: 298621.7108484595 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 262575.8810506518,
            "unit": "ns/iter",
            "extra": "iterations: 2665\ncpu: 262537.70431519684 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 213791.64822738495,
            "unit": "ns/iter",
            "extra": "iterations: 3272\ncpu: 213770.6421149142 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 53162570.27272606,
            "unit": "ns/iter",
            "extra": "iterations: 11\ncpu: 53156900.27272718 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 16510.613097076017,
            "unit": "ns/iter",
            "extra": "iterations: 42101\ncpu: 16509.09660103084 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 283457.3828070499,
            "unit": "ns/iter",
            "extra": "iterations: 2466\ncpu: 283336.8592862715 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "committer": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "distinct": true,
          "id": "af7b565a49bc589424df0644804f1d6279723a03",
          "message": "avoid conflict with naja.py user script",
          "timestamp": "2026-07-30T10:36:25+02:00",
          "tree_id": "57b34fe10cd971d38816b63168a4e58e634067ac",
          "url": "https://github.com/najaeda/naja/commit/af7b565a49bc589424df0644804f1d6279723a03"
        },
        "date": 1785402125608,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 178450.1391544936,
            "unit": "ns/iter",
            "extra": "iterations: 3974\ncpu: 178431.68268746854 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 266865.8140511697,
            "unit": "ns/iter",
            "extra": "iterations: 2619\ncpu: 266849.41351660946 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1692111.4186602694,
            "unit": "ns/iter",
            "extra": "iterations: 418\ncpu: 1691973.0023923453 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 16311762.720930388,
            "unit": "ns/iter",
            "extra": "iterations: 43\ncpu: 16310673.04651162 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1215.2639989723712,
            "unit": "ns/iter",
            "extra": "iterations: 576078\ncpu: 1214.9927179999934 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12055.365915391585,
            "unit": "ns/iter",
            "extra": "iterations: 58079\ncpu: 12053.380464539683 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 177380.7592592555,
            "unit": "ns/iter",
            "extra": "iterations: 3996\ncpu: 177370.77277277265 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 432029.0875462377,
            "unit": "ns/iter",
            "extra": "iterations: 1622\ncpu: 431985.1374845871 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 381762.0443349546,
            "unit": "ns/iter",
            "extra": "iterations: 1827\ncpu: 381722.71975916816 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 306373.3312937143,
            "unit": "ns/iter",
            "extra": "iterations: 2288\ncpu: 306360.23645104957 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 66881686.22222039,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 66879173.555555634 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20593.498564733734,
            "unit": "ns/iter",
            "extra": "iterations: 34140\ncpu: 20591.2450205038 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 354373.8718866458,
            "unit": "ns/iter",
            "extra": "iterations: 1967\ncpu: 354190.4280630275 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "420b5ec7c4d02e5f4b3919b047cdbc0347c45d75",
          "message": "fix build error due to module shadowing (#417)\n\n* fix build error due to module shadowing\n\n* linting fix",
          "timestamp": "2026-07-31T00:13:49+02:00",
          "tree_id": "dc0ef1a8dac20f958c63d5503563956439fcc768",
          "url": "https://github.com/najaeda/naja/commit/420b5ec7c4d02e5f4b3919b047cdbc0347c45d75"
        },
        "date": 1785454106870,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 160843.10553505458,
            "unit": "ns/iter",
            "extra": "iterations: 4065\ncpu: 160819.1854858549 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 253823.94618181727,
            "unit": "ns/iter",
            "extra": "iterations: 2750\ncpu: 253811.66290909087 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1655133.3717647514,
            "unit": "ns/iter",
            "extra": "iterations: 425\ncpu: 1654838.7552941174 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 16826982.70731707,
            "unit": "ns/iter",
            "extra": "iterations: 41\ncpu: 16825432.829268295 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1280.3307760239372,
            "unit": "ns/iter",
            "extra": "iterations: 546929\ncpu: 1280.1616553519736 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12525.069224293888,
            "unit": "ns/iter",
            "extra": "iterations: 55833\ncpu: 12523.728386438122 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 137671.29591432278,
            "unit": "ns/iter",
            "extra": "iterations: 5042\ncpu: 137665.85382784615 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 383884.641095881,
            "unit": "ns/iter",
            "extra": "iterations: 1825\ncpu: 383844.28821917815 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 341135.4181201545,
            "unit": "ns/iter",
            "extra": "iterations: 2064\ncpu: 341105.1216085273 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 279196.07780865044,
            "unit": "ns/iter",
            "extra": "iterations: 2519\ncpu: 279156.7788805086 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 67758736.11111176,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 67697937.55555555 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 21279.101739800066,
            "unit": "ns/iter",
            "extra": "iterations: 32475\ncpu: 21276.880831408755 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 349887.38292525895,
            "unit": "ns/iter",
            "extra": "iterations: 2003\ncpu: 349683.5047429062 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "3a29f0cb35f1e0590b456810e3a1f6957596ff56",
          "message": "fix and tests (#418)",
          "timestamp": "2026-07-31T10:44:20+02:00",
          "tree_id": "94ea7c711c36b6993797138ea02b6975c15ab5fe",
          "url": "https://github.com/najaeda/naja/commit/3a29f0cb35f1e0590b456810e3a1f6957596ff56"
        },
        "date": 1785487617412,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 183101.95361216972,
            "unit": "ns/iter",
            "extra": "iterations: 3945\ncpu: 183074.8854245881 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 274059.065140149,
            "unit": "ns/iter",
            "extra": "iterations: 2533\ncpu: 273992.6395578366 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1709336.8341463478,
            "unit": "ns/iter",
            "extra": "iterations: 410\ncpu: 1708783.7682926839 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 16755291.023809519,
            "unit": "ns/iter",
            "extra": "iterations: 42\ncpu: 16754456.238095239 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1214.5799852514226,
            "unit": "ns/iter",
            "extra": "iterations: 576325\ncpu: 1214.510795124279 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 11992.221629112571,
            "unit": "ns/iter",
            "extra": "iterations: 58449\ncpu: 11990.854847816056 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 157761.96282709524,
            "unit": "ns/iter",
            "extra": "iterations: 4089\ncpu: 157748.39667400336 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 448286.87294562714,
            "unit": "ns/iter",
            "extra": "iterations: 1582\ncpu: 448242.47218710516 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 389325.76914413174,
            "unit": "ns/iter",
            "extra": "iterations: 1776\ncpu: 389297.20664414385 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 313547.2835420463,
            "unit": "ns/iter",
            "extra": "iterations: 2236\ncpu: 313529.12030411453 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 79283101.85714541,
            "unit": "ns/iter",
            "extra": "iterations: 7\ncpu: 79273482.14285703 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20565.86226912858,
            "unit": "ns/iter",
            "extra": "iterations: 34110\ncpu: 20564.18343594254 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 364249.23856584507,
            "unit": "ns/iter",
            "extra": "iterations: 1924\ncpu: 364067.3477130586 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "noam.chn1@gmail.com",
            "name": "Noam Cohen",
            "username": "nanocoh"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "ed7cdac682c573b00c04015c3f2cf7df51808d25",
          "message": "Parsing sequential data in liberty",
          "timestamp": "2026-07-31T14:58:53+02:00",
          "tree_id": "8ce7c60842b7fe9bebcff98fb0d45e878f6e597e",
          "url": "https://github.com/najaeda/naja/commit/ed7cdac682c573b00c04015c3f2cf7df51808d25"
        },
        "date": 1785502886343,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 212690.6193324946,
            "unit": "ns/iter",
            "extra": "iterations: 3176\ncpu: 212659.17947103275 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 311288.56236029643,
            "unit": "ns/iter",
            "extra": "iterations: 2237\ncpu: 311193.54537326784 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1724048.2899262477,
            "unit": "ns/iter",
            "extra": "iterations: 407\ncpu: 1723489.904176905 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18217926.12500062,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 18216368.525000002 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1215.3053996301596,
            "unit": "ns/iter",
            "extra": "iterations: 577021\ncpu: 1215.16157470872 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12090.396544268851,
            "unit": "ns/iter",
            "extra": "iterations: 57817\ncpu: 12087.623570922042 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 170523.13361324213,
            "unit": "ns/iter",
            "extra": "iterations: 4049\ncpu: 170497.27438873786 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 477015.0879945458,
            "unit": "ns/iter",
            "extra": "iterations: 1466\ncpu: 476960.24351978156 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 428960.39154412504,
            "unit": "ns/iter",
            "extra": "iterations: 1632\ncpu: 428908.2089460785 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 353328.6512330159,
            "unit": "ns/iter",
            "extra": "iterations: 1987\ncpu: 353300.56014091644 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 66298348.87499797,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 66287135.00000005 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20847.304068459463,
            "unit": "ns/iter",
            "extra": "iterations: 33772\ncpu: 20846.05625962333 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 405794.8807868184,
            "unit": "ns/iter",
            "extra": "iterations: 1728\ncpu: 405691.13425924286 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "noam.chn1@gmail.com",
            "name": "Noam Cohen",
            "username": "nanocoh"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "1eff015d4897a96109e80c1d88183075c8437155",
          "message": "cov (#420)",
          "timestamp": "2026-07-31T19:51:59+02:00",
          "tree_id": "1f467e2eecf1cc9c94a94e25663b974db731a036",
          "url": "https://github.com/najaeda/naja/commit/1eff015d4897a96109e80c1d88183075c8437155"
        },
        "date": 1785520907157,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 216035.05560621683,
            "unit": "ns/iter",
            "extra": "iterations: 3291\ncpu: 215984.59100577337 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 302202.2458028241,
            "unit": "ns/iter",
            "extra": "iterations: 2323\ncpu: 302148.6409814895 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1736275.8029197492,
            "unit": "ns/iter",
            "extra": "iterations: 411\ncpu: 1735979.2944038927 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 16788032.024390016,
            "unit": "ns/iter",
            "extra": "iterations: 41\ncpu: 16787159.731707323 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1214.124989596154,
            "unit": "ns/iter",
            "extra": "iterations: 576704\ncpu: 1214.0519816058152 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12037.896609122767,
            "unit": "ns/iter",
            "extra": "iterations: 58274\ncpu: 12037.38408209492 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 160586.36638126333,
            "unit": "ns/iter",
            "extra": "iterations: 4438\ncpu: 160577.79517800827 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 445280.9536213091,
            "unit": "ns/iter",
            "extra": "iterations: 1574\ncpu: 445270.93392630236 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 397872.1745309937,
            "unit": "ns/iter",
            "extra": "iterations: 1759\ncpu: 397808.1813530415 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 320511.0653503044,
            "unit": "ns/iter",
            "extra": "iterations: 2127\ncpu: 320496.37329572113 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 65519230.888879865,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 65513131.222222276 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20799.786013115812,
            "unit": "ns/iter",
            "extra": "iterations: 33703\ncpu: 20798.19903272708 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 404388.7355865489,
            "unit": "ns/iter",
            "extra": "iterations: 1717\ncpu: 404272.41234712297 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "f65692a1eb8ede66e47645aa86c87713ed832636",
          "message": "Fix bug and improve error messaging in Liberty Constructor (#421)\n\n* Fix bug and improve error messaging in Liberty Constructor\n\n* coverage",
          "timestamp": "2026-08-01T17:01:23+02:00",
          "tree_id": "61eadf5c929d4034156150f22edfd6dbf021ecc0",
          "url": "https://github.com/najaeda/naja/commit/f65692a1eb8ede66e47645aa86c87713ed832636"
        },
        "date": 1785596632366,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 206773.94023544076,
            "unit": "ns/iter",
            "extra": "iterations: 3313\ncpu: 206730.63718683977 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 296291.3884648019,
            "unit": "ns/iter",
            "extra": "iterations: 2358\ncpu: 296209.8944020357 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1640725.9190476085,
            "unit": "ns/iter",
            "extra": "iterations: 420\ncpu: 1640260.669047619 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17428098.025000338,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17425313.475 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1276.317196904387,
            "unit": "ns/iter",
            "extra": "iterations: 548779\ncpu: 1276.256343628309 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12564.571812272474,
            "unit": "ns/iter",
            "extra": "iterations: 55847\ncpu: 12564.049492363063 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 134305.78632478672,
            "unit": "ns/iter",
            "extra": "iterations: 5148\ncpu: 134292.95959595958 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 424111.34564165794,
            "unit": "ns/iter",
            "extra": "iterations: 1652\ncpu: 424105.75968522986 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 380318.469310165,
            "unit": "ns/iter",
            "extra": "iterations: 1841\ncpu: 380263.8571428573 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 316842.35811424,
            "unit": "ns/iter",
            "extra": "iterations: 2206\ncpu: 316807.7688123295 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 66390828.25000031,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 66383589.37499999 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 21550.22109240654,
            "unit": "ns/iter",
            "extra": "iterations: 32552\ncpu: 21548.183306709274 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 388772.63938047126,
            "unit": "ns/iter",
            "extra": "iterations: 1808\ncpu: 388653.63550884696 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "45e88b0916b5ce676dd852282237569b527019e4",
          "message": "Improve error messages focusing on users (#422)",
          "timestamp": "2026-08-02T11:23:05+02:00",
          "tree_id": "be8182bd3364cf19e79c8621b84b5139f9d638aa",
          "url": "https://github.com/najaeda/naja/commit/45e88b0916b5ce676dd852282237569b527019e4"
        },
        "date": 1785662735203,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 145441.47685375068,
            "unit": "ns/iter",
            "extra": "iterations: 4882\ncpu: 145427.64256452274 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 222979.8931445115,
            "unit": "ns/iter",
            "extra": "iterations: 3107\ncpu: 222922.18249114905 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1392948.504816947,
            "unit": "ns/iter",
            "extra": "iterations: 519\ncpu: 1392530.5260115606 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 14053234.122448951,
            "unit": "ns/iter",
            "extra": "iterations: 49\ncpu: 14050971.428571424 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 807.0280299828518,
            "unit": "ns/iter",
            "extra": "iterations: 861292\ncpu: 806.9652336257623 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 8670.69550068336,
            "unit": "ns/iter",
            "extra": "iterations: 82657\ncpu: 8669.973057333313 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 84031.3060732861,
            "unit": "ns/iter",
            "extra": "iterations: 7887\ncpu: 84017.70559147972 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 286609.05010438495,
            "unit": "ns/iter",
            "extra": "iterations: 2395\ncpu: 286580.0860125261 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 255754.9114470887,
            "unit": "ns/iter",
            "extra": "iterations: 2778\ncpu: 255703.4719222465 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 199718.31039488476,
            "unit": "ns/iter",
            "extra": "iterations: 3444\ncpu: 199685.75232288017 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 50056395.363634974,
            "unit": "ns/iter",
            "extra": "iterations: 11\ncpu: 50048828.72727265 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 19156.586521894133,
            "unit": "ns/iter",
            "extra": "iterations: 36563\ncpu: 19153.468670513892 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 224882.9550341083,
            "unit": "ns/iter",
            "extra": "iterations: 3069\ncpu: 224813.26849138513 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "committer": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "distinct": true,
          "id": "7e9d54ba9629d07b61e16df83d0d198a145bd2e4",
          "message": "update README",
          "timestamp": "2026-08-02T22:25:06+02:00",
          "tree_id": "832f6907436f5836354996da84eff8f73fdee429",
          "url": "https://github.com/najaeda/naja/commit/7e9d54ba9629d07b61e16df83d0d198a145bd2e4"
        },
        "date": 1785702466382,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 214567.26485828173,
            "unit": "ns/iter",
            "extra": "iterations: 3281\ncpu: 214527.1002743066 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 304868.3571428556,
            "unit": "ns/iter",
            "extra": "iterations: 2282\ncpu: 304788.59114811564 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1696139.8817734513,
            "unit": "ns/iter",
            "extra": "iterations: 406\ncpu: 1695833.8940886697 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 16513676.219511949,
            "unit": "ns/iter",
            "extra": "iterations: 41\ncpu: 16511328.146341465 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1216.9678240506641,
            "unit": "ns/iter",
            "extra": "iterations: 573938\ncpu: 1216.8711550724995 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12157.55386499699,
            "unit": "ns/iter",
            "extra": "iterations: 57969\ncpu: 12156.232745087898 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 168500.89414270082,
            "unit": "ns/iter",
            "extra": "iterations: 4695\ncpu: 168484.5367412142 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 475800.0429447804,
            "unit": "ns/iter",
            "extra": "iterations: 1467\ncpu: 475731.9918200415 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 430376.26178661257,
            "unit": "ns/iter",
            "extra": "iterations: 1612\ncpu: 430348.4633995045 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 351987.45308580896,
            "unit": "ns/iter",
            "extra": "iterations: 1993\ncpu: 351953.8680381335 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 65682348.22222192,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 65676414.44444443 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20795.657005118177,
            "unit": "ns/iter",
            "extra": "iterations: 33604\ncpu: 20793.368349006058 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 396710.6100450808,
            "unit": "ns/iter",
            "extra": "iterations: 1772\ncpu: 396491.6444695414 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "776920d794e2da7a5307b0cad9fc44bfe7666d60",
          "message": "fix and test Instance Arrays (#423)",
          "timestamp": "2026-08-04T09:29:40+02:00",
          "tree_id": "bba9cf3bc3c120daeebb26a20682f896c804800e",
          "url": "https://github.com/najaeda/naja/commit/776920d794e2da7a5307b0cad9fc44bfe7666d60"
        },
        "date": 1785828741420,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 214529.53512705787,
            "unit": "ns/iter",
            "extra": "iterations: 3345\ncpu: 214489.14349775785 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 303151.83340525685,
            "unit": "ns/iter",
            "extra": "iterations: 2317\ncpu: 303082.1735002158 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1687774.3777239218,
            "unit": "ns/iter",
            "extra": "iterations: 413\ncpu: 1687440.8159806298 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17273253.44999997,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17270839.424999993 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1212.7014499025825,
            "unit": "ns/iter",
            "extra": "iterations: 577625\ncpu: 1212.686822765635 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12087.13648139787,
            "unit": "ns/iter",
            "extra": "iterations: 57898\ncpu: 12085.061072921355 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 177164.13273453625,
            "unit": "ns/iter",
            "extra": "iterations: 4008\ncpu: 177155.9363772457 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 443333.29087452183,
            "unit": "ns/iter",
            "extra": "iterations: 1578\ncpu: 443285.3415716097 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 397338.6459275683,
            "unit": "ns/iter",
            "extra": "iterations: 1768\ncpu: 397327.9762443439 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 320234.6126248607,
            "unit": "ns/iter",
            "extra": "iterations: 2202\ncpu: 320194.9863760216 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 66807719.11109989,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 66503190.222222246 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20636.384120108458,
            "unit": "ns/iter",
            "extra": "iterations: 33703\ncpu: 20634.55926772097 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 390509.10573217954,
            "unit": "ns/iter",
            "extra": "iterations: 1797\ncpu: 390472.008903765 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "776920d794e2da7a5307b0cad9fc44bfe7666d60",
          "message": "fix and test Instance Arrays (#423)",
          "timestamp": "2026-08-04T09:29:40+02:00",
          "tree_id": "bba9cf3bc3c120daeebb26a20682f896c804800e",
          "url": "https://github.com/najaeda/naja/commit/776920d794e2da7a5307b0cad9fc44bfe7666d60"
        },
        "date": 1785829556288,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 212979.24478178873,
            "unit": "ns/iter",
            "extra": "iterations: 3162\ncpu: 212965.84977862114 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 304571.68138528214,
            "unit": "ns/iter",
            "extra": "iterations: 2310\ncpu: 304431.9692640693 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1748242.3616505642,
            "unit": "ns/iter",
            "extra": "iterations: 412\ncpu: 1747954.5752427184 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18190388.289473217,
            "unit": "ns/iter",
            "extra": "iterations: 38\ncpu: 18186726.39473684 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1214.557682560238,
            "unit": "ns/iter",
            "extra": "iterations: 576084\ncpu: 1214.2622603648083 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12130.865107070209,
            "unit": "ns/iter",
            "extra": "iterations: 57579\ncpu: 12129.257837058647 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 175963.9585677813,
            "unit": "ns/iter",
            "extra": "iterations: 3910\ncpu: 175951.3040920715 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 478343.9652588425,
            "unit": "ns/iter",
            "extra": "iterations: 1468\ncpu: 478276.37738419644 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 430223.29386504553,
            "unit": "ns/iter",
            "extra": "iterations: 1630\ncpu: 430162.5024539868 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 352306.5018430639,
            "unit": "ns/iter",
            "extra": "iterations: 1899\ncpu: 352223.3570300162 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 66927028.12500073,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 66918196.9999999 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20826.562963842123,
            "unit": "ns/iter",
            "extra": "iterations: 33686\ncpu: 20823.94965267468 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 398138.49002877413,
            "unit": "ns/iter",
            "extra": "iterations: 1755\ncpu: 398009.36011396506 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "776920d794e2da7a5307b0cad9fc44bfe7666d60",
          "message": "fix and test Instance Arrays (#423)",
          "timestamp": "2026-08-04T09:29:40+02:00",
          "tree_id": "bba9cf3bc3c120daeebb26a20682f896c804800e",
          "url": "https://github.com/najaeda/naja/commit/776920d794e2da7a5307b0cad9fc44bfe7666d60"
        },
        "date": 1785853784717,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 223120.9160328565,
            "unit": "ns/iter",
            "extra": "iterations: 3287\ncpu: 221980.48707027684 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 303950.8862935217,
            "unit": "ns/iter",
            "extra": "iterations: 2269\ncpu: 303844.22785368003 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1722389.2689486665,
            "unit": "ns/iter",
            "extra": "iterations: 409\ncpu: 1721860.4572127145 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 19180540.763157617,
            "unit": "ns/iter",
            "extra": "iterations: 38\ncpu: 19176817.47368422 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1217.6234478183771,
            "unit": "ns/iter",
            "extra": "iterations: 575883\ncpu: 1217.4521508709229 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12099.202431947364,
            "unit": "ns/iter",
            "extra": "iterations: 57896\ncpu: 12095.664968218884 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 168392.30903026537,
            "unit": "ns/iter",
            "extra": "iterations: 4197\ncpu: 168372.75411007844 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 479137.4560327277,
            "unit": "ns/iter",
            "extra": "iterations: 1467\ncpu: 479086.0981595094 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 429063.9292743036,
            "unit": "ns/iter",
            "extra": "iterations: 1626\ncpu: 429046.4145141456 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 351536.92914572736,
            "unit": "ns/iter",
            "extra": "iterations: 1990\ncpu: 351470.17939698446 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 66584017.66666581,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 66577997.88888892 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20663.935396998662,
            "unit": "ns/iter",
            "extra": "iterations: 33791\ncpu: 20661.238643425775 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 400335.20862115157,
            "unit": "ns/iter",
            "extra": "iterations: 1740\ncpu: 400181.6586206965 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "2a3504387b2dcced13f951d0f3be50c1d870ff18",
          "message": "support core-v-mcu (#424)\n\n* support core-v-mcu\n\n* coverage\n\n* bump naja version to 0.7.20\n\n* align Python primitives timing modeling\n\n* keep track of SNLDesignModeling design choice for the future\n\n* fix flake errors\n\n* coverage\n\n* coverage",
          "timestamp": "2026-08-04T20:06:06+02:00",
          "tree_id": "d7add6ad4f4fbbe2ad6c254c7292b1aca6c0d99d",
          "url": "https://github.com/najaeda/naja/commit/2a3504387b2dcced13f951d0f3be50c1d870ff18"
        },
        "date": 1785866926321,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 215646.569436201,
            "unit": "ns/iter",
            "extra": "iterations: 3370\ncpu: 215619.50118694364 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 300068.66994862584,
            "unit": "ns/iter",
            "extra": "iterations: 2336\ncpu: 300040.23672945215 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1707165.8149038474,
            "unit": "ns/iter",
            "extra": "iterations: 416\ncpu: 1706823.074519231 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 16848482.60975622,
            "unit": "ns/iter",
            "extra": "iterations: 41\ncpu: 16846419.170731705 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1231.4081178647962,
            "unit": "ns/iter",
            "extra": "iterations: 569534\ncpu: 1231.3244336597988 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12195.158179980355,
            "unit": "ns/iter",
            "extra": "iterations: 57384\ncpu: 12194.257092569354 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 161063.79936616207,
            "unit": "ns/iter",
            "extra": "iterations: 4102\ncpu: 161057.39078498303 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 467624.46987951506,
            "unit": "ns/iter",
            "extra": "iterations: 1494\ncpu: 467571.112449799 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 420892.20327669656,
            "unit": "ns/iter",
            "extra": "iterations: 1648\ncpu: 420845.6019417482 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 344601.12881022727,
            "unit": "ns/iter",
            "extra": "iterations: 2034\ncpu: 344569.7251720748 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 65583776.11111155,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 65581178.33333337 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20776.15049021131,
            "unit": "ns/iter",
            "extra": "iterations: 33557\ncpu: 20773.3824239354 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 391188.793969615,
            "unit": "ns/iter",
            "extra": "iterations: 1791\ncpu: 391069.2747068858 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "noam.chn1@gmail.com",
            "name": "Noam Cohen",
            "username": "nanocoh"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "a8b4e025a341d8018dd90fde163508c5eb18e5a2",
          "message": "divmode truth table support (#425)\n\nModel divmod outputs with generic truth tables",
          "timestamp": "2026-08-08T11:54:40+02:00",
          "tree_id": "52c961762d965eddd3621a36a01e8be97947b91c",
          "url": "https://github.com/najaeda/naja/commit/a8b4e025a341d8018dd90fde163508c5eb18e5a2"
        },
        "date": 1786183031626,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 210591.2074120259,
            "unit": "ns/iter",
            "extra": "iterations: 3211\ncpu: 210543.11616318903 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 305661.7980017387,
            "unit": "ns/iter",
            "extra": "iterations: 2302\ncpu: 305558.8275412685 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1772962.66165408,
            "unit": "ns/iter",
            "extra": "iterations: 399\ncpu: 1772524.5814536337 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 19010788.405405432,
            "unit": "ns/iter",
            "extra": "iterations: 37\ncpu: 19007439.83783784 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1216.331408200499,
            "unit": "ns/iter",
            "extra": "iterations: 575891\ncpu: 1216.260908748357 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12058.463071957518,
            "unit": "ns/iter",
            "extra": "iterations: 57937\ncpu: 12057.144363705398 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 166794.2633928526,
            "unit": "ns/iter",
            "extra": "iterations: 4032\ncpu: 166778.1941964287 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 474980.60040844185,
            "unit": "ns/iter",
            "extra": "iterations: 1469\ncpu: 474839.4247787608 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 430390.16400491504,
            "unit": "ns/iter",
            "extra": "iterations: 1628\ncpu: 430351.69226044184 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 350656.3214644022,
            "unit": "ns/iter",
            "extra": "iterations: 1994\ncpu: 350583.3259779333 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 67415951.24999833,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 67413539.37499994 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20829.77607389522,
            "unit": "ns/iter",
            "extra": "iterations: 33453\ncpu: 20826.93193435565 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 393586.3583948406,
            "unit": "ns/iter",
            "extra": "iterations: 1769\ncpu: 393415.0559638659 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "ddae3e04c871550edf13ef6cc3c7e6e085ff3fa4",
          "message": "first version of multi clock related terms (#426)",
          "timestamp": "2026-08-09T22:53:45+02:00",
          "tree_id": "bb4e738e240ce4339e85811f5b4db39943de959a",
          "url": "https://github.com/najaeda/naja/commit/ddae3e04c871550edf13ef6cc3c7e6e085ff3fa4"
        },
        "date": 1786308964663,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 200273.18786464457,
            "unit": "ns/iter",
            "extra": "iterations: 3428\ncpu: 200271.00554259046 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 294640.3906250075,
            "unit": "ns/iter",
            "extra": "iterations: 2368\ncpu: 294614.0777027028 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1665809.0909091397,
            "unit": "ns/iter",
            "extra": "iterations: 407\ncpu: 1665650.0933660935 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18394109.94871826,
            "unit": "ns/iter",
            "extra": "iterations: 39\ncpu: 18391226.28205128 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1312.316915737939,
            "unit": "ns/iter",
            "extra": "iterations: 533716\ncpu: 1311.7675055647576 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12902.889085512892,
            "unit": "ns/iter",
            "extra": "iterations: 54249\ncpu: 12900.07279396855 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 136197.51825429665,
            "unit": "ns/iter",
            "extra": "iterations: 4766\ncpu: 136172.76856903074 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 388891.30543236696,
            "unit": "ns/iter",
            "extra": "iterations: 1804\ncpu: 388797.64800443465 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 345531.93473992933,
            "unit": "ns/iter",
            "extra": "iterations: 2038\ncpu: 345443.4931305205 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 281513.0734693853,
            "unit": "ns/iter",
            "extra": "iterations: 2450\ncpu: 281482.8424489793 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 69154728.2500018,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 69145666.87500012 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 21913.9895803997,
            "unit": "ns/iter",
            "extra": "iterations: 31959\ncpu: 21911.702587690485 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 388044.00166277937,
            "unit": "ns/iter",
            "extra": "iterations: 1804\ncpu: 387932.1762749374 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "83f3c95d0d26438240ee852e6d5b2ddd0e2bd9cb",
          "message": "allow : in liberty identifiers (#428)",
          "timestamp": "2026-08-12T11:47:40+02:00",
          "tree_id": "8e368b423ecdf7e27915a0468647a5181f735975",
          "url": "https://github.com/najaeda/naja/commit/83f3c95d0d26438240ee852e6d5b2ddd0e2bd9cb"
        },
        "date": 1786528210616,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 200428.3836734635,
            "unit": "ns/iter",
            "extra": "iterations: 3430\ncpu: 200400.51632653063 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 294807.11345423607,
            "unit": "ns/iter",
            "extra": "iterations: 2371\ncpu: 294700.40615773946 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1698402.1229314753,
            "unit": "ns/iter",
            "extra": "iterations: 423\ncpu: 1697846.1583924356 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18642266.749999695,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 18637899.20000001 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1336.0675565133436,
            "unit": "ns/iter",
            "extra": "iterations: 531777\ncpu: 1335.7825780355295 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12896.857876428696,
            "unit": "ns/iter",
            "extra": "iterations: 54333\ncpu: 12894.861299762568 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 142162.55502582644,
            "unit": "ns/iter",
            "extra": "iterations: 5034\ncpu: 142135.32061978546 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 436601.22134626895,
            "unit": "ns/iter",
            "extra": "iterations: 1649\ncpu: 436417.54639175243 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 383091.6269972513,
            "unit": "ns/iter",
            "extra": "iterations: 1815\ncpu: 382988.0953168041 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 318057.7964480905,
            "unit": "ns/iter",
            "extra": "iterations: 2196\ncpu: 317997.85701275035 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 67484017.5,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 67468439.50000003 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 21966.454140351692,
            "unit": "ns/iter",
            "extra": "iterations: 31193\ncpu: 21963.47132369446 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 377152.127302021,
            "unit": "ns/iter",
            "extra": "iterations: 1846\ncpu: 377052.9582881963 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "cbc6c3dc3f703f7ed28e7f47d88642f3ae4dd34d",
          "message": "contextually accepts :, !, and & (#429)",
          "timestamp": "2026-08-20T19:49:45+02:00",
          "tree_id": "b07b8b9657de322bbe0d3d30caebbae21d4eeab6",
          "url": "https://github.com/najaeda/naja/commit/cbc6c3dc3f703f7ed28e7f47d88642f3ae4dd34d"
        },
        "date": 1787248341849,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 225964.97328724276,
            "unit": "ns/iter",
            "extra": "iterations: 3182\ncpu: 225942.8573224387 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 305030.33116600604,
            "unit": "ns/iter",
            "extra": "iterations: 2307\ncpu: 304941.18378846993 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1698687.2968369706,
            "unit": "ns/iter",
            "extra": "iterations: 411\ncpu: 1698230.8467153283 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 16502277.162790367,
            "unit": "ns/iter",
            "extra": "iterations: 43\ncpu: 16499877.604651174 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1217.4458954704758,
            "unit": "ns/iter",
            "extra": "iterations: 575072\ncpu: 1217.2560027266154 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12093.824330630778,
            "unit": "ns/iter",
            "extra": "iterations: 57853\ncpu: 12091.874578673534 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 192460.5225464151,
            "unit": "ns/iter",
            "extra": "iterations: 4147\ncpu: 192436.05449722684 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 479177.8099173698,
            "unit": "ns/iter",
            "extra": "iterations: 1452\ncpu: 479105.3643250685 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 433144.5918367359,
            "unit": "ns/iter",
            "extra": "iterations: 1617\ncpu: 433100.45640074206 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 354463.5825734491,
            "unit": "ns/iter",
            "extra": "iterations: 1974\ncpu: 354412.5116514689 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 66810068.666664325,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 66801550 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20869.709645727962,
            "unit": "ns/iter",
            "extra": "iterations: 33590\ncpu: 20867.376838344768 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 397923.61165753164,
            "unit": "ns/iter",
            "extra": "iterations: 1733\ncpu: 397766.6699365183 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "42012e809ec061665fe90fbef9829dc7208efca4",
          "message": "Fix automatic dynamic select (#431)\n\n* Fix automatic dynamic select\n\n* bump naja version to 0.7.21",
          "timestamp": "2026-08-26T11:38:49+02:00",
          "tree_id": "7d8197fb84eb71406a09110cedf702b8d6dbc6a2",
          "url": "https://github.com/najaeda/naja/commit/42012e809ec061665fe90fbef9829dc7208efca4"
        },
        "date": 1787738013296,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 216902.82655246876,
            "unit": "ns/iter",
            "extra": "iterations: 3269\ncpu: 216890.78831446925 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 305668.6148148185,
            "unit": "ns/iter",
            "extra": "iterations: 2295\ncpu: 305572.31154684105 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1702491.5269607722,
            "unit": "ns/iter",
            "extra": "iterations: 408\ncpu: 1702293.8431372547 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17184013.780487612,
            "unit": "ns/iter",
            "extra": "iterations: 41\ncpu: 17181376.92682927 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1218.5849770468224,
            "unit": "ns/iter",
            "extra": "iterations: 574867\ncpu: 1218.5142006759822 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12062.280855165645,
            "unit": "ns/iter",
            "extra": "iterations: 58094\ncpu: 12059.682015354432 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 158794.26699833455,
            "unit": "ns/iter",
            "extra": "iterations: 4221\ncpu: 158787.74176735381 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 473569.41204332997,
            "unit": "ns/iter",
            "extra": "iterations: 1478\ncpu: 473517.56833558856 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 429262.6640146894,
            "unit": "ns/iter",
            "extra": "iterations: 1634\ncpu: 429230.1456548353 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 350532.65084915323,
            "unit": "ns/iter",
            "extra": "iterations: 2002\ncpu: 350492.1143856149 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 67702395.3333307,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 67696255.3333333 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20777.517768005284,
            "unit": "ns/iter",
            "extra": "iterations: 33656\ncpu: 20774.63979676731 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 399399.69379561435,
            "unit": "ns/iter",
            "extra": "iterations: 1757\ncpu: 399228.24473536504 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "b03d4046d5454348e0ddacd369154c22a9d24d8b",
          "message": "add a mode traversing assigns in equipotential (#433)",
          "timestamp": "2026-08-30T20:46:27+02:00",
          "tree_id": "a734183cf417db01f52d7f6d648f3a4f21129316",
          "url": "https://github.com/najaeda/naja/commit/b03d4046d5454348e0ddacd369154c22a9d24d8b"
        },
        "date": 1788115742369,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 213114.8400362878,
            "unit": "ns/iter",
            "extra": "iterations: 3307\ncpu: 213074.2089507106 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 303764.3977420765,
            "unit": "ns/iter",
            "extra": "iterations: 2303\ncpu: 303680.3812418584 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1753777.5560976025,
            "unit": "ns/iter",
            "extra": "iterations: 410\ncpu: 1753331.963414634 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17871159.575000204,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17870252.499999993 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1203.432160483287,
            "unit": "ns/iter",
            "extra": "iterations: 579699\ncpu: 1203.3332522567741 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 11962.322424324766,
            "unit": "ns/iter",
            "extra": "iterations: 58606\ncpu: 11961.452069753945 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 180726.94123738303,
            "unit": "ns/iter",
            "extra": "iterations: 3863\ncpu: 180695.90111312442 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 478726.04430811707,
            "unit": "ns/iter",
            "extra": "iterations: 1467\ncpu: 478695.0259032042 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 428407.676488647,
            "unit": "ns/iter",
            "extra": "iterations: 1629\ncpu: 428377.30509515025 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 351604.30761522835,
            "unit": "ns/iter",
            "extra": "iterations: 1996\ncpu: 351559.02054108237 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 67240647.77777825,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 67231298.77777761 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20619.98161840587,
            "unit": "ns/iter",
            "extra": "iterations: 33947\ncpu: 20617.167790968288 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 399518.4877358433,
            "unit": "ns/iter",
            "extra": "iterations: 1753\ncpu: 399357.66913862067 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "f0080c536f641a6d961aede806766ef89d3c52a4",
          "message": "export traverse_assigns mode at python level (#435)\n\n* export traverse_assigns mode at python level\n\n* cleaning and coverage",
          "timestamp": "2026-08-31T16:24:03+02:00",
          "tree_id": "270e38b9c137936aa14e97d0c4755c08a1970d4a",
          "url": "https://github.com/najaeda/naja/commit/f0080c536f641a6d961aede806766ef89d3c52a4"
        },
        "date": 1788186387836,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 189463.74854142088,
            "unit": "ns/iter",
            "extra": "iterations: 3428\ncpu: 189443.79550758464 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 284777.8741935468,
            "unit": "ns/iter",
            "extra": "iterations: 2480\ncpu: 284677.1568548387 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1686950.148584931,
            "unit": "ns/iter",
            "extra": "iterations: 424\ncpu: 1686454.3820754718 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 16705360.682926878,
            "unit": "ns/iter",
            "extra": "iterations: 41\ncpu: 16702996.829268284 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1297.7956684486035,
            "unit": "ns/iter",
            "extra": "iterations: 539114\ncpu: 1297.6707041553361 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12778.750588664563,
            "unit": "ns/iter",
            "extra": "iterations: 54785\ncpu: 12775.843351282287 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 137373.2632097826,
            "unit": "ns/iter",
            "extra": "iterations: 5072\ncpu: 137353.11652208192 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 421792.8186746915,
            "unit": "ns/iter",
            "extra": "iterations: 1660\ncpu: 421736.1807228914 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 382707.8639085364,
            "unit": "ns/iter",
            "extra": "iterations: 1837\ncpu: 382669.3658138272 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 318679.6983189464,
            "unit": "ns/iter",
            "extra": "iterations: 2201\ncpu: 318640.0009086786 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 69518737.50000104,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 69508902.49999997 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 21814.35758518718,
            "unit": "ns/iter",
            "extra": "iterations: 32135\ncpu: 21810.6947253773 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 386781.95138043666,
            "unit": "ns/iter",
            "extra": "iterations: 1810\ncpu: 386742.36906075914 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "committer": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "distinct": true,
          "id": "fb56d004a8dbfa7b083cfcf186fe7698606e9420",
          "message": "permit coverage for PRs",
          "timestamp": "2026-09-01T14:22:10+02:00",
          "tree_id": "bbc48ba57f78000a8aa2648261c09083b0a0e91d",
          "url": "https://github.com/najaeda/naja/commit/fb56d004a8dbfa7b083cfcf186fe7698606e9420"
        },
        "date": 1788265484681,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 218848.82296203944,
            "unit": "ns/iter",
            "extra": "iterations: 3214\ncpu: 218801.86807716242 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 308169.96654929814,
            "unit": "ns/iter",
            "extra": "iterations: 2272\ncpu: 308142.400528169 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1711195.8722358386,
            "unit": "ns/iter",
            "extra": "iterations: 407\ncpu: 1710934.545454546 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 16755258.428571686,
            "unit": "ns/iter",
            "extra": "iterations: 42\ncpu: 16753206.380952379 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1204.2745804347574,
            "unit": "ns/iter",
            "extra": "iterations: 581316\ncpu: 1204.0842760219925 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 11919.156839040494,
            "unit": "ns/iter",
            "extra": "iterations: 58729\ncpu: 11917.176454562474 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 186313.1198268933,
            "unit": "ns/iter",
            "extra": "iterations: 3697\ncpu: 186290.60129835008 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 476501.23569482035,
            "unit": "ns/iter",
            "extra": "iterations: 1468\ncpu: 476458.77588555875 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 430296.9279999966,
            "unit": "ns/iter",
            "extra": "iterations: 1625\ncpu: 430223.3113846151 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 353028.1867801585,
            "unit": "ns/iter",
            "extra": "iterations: 1997\ncpu: 353003.50625938916 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 68454198.55555531,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 68444897.33333327 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20588.313156341934,
            "unit": "ns/iter",
            "extra": "iterations: 33900\ncpu: 20587.31743362832 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 400203.1617132421,
            "unit": "ns/iter",
            "extra": "iterations: 1750\ncpu: 400120.55828574335 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "noam.chn1@gmail.com",
            "name": "Noam Cohen",
            "username": "nanocoh"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "b995833c7242f167134461c19b0521556047a068",
          "message": "Enhance python api + parameter truth table + liberty fix (#427)\n\n* Model divmod outputs with generic truth tables\n\n* Add Python sequential model API\n\n* Add instance-parameterized truth tables\n\n* Share Naja runtime with Python extension in Bazel\n\n* Fix Bazel Python test runtime loading\n\n* Link TBB into Bazel shared runtime\n\n* Keep mixed FF/latch Liberty cells loadable\n\n* Fix Liberty state cells getting constant truth tables\n\n* Preserve timing for opaque Liberty state cells\n\n* test: improve Naja API coverage\n\n* test: cover remaining modeling branches",
          "timestamp": "2026-09-01T19:49:45+02:00",
          "tree_id": "15211a8f44fa49581900a8674c0b305d5a798511",
          "url": "https://github.com/najaeda/naja/commit/b995833c7242f167134461c19b0521556047a068"
        },
        "date": 1788285133221,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 215279.52140198788,
            "unit": "ns/iter",
            "extra": "iterations: 3224\ncpu: 215249.2186724566 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 306973.50680122117,
            "unit": "ns/iter",
            "extra": "iterations: 2279\ncpu: 306927.8670469504 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1812525.6497462303,
            "unit": "ns/iter",
            "extra": "iterations: 394\ncpu: 1812223.0355329947 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17178097.102563817,
            "unit": "ns/iter",
            "extra": "iterations: 39\ncpu: 17175692.461538464 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1244.0182558784657,
            "unit": "ns/iter",
            "extra": "iterations: 560477\ncpu: 1243.9085832246458 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12294.424830533586,
            "unit": "ns/iter",
            "extra": "iterations: 57091\ncpu: 12292.197491723742 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 157985.2697339737,
            "unit": "ns/iter",
            "extra": "iterations: 4586\ncpu: 157960.33515045795 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 478214.54135339375,
            "unit": "ns/iter",
            "extra": "iterations: 1463\ncpu: 478132.16951469553 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 431783.87708460586,
            "unit": "ns/iter",
            "extra": "iterations: 1619\ncpu: 431766.0420012354 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 352909.35169277777,
            "unit": "ns/iter",
            "extra": "iterations: 1979\ncpu: 352856.4926730668 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 67392147.22222187,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 67390915.66666666 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20193.340861298908,
            "unit": "ns/iter",
            "extra": "iterations: 34715\ncpu: 20189.40265015124 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 398531.2094483188,
            "unit": "ns/iter",
            "extra": "iterations: 1757\ncpu: 398368.6260671168 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "972c83765425088c521fe3ac22594e2ed22feb82",
          "message": "remove unused intermediate net (#436)\n\n* remove unused intermediate net\n\n* fix and more testing",
          "timestamp": "2026-09-03T11:29:12+02:00",
          "tree_id": "ae6a4658502439287568b113a61b77141966cc98",
          "url": "https://github.com/najaeda/naja/commit/972c83765425088c521fe3ac22594e2ed22feb82"
        },
        "date": 1788427905103,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 212170.9294990774,
            "unit": "ns/iter",
            "extra": "iterations: 3234\ncpu: 212144.50711193567 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 305450.7447869349,
            "unit": "ns/iter",
            "extra": "iterations: 2206\ncpu: 305414.74116047146 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1743716.5571776938,
            "unit": "ns/iter",
            "extra": "iterations: 411\ncpu: 1743439.0559610703 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 16993919.575000405,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 16991630.575000007 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1245.751913748687,
            "unit": "ns/iter",
            "extra": "iterations: 561986\ncpu: 1245.659628531672 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12314.031871181003,
            "unit": "ns/iter",
            "extra": "iterations: 56948\ncpu: 12312.400523284401 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 166474.72185776875,
            "unit": "ns/iter",
            "extra": "iterations: 3811\ncpu: 166463.66596693784 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 483790.44724302297,
            "unit": "ns/iter",
            "extra": "iterations: 1469\ncpu: 483710.7590197417 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 429036.9761759168,
            "unit": "ns/iter",
            "extra": "iterations: 1637\ncpu: 428999.83934025734 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 351201.96600001917,
            "unit": "ns/iter",
            "extra": "iterations: 2000\ncpu: 351166.50450000004 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 67726123.33333604,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 67721529.66666667 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20170.507104769484,
            "unit": "ns/iter",
            "extra": "iterations: 34695\ncpu: 20169.033088341268 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 400834.8883733992,
            "unit": "ns/iter",
            "extra": "iterations: 1720\ncpu: 400653.16918604774 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "8fe8233faffe2371dec264efd732394bbc3bf735",
          "message": "Remove non useful intermediate nets (#437)\n\n* Remove non useful intermediate nets\n\n* update regress count and coverage",
          "timestamp": "2026-09-03T20:17:32+02:00",
          "tree_id": "94b217e534578c9e5a804b8b8f0057e791e9e9df",
          "url": "https://github.com/najaeda/naja/commit/8fe8233faffe2371dec264efd732394bbc3bf735"
        },
        "date": 1788459622331,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 216319.5919450507,
            "unit": "ns/iter",
            "extra": "iterations: 3203\ncpu: 216307.35622853576 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 309888.40719362,
            "unit": "ns/iter",
            "extra": "iterations: 2252\ncpu: 309833.03108348127 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1756830.2315270235,
            "unit": "ns/iter",
            "extra": "iterations: 406\ncpu: 1756643.7389162562 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17956599.769231383,
            "unit": "ns/iter",
            "extra": "iterations: 39\ncpu: 17954207.615384612 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1259.490214993731,
            "unit": "ns/iter",
            "extra": "iterations: 560807\ncpu: 1259.4250517557741 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12307.615054293752,
            "unit": "ns/iter",
            "extra": "iterations: 56821\ncpu: 12305.603861248492 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 176817.39029536262,
            "unit": "ns/iter",
            "extra": "iterations: 3792\ncpu: 176800.45490506315 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 478921.56361149193,
            "unit": "ns/iter",
            "extra": "iterations: 1462\ncpu: 478820.5950752393 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 433380.49287041393,
            "unit": "ns/iter",
            "extra": "iterations: 1613\ncpu: 433328.5995040304 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 354235.6283408784,
            "unit": "ns/iter",
            "extra": "iterations: 1983\ncpu: 354150.1235501772 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 68603323.77777922,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 68597819.3333333 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20199.695483089356,
            "unit": "ns/iter",
            "extra": "iterations: 34714\ncpu: 20198.0842887596 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 404485.9757077959,
            "unit": "ns/iter",
            "extra": "iterations: 1729\ncpu: 404317.3776749553 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "committer": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "distinct": true,
          "id": "60fb06446910c0e1f7d9bd89e722056748f24529",
          "message": "bump najaeda version to 0.7.22",
          "timestamp": "2026-09-04T10:21:01+02:00",
          "tree_id": "5f0cecc236cd5a3e9502f5fde9223e92d6f1ebc6",
          "url": "https://github.com/najaeda/naja/commit/60fb06446910c0e1f7d9bd89e722056748f24529"
        },
        "date": 1788510229459,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 211926.33892616918,
            "unit": "ns/iter",
            "extra": "iterations: 3278\ncpu: 211897.7230018304 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 303975.7105719228,
            "unit": "ns/iter",
            "extra": "iterations: 2308\ncpu: 303959.62824956665 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1732558.1638141593,
            "unit": "ns/iter",
            "extra": "iterations: 409\ncpu: 1732432.7530562354 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 16642468.51219548,
            "unit": "ns/iter",
            "extra": "iterations: 41\ncpu: 16640475.731707323 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1272.7003936475655,
            "unit": "ns/iter",
            "extra": "iterations: 561924\ncpu: 1272.6247659825885 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12338.122390741622,
            "unit": "ns/iter",
            "extra": "iterations: 56769\ncpu: 12336.563018548855 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 179760.60689968074,
            "unit": "ns/iter",
            "extra": "iterations: 4406\ncpu: 179747.31093962782 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 469332.5550335511,
            "unit": "ns/iter",
            "extra": "iterations: 1490\ncpu: 469262.5255033561 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 423563.6332326374,
            "unit": "ns/iter",
            "extra": "iterations: 1655\ncpu: 423528.3287009062 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 345204.0598200111,
            "unit": "ns/iter",
            "extra": "iterations: 1889\ncpu: 345182.6680783486 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 67930692.11111122,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 67924555.1111111 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20170.32311913359,
            "unit": "ns/iter",
            "extra": "iterations: 34625\ncpu: 20168.68153068591 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 394283.37134900695,
            "unit": "ns/iter",
            "extra": "iterations: 1780\ncpu: 394147.54719099886 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "44a7ae0888dba691a42f0b94cf64fd41be36efcc",
          "message": "fix 349 (#440)",
          "timestamp": "2026-09-07T14:57:16+02:00",
          "tree_id": "fe8763c2b912afb420c56498a390eb046939ede9",
          "url": "https://github.com/najaeda/naja/commit/44a7ae0888dba691a42f0b94cf64fd41be36efcc"
        },
        "date": 1788785987351,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 211592.7064220237,
            "unit": "ns/iter",
            "extra": "iterations: 3270\ncpu: 211541.97706422018 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 303604.06802426954,
            "unit": "ns/iter",
            "extra": "iterations: 2308\ncpu: 303547.1936741767 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1743391.0891566302,
            "unit": "ns/iter",
            "extra": "iterations: 415\ncpu: 1742927.7734939768 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 16775995.85365797,
            "unit": "ns/iter",
            "extra": "iterations: 41\ncpu: 16775424.219512194 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1245.0609401357822,
            "unit": "ns/iter",
            "extra": "iterations: 561387\ncpu: 1244.797323415041 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12339.147861090245,
            "unit": "ns/iter",
            "extra": "iterations: 56641\ncpu: 12338.607969492068 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 168606.71353040056,
            "unit": "ns/iter",
            "extra": "iterations: 4161\ncpu: 168587.92742129305 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 472569.36246611684,
            "unit": "ns/iter",
            "extra": "iterations: 1476\ncpu: 472492.9058265578 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 425705.6615946491,
            "unit": "ns/iter",
            "extra": "iterations: 1643\ncpu: 425661.1552038951 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 348261.2724103557,
            "unit": "ns/iter",
            "extra": "iterations: 2008\ncpu: 348229.98207171325 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 67447103.66666596,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 67437012.2222224 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20171.024924449306,
            "unit": "ns/iter",
            "extra": "iterations: 34745\ncpu: 20170.31549863289 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 398293.9256939326,
            "unit": "ns/iter",
            "extra": "iterations: 1763\ncpu: 398200.7623369615 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "committer": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "distinct": true,
          "id": "2a8ff767d8f31819644c2552793d71b6815164c9",
          "message": "bump naja version to 0.7.23",
          "timestamp": "2026-09-07T15:02:34+02:00",
          "tree_id": "40ebf767bd94960025cb236398a55f03f07fe350",
          "url": "https://github.com/najaeda/naja/commit/2a8ff767d8f31819644c2552793d71b6815164c9"
        },
        "date": 1788787997184,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 150950.81667036112,
            "unit": "ns/iter",
            "extra": "iterations: 4511\ncpu: 150846.4384837065 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 218214.89460554943,
            "unit": "ns/iter",
            "extra": "iterations: 3207\ncpu: 218174.31618334897 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1265209.062500037,
            "unit": "ns/iter",
            "extra": "iterations: 544\ncpu: 1264923.8805147063 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 13481103.299999971,
            "unit": "ns/iter",
            "extra": "iterations: 50\ncpu: 13479068.26 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1047.7944930527708,
            "unit": "ns/iter",
            "extra": "iterations: 668610\ncpu: 1047.6905206323563 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 10253.505922141028,
            "unit": "ns/iter",
            "extra": "iterations: 68303\ncpu: 10246.517341844425 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 108940.79056137995,
            "unit": "ns/iter",
            "extra": "iterations: 6484\ncpu: 108921.66363355963 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 297188.9931769716,
            "unit": "ns/iter",
            "extra": "iterations: 2345\ncpu: 297154.36204690824 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 263060.22372372384,
            "unit": "ns/iter",
            "extra": "iterations: 2664\ncpu: 263033.2023273275 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 216379.22405876595,
            "unit": "ns/iter",
            "extra": "iterations: 3267\ncpu: 216350.5702479339 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 52459914.454547375,
            "unit": "ns/iter",
            "extra": "iterations: 11\ncpu: 52450221.90909087 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 16706.227070071753,
            "unit": "ns/iter",
            "extra": "iterations: 41943\ncpu: 16703.510120878338 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 295220.45947155316,
            "unit": "ns/iter",
            "extra": "iterations: 2381\ncpu: 295301.56866864505 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "3d7d3c9c3b8fdb879d098bc4d9ea3a8ab71752a4",
          "message": "add errors and documentation (#441)\n\n* add errors and documentation\n\n* Fix tests\n\n* bump gtest version",
          "timestamp": "2026-09-09T13:19:47+02:00",
          "tree_id": "73a5e374232d88d81850a5d5ad2a86d6fdac22f5",
          "url": "https://github.com/najaeda/naja/commit/3d7d3c9c3b8fdb879d098bc4d9ea3a8ab71752a4"
        },
        "date": 1788952933751,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 212835.98899419067,
            "unit": "ns/iter",
            "extra": "iterations: 3271\ncpu: 212825.7114032406 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 306479.52753497276,
            "unit": "ns/iter",
            "extra": "iterations: 2288\ncpu: 306450.3225524475 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1723620.7346437648,
            "unit": "ns/iter",
            "extra": "iterations: 407\ncpu: 1723282.464373465 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17227576.625000294,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17225222.674999997 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1221.5775345108814,
            "unit": "ns/iter",
            "extra": "iterations: 574312\ncpu: 1221.2953046427724 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12131.581928839378,
            "unit": "ns/iter",
            "extra": "iterations: 57672\ncpu: 12129.81467609932 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 173769.55704189782,
            "unit": "ns/iter",
            "extra": "iterations: 3962\ncpu: 173751.10676426053 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 465808.2624161088,
            "unit": "ns/iter",
            "extra": "iterations: 1490\ncpu: 465781.7161073824 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 419664.3435068796,
            "unit": "ns/iter",
            "extra": "iterations: 1671\ncpu: 419628.110712149 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 343920.19754300494,
            "unit": "ns/iter",
            "extra": "iterations: 2035\ncpu: 343895.0997542993 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 67198891.11111064,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 67188006.88888887 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20316.627089621088,
            "unit": "ns/iter",
            "extra": "iterations: 34456\ncpu: 20314.75467262596 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 389546.4966560205,
            "unit": "ns/iter",
            "extra": "iterations: 1794\ncpu: 389378.7892976679 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "1a70e1896985f29d26e4eb751cab3a4422f94be8",
          "message": "upgrade naja verilog signed nets (#442)",
          "timestamp": "2026-09-09T16:55:24+02:00",
          "tree_id": "ab98def837e7f6cbf1fc7ee5e96f689996a1dd95",
          "url": "https://github.com/najaeda/naja/commit/1a70e1896985f29d26e4eb751cab3a4422f94be8"
        },
        "date": 1788966662105,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 183393.9775251184,
            "unit": "ns/iter",
            "extra": "iterations: 3782\ncpu: 183381.96536224222 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 285382.8611217147,
            "unit": "ns/iter",
            "extra": "iterations: 2621\ncpu: 285326.9297977871 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1674928.7535885475,
            "unit": "ns/iter",
            "extra": "iterations: 418\ncpu: 1674452.1196172258 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 16943821.102564435,
            "unit": "ns/iter",
            "extra": "iterations: 39\ncpu: 16942103.769230776 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1081.1115675639758,
            "unit": "ns/iter",
            "extra": "iterations: 658946\ncpu: 1081.0935888525014 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 11373.379118019948,
            "unit": "ns/iter",
            "extra": "iterations: 61498\ncpu: 11373.468437997988 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 108947.12010359248,
            "unit": "ns/iter",
            "extra": "iterations: 6178\ncpu: 108941.82648106181 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 416577.019721575,
            "unit": "ns/iter",
            "extra": "iterations: 1724\ncpu: 416562.41067285364 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 366327.9494845306,
            "unit": "ns/iter",
            "extra": "iterations: 1940\ncpu: 366321.2742268039 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 286854.4692556588,
            "unit": "ns/iter",
            "extra": "iterations: 2472\ncpu: 286833.72289643995 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 64597342.444446646,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 64593833.33333334 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 22586.387777053074,
            "unit": "ns/iter",
            "extra": "iterations: 30680\ncpu: 22585.96998044325 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 299362.23051157006,
            "unit": "ns/iter",
            "extra": "iterations: 2386\ncpu: 299277.1810561655 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "8d01f457de9fd1225db5f0ea0462e27dc81ed9a2",
          "message": "switch to upstream slang (#443)",
          "timestamp": "2026-09-10T11:16:27+02:00",
          "tree_id": "8078604d912948c873543cab74905c076c42e136",
          "url": "https://github.com/najaeda/naja/commit/8d01f457de9fd1225db5f0ea0462e27dc81ed9a2"
        },
        "date": 1789031937052,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 194023.1629507246,
            "unit": "ns/iter",
            "extra": "iterations: 3633\ncpu: 194002.86815304158 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 280721.63809911325,
            "unit": "ns/iter",
            "extra": "iterations: 2462\ncpu: 280669.0938261576 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1685057.926014273,
            "unit": "ns/iter",
            "extra": "iterations: 419\ncpu: 1684458.5107398576 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17958975.524999943,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17954920.625000004 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1319.8449684862708,
            "unit": "ns/iter",
            "extra": "iterations: 528499\ncpu: 1319.569450462536 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 13016.545762837999,
            "unit": "ns/iter",
            "extra": "iterations: 53668\ncpu: 13012.632406648287 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 145929.07609577532,
            "unit": "ns/iter",
            "extra": "iterations: 4928\ncpu: 145893.22260551964 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 431414.9266337727,
            "unit": "ns/iter",
            "extra": "iterations: 1622\ncpu: 431247.55980271235 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 387327.45173744764,
            "unit": "ns/iter",
            "extra": "iterations: 1813\ncpu: 387257.4715940432 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 321349.54118730593,
            "unit": "ns/iter",
            "extra": "iterations: 2173\ncpu: 321275.96732627717 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 67724580.87500155,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 67709700.125 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 21396.928827242573,
            "unit": "ns/iter",
            "extra": "iterations: 32948\ncpu: 21393.502185261605 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 390193.29477143113,
            "unit": "ns/iter",
            "extra": "iterations: 1798\ncpu: 390069.06006677763 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "7a4b82631aa5a2f41fd8b94bc218c5e5f034cc1a",
          "message": "Regress borg gpu (#444)\n\n* adding borg GPU in regress and restructuring regress\n\n* borg in central regress",
          "timestamp": "2026-09-14T06:54:13+02:00",
          "tree_id": "65cb0ef1ced06643a6df700df3cd903fcb945b24",
          "url": "https://github.com/najaeda/naja/commit/7a4b82631aa5a2f41fd8b94bc218c5e5f034cc1a"
        },
        "date": 1789361804540,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 193569.64925170245,
            "unit": "ns/iter",
            "extra": "iterations: 3675\ncpu: 193563.47020408162 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 287185.06783467496,
            "unit": "ns/iter",
            "extra": "iterations: 2226\ncpu: 287121.58265947894 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1683248.419811326,
            "unit": "ns/iter",
            "extra": "iterations: 424\ncpu: 1682856.5825471699 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 19367294.21621644,
            "unit": "ns/iter",
            "extra": "iterations: 37\ncpu: 19364726.10810811 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1009.3041724113291,
            "unit": "ns/iter",
            "extra": "iterations: 689050\ncpu: 1009.2514607067695 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 10443.666888348032,
            "unit": "ns/iter",
            "extra": "iterations: 66161\ncpu: 10442.239083447957 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 105879.56240346031,
            "unit": "ns/iter",
            "extra": "iterations: 6474\ncpu: 105867.67068273085 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 424029.2241691905,
            "unit": "ns/iter",
            "extra": "iterations: 1655\ncpu: 423967.88277945627 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 380111.29300054477,
            "unit": "ns/iter",
            "extra": "iterations: 1843\ncpu: 380094.8730330989 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 306484.04292597383,
            "unit": "ns/iter",
            "extra": "iterations: 2283\ncpu: 306444.4244415243 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 67383231.77777754,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 67374118.44444433 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20776.057015590453,
            "unit": "ns/iter",
            "extra": "iterations: 33675\ncpu: 20774.895590200445 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 310984.93006641575,
            "unit": "ns/iter",
            "extra": "iterations: 2245\ncpu: 310915.63474387134 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "3c9a5bb7de91738744c6b91183c6fc6b8528043f",
          "message": "Fix the SystemVerilog table-select lowering (#446)",
          "timestamp": "2026-09-14T06:54:40+02:00",
          "tree_id": "d940f461b6ac824ac20fb704b23bf5d5f5c6c044",
          "url": "https://github.com/najaeda/naja/commit/3c9a5bb7de91738744c6b91183c6fc6b8528043f"
        },
        "date": 1789362539644,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 198844.69733372735,
            "unit": "ns/iter",
            "extra": "iterations: 3413\ncpu: 198834.51772634048 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 298927.8041958052,
            "unit": "ns/iter",
            "extra": "iterations: 2288\ncpu: 298878.08872377616 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1721414.8212560855,
            "unit": "ns/iter",
            "extra": "iterations: 414\ncpu: 1721003.20531401 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17534254.268292114,
            "unit": "ns/iter",
            "extra": "iterations: 41\ncpu: 17529229.268292684 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1319.711442579788,
            "unit": "ns/iter",
            "extra": "iterations: 530501\ncpu: 1319.520675738594 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 13011.78946197499,
            "unit": "ns/iter",
            "extra": "iterations: 53938\ncpu: 13008.740109014048 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 138499.74857839462,
            "unit": "ns/iter",
            "extra": "iterations: 4924\ncpu: 138476.71852152722 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 383580.57158590155,
            "unit": "ns/iter",
            "extra": "iterations: 1816\ncpu: 383490.54625550663 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 343361.2763157878,
            "unit": "ns/iter",
            "extra": "iterations: 2052\ncpu: 343212.91179337184 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 281634.76748808037,
            "unit": "ns/iter",
            "extra": "iterations: 2516\ncpu: 281585.2094594594 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 68179793.50000058,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 68165981.37499996 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 21071.60872852658,
            "unit": "ns/iter",
            "extra": "iterations: 33064\ncpu: 21068.491259375776 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 385288.985099715,
            "unit": "ns/iter",
            "extra": "iterations: 1812\ncpu: 385136.83940394863 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "767f7cc4cc7e5149596cb2a1bcec6a31d44ff7a9",
          "message": "elaboration optimization (#445)\n\n* elaboration optimization\n\n* update golden numbers\n\n* coverage",
          "timestamp": "2026-09-14T16:50:08+02:00",
          "tree_id": "e77dc415cd3e2248ac2936205d238cd0396913fe",
          "url": "https://github.com/najaeda/naja/commit/767f7cc4cc7e5149596cb2a1bcec6a31d44ff7a9"
        },
        "date": 1789397542029,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 153357.16784765018,
            "unit": "ns/iter",
            "extra": "iterations: 4516\ncpu: 153298.73117803366 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 233821.94632860902,
            "unit": "ns/iter",
            "extra": "iterations: 3037\ncpu: 233748.3207112282 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1286256.055133083,
            "unit": "ns/iter",
            "extra": "iterations: 526\ncpu: 1285749.6216730038 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 13399932.307692645,
            "unit": "ns/iter",
            "extra": "iterations: 52\ncpu: 13398353.749999998 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1057.9689673840182,
            "unit": "ns/iter",
            "extra": "iterations: 663141\ncpu: 1057.604510956192 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 10082.123897764764,
            "unit": "ns/iter",
            "extra": "iterations: 69291\ncpu: 10080.856272820423 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 111935.77374942879,
            "unit": "ns/iter",
            "extra": "iterations: 6537\ncpu: 111920.48508490142 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 331276.5466101668,
            "unit": "ns/iter",
            "extra": "iterations: 2124\ncpu: 331239.44679849327 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 298634.88884174364,
            "unit": "ns/iter",
            "extra": "iterations: 2357\ncpu: 298600.69198133214 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 247838.57613314566,
            "unit": "ns/iter",
            "extra": "iterations: 2824\ncpu: 247806.49114730908 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 52511582.27272609,
            "unit": "ns/iter",
            "extra": "iterations: 11\ncpu: 52506640.909090884 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 16373.31680022757,
            "unit": "ns/iter",
            "extra": "iterations: 42178\ncpu: 16370.714424581554 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 299490.2941173769,
            "unit": "ns/iter",
            "extra": "iterations: 2346\ncpu: 299381.2101449496 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "212641fe98d4e3e6d92224417d298ee5b20c4b93",
          "message": "Fix Verilog HEX padding (#447)",
          "timestamp": "2026-09-15T10:21:21+02:00",
          "tree_id": "521e4b942a7b266281f015f24316000c9fae1a5c",
          "url": "https://github.com/najaeda/naja/commit/212641fe98d4e3e6d92224417d298ee5b20c4b93"
        },
        "date": 1789461461872,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 152956.5951742613,
            "unit": "ns/iter",
            "extra": "iterations: 4476\ncpu: 152920.4486148347 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 223229.35872395863,
            "unit": "ns/iter",
            "extra": "iterations: 3072\ncpu: 223130.57812500003 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1288431.0627306479,
            "unit": "ns/iter",
            "extra": "iterations: 542\ncpu: 1288364.937269373 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 13523749.117647234,
            "unit": "ns/iter",
            "extra": "iterations: 51\ncpu: 13521779.901960788 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1053.804979914565,
            "unit": "ns/iter",
            "extra": "iterations: 664911\ncpu: 1053.6614253636947 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 10348.417225686453,
            "unit": "ns/iter",
            "extra": "iterations: 67678\ncpu: 10347.326442861782 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 110630.74885094917,
            "unit": "ns/iter",
            "extra": "iterations: 6092\ncpu: 110612.52708470139 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 323097.3186409586,
            "unit": "ns/iter",
            "extra": "iterations: 2178\ncpu: 323068.87373737345 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 290235.5717821813,
            "unit": "ns/iter",
            "extra": "iterations: 2424\ncpu: 290167.4393564357 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 240393.79621343387,
            "unit": "ns/iter",
            "extra": "iterations: 2905\ncpu: 240373.41239242692 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 52544066.99999948,
            "unit": "ns/iter",
            "extra": "iterations: 10\ncpu: 52533555.00000012 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 16381.656522664327,
            "unit": "ns/iter",
            "extra": "iterations: 42291\ncpu: 16380.182875789153 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 292290.8779680048,
            "unit": "ns/iter",
            "extra": "iterations: 2401\ncpu: 292384.17492712033 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "e82ec5bd5550dfee0242b1228c0541ebe3d82739",
          "message": "parallelize workflows (#448)",
          "timestamp": "2026-09-15T14:17:17+02:00",
          "tree_id": "eb5d74005c267714906400f432fc87c14c754db8",
          "url": "https://github.com/najaeda/naja/commit/e82ec5bd5550dfee0242b1228c0541ebe3d82739"
        },
        "date": 1789474770760,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 152197.71376107194,
            "unit": "ns/iter",
            "extra": "iterations: 4629\ncpu: 152188.66061784403 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 225864.36677419252,
            "unit": "ns/iter",
            "extra": "iterations: 3100\ncpu: 225812.68774193546 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1322831.422222203,
            "unit": "ns/iter",
            "extra": "iterations: 540\ncpu: 1322652.8407407403 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 13415894.819999607,
            "unit": "ns/iter",
            "extra": "iterations: 50\ncpu: 13414248.679999998 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1053.653892145145,
            "unit": "ns/iter",
            "extra": "iterations: 663169\ncpu: 1053.4121076226415 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 10362.491611908044,
            "unit": "ns/iter",
            "extra": "iterations: 67417\ncpu: 10361.592536007232 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 109794.06832989107,
            "unit": "ns/iter",
            "extra": "iterations: 6293\ncpu: 109786.11250595903 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 328045.762102355,
            "unit": "ns/iter",
            "extra": "iterations: 2169\ncpu: 328008.1526048873 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 294447.77967513836,
            "unit": "ns/iter",
            "extra": "iterations: 2401\ncpu: 294434.43648479786 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 240496.96197327742,
            "unit": "ns/iter",
            "extra": "iterations: 2919\ncpu: 240477.5710859888 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 52531744.45454595,
            "unit": "ns/iter",
            "extra": "iterations: 11\ncpu: 52528369.636363626 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 16422.26728477431,
            "unit": "ns/iter",
            "extra": "iterations: 42711\ncpu: 16421.236894476857 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 293387.36848769867,
            "unit": "ns/iter",
            "extra": "iterations: 2380\ncpu: 293323.70294114965 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "7d27662571e88c78ed301e0c6ed5a56eca2186d6",
          "message": "bump to 0.7.24 version (#449)",
          "timestamp": "2026-09-15T18:57:02+02:00",
          "tree_id": "08e1f45329bf3be38086eb1339f6360889eae14f",
          "url": "https://github.com/najaeda/naja/commit/7d27662571e88c78ed301e0c6ed5a56eca2186d6"
        },
        "date": 1789491570004,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 197988.2140593512,
            "unit": "ns/iter",
            "extra": "iterations: 3471\ncpu: 197978.93258426967 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 295594.7872340407,
            "unit": "ns/iter",
            "extra": "iterations: 2256\ncpu: 295511.53324468085 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1729955.2976190061,
            "unit": "ns/iter",
            "extra": "iterations: 336\ncpu: 1729498.3095238088 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17493129.54054044,
            "unit": "ns/iter",
            "extra": "iterations: 37\ncpu: 17489718.513513513 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1323.1776059004421,
            "unit": "ns/iter",
            "extra": "iterations: 529318\ncpu: 1323.0916292285538 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 13086.913845122364,
            "unit": "ns/iter",
            "extra": "iterations: 53694\ncpu: 13085.692181621775 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 144532.9345129178,
            "unit": "ns/iter",
            "extra": "iterations: 4917\ncpu: 144512.05979255648 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 424863.2670316398,
            "unit": "ns/iter",
            "extra": "iterations: 1644\ncpu: 424834.45620437927 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 384953.88241758745,
            "unit": "ns/iter",
            "extra": "iterations: 1820\ncpu: 384908.89505494543 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 320981.0650481105,
            "unit": "ns/iter",
            "extra": "iterations: 2183\ncpu: 320927.5932203383 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 67517277.62500082,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 67509088.50000003 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 21344.296597382305,
            "unit": "ns/iter",
            "extra": "iterations: 32475\ncpu: 21342.6733487298 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 388906.1376450984,
            "unit": "ns/iter",
            "extra": "iterations: 1809\ncpu: 388863.8601437377 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "committer": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "distinct": true,
          "id": "1a37ed29cc4a70529c4a0ccb4ff7050d00895dfc",
          "message": "update README",
          "timestamp": "2026-09-16T12:02:34+02:00",
          "tree_id": "fab2d4fb847109ddd86108e449e56f6604f935d2",
          "url": "https://github.com/najaeda/naja/commit/1a37ed29cc4a70529c4a0ccb4ff7050d00895dfc"
        },
        "date": 1789553085254,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 155573.37288135508,
            "unit": "ns/iter",
            "extra": "iterations: 4425\ncpu: 155527.59661016948 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 219012.83097706368,
            "unit": "ns/iter",
            "extra": "iterations: 3183\ncpu: 218960.61577128494 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1286027.756363627,
            "unit": "ns/iter",
            "extra": "iterations: 550\ncpu: 1285686.7454545451 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 13111602.722222276,
            "unit": "ns/iter",
            "extra": "iterations: 54\ncpu: 13110024.94444444 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1061.809713801699,
            "unit": "ns/iter",
            "extra": "iterations: 659927\ncpu: 1061.6199322046232 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 10386.740460955085,
            "unit": "ns/iter",
            "extra": "iterations: 67512\ncpu: 10386.224286052853 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 108922.58364369995,
            "unit": "ns/iter",
            "extra": "iterations: 6444\ncpu: 108916.55183116074 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 329572.6388376437,
            "unit": "ns/iter",
            "extra": "iterations: 2168\ncpu: 329551.81549815513 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 290503.45630668255,
            "unit": "ns/iter",
            "extra": "iterations: 2426\ncpu: 290492.39571310824 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 240281.99485773087,
            "unit": "ns/iter",
            "extra": "iterations: 2917\ncpu: 240262.363729859 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 52716334.18181729,
            "unit": "ns/iter",
            "extra": "iterations: 11\ncpu: 52710816.00000003 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 16367.158097837328,
            "unit": "ns/iter",
            "extra": "iterations: 42499\ncpu: 16364.298242311597 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 291742.86100743245,
            "unit": "ns/iter",
            "extra": "iterations: 2403\ncpu: 291624.0079067708 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "committer": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "distinct": true,
          "id": "454c0c36fc0f6e2adedeb69251768b7cc5e335f2",
          "message": "enhance diagram",
          "timestamp": "2026-09-16T12:22:59+02:00",
          "tree_id": "dd829be7dd3ce2a984d9a6b8582bbfa3abf07862",
          "url": "https://github.com/najaeda/naja/commit/454c0c36fc0f6e2adedeb69251768b7cc5e335f2"
        },
        "date": 1789554781353,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 213890.78120278474,
            "unit": "ns/iter",
            "extra": "iterations: 3309\ncpu: 213882.72559685705 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 303069.2122664297,
            "unit": "ns/iter",
            "extra": "iterations: 2087\ncpu: 303048.39338763774 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1705456.6893204108,
            "unit": "ns/iter",
            "extra": "iterations: 412\ncpu: 1705314.9199029133 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17210260.195121817,
            "unit": "ns/iter",
            "extra": "iterations: 41\ncpu: 17210118.536585372 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1219.207063252864,
            "unit": "ns/iter",
            "extra": "iterations: 573192\ncpu: 1219.0790136638325 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12168.057033625684,
            "unit": "ns/iter",
            "extra": "iterations: 57545\ncpu: 12167.599148492487 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 179769.8620024137,
            "unit": "ns/iter",
            "extra": "iterations: 4145\ncpu: 179750.8407720144 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 468621.36193028314,
            "unit": "ns/iter",
            "extra": "iterations: 1492\ncpu: 468610.96246648784 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 428340.32228916755,
            "unit": "ns/iter",
            "extra": "iterations: 1660\ncpu: 428307.5813253008 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 345929.77286134503,
            "unit": "ns/iter",
            "extra": "iterations: 2034\ncpu: 345909.16617502464 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 66677570.66666505,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 66670540.66666663 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20309.888385992715,
            "unit": "ns/iter",
            "extra": "iterations: 34467\ncpu: 20308.65294339512 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 390933.6774738114,
            "unit": "ns/iter",
            "extra": "iterations: 1789\ncpu: 390800.9636668521 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "committer": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "distinct": true,
          "id": "d9349f0056b69ffc23593051fbc81c4e1eedd780",
          "message": "Enhance diagram",
          "timestamp": "2026-09-16T12:26:32+02:00",
          "tree_id": "fab2d4fb847109ddd86108e449e56f6604f935d2",
          "url": "https://github.com/najaeda/naja/commit/d9349f0056b69ffc23593051fbc81c4e1eedd780"
        },
        "date": 1789555461374,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 216208.35591095348,
            "unit": "ns/iter",
            "extra": "iterations: 3189\ncpu: 216193.55534650365 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 305493.02038925217,
            "unit": "ns/iter",
            "extra": "iterations: 2158\ncpu: 305417.17099165893 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1759241.665036666,
            "unit": "ns/iter",
            "extra": "iterations: 409\ncpu: 1758631.102689487 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17080748.200000074,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17079210.425000004 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1232.1346372361788,
            "unit": "ns/iter",
            "extra": "iterations: 566760\ncpu: 1231.9948373209115 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 20267.34042090365,
            "unit": "ns/iter",
            "extra": "iterations: 36778\ncpu: 20263.765974223723 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 184486.7654476741,
            "unit": "ns/iter",
            "extra": "iterations: 3965\ncpu: 184236.73543505673 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 476355.20367846277,
            "unit": "ns/iter",
            "extra": "iterations: 1468\ncpu: 476293.7820163491 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 426734.6433353378,
            "unit": "ns/iter",
            "extra": "iterations: 1643\ncpu: 426662.62142422475 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 349307.62680617836,
            "unit": "ns/iter",
            "extra": "iterations: 2007\ncpu: 349272.3228699557 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 67185373.44444384,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 67176659.77777779 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20326.425595410663,
            "unit": "ns/iter",
            "extra": "iterations: 34514\ncpu: 20324.040244538413 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 395565.1252135923,
            "unit": "ns/iter",
            "extra": "iterations: 1765\ncpu: 395366.5263456186 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "51461f006e0c52e3c749debbfe3b2b08d15caf91",
          "message": "Refactor flowchart and update links in README\n\nUpdated flowchart connections and added new links.",
          "timestamp": "2026-09-17T11:51:09+02:00",
          "tree_id": "80f0e3b6184d5a86fa6ad95a8c9d9727fd345b43",
          "url": "https://github.com/najaeda/naja/commit/51461f006e0c52e3c749debbfe3b2b08d15caf91"
        },
        "date": 1789638818123,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 213949.4280800449,
            "unit": "ns/iter",
            "extra": "iterations: 3198\ncpu: 213898.31801125704 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 307250.7758696677,
            "unit": "ns/iter",
            "extra": "iterations: 2271\ncpu: 307160.53808894765 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1721322.263546801,
            "unit": "ns/iter",
            "extra": "iterations: 406\ncpu: 1720905.487684728 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17464380.80487856,
            "unit": "ns/iter",
            "extra": "iterations: 41\ncpu: 17461371.658536587 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1220.6472272550673,
            "unit": "ns/iter",
            "extra": "iterations: 573457\ncpu: 1220.5361151751576 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12136.070952438286,
            "unit": "ns/iter",
            "extra": "iterations: 57757\ncpu: 12134.518846200463 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 189546.79262452264,
            "unit": "ns/iter",
            "extra": "iterations: 4176\ncpu: 189520.47772988488 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 473908.47861507104,
            "unit": "ns/iter",
            "extra": "iterations: 1473\ncpu: 473826.3951120163 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 426787.38095238036,
            "unit": "ns/iter",
            "extra": "iterations: 1638\ncpu: 426737.5598290591 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 350031.8212680997,
            "unit": "ns/iter",
            "extra": "iterations: 2003\ncpu: 349944.75636545103 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 66866246.55555483,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 66865823.55555575 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20259.74358455096,
            "unit": "ns/iter",
            "extra": "iterations: 34565\ncpu: 20256.86833502098 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 395554.7080050702,
            "unit": "ns/iter",
            "extra": "iterations: 1774\ncpu: 395406.36809469573 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "788edb57f439f6088928452cda371ddd417a2a87",
          "message": "vhdl frontend (#451)\n\n* first commit\n\n* save phase 0\n\n* save current status\n\n* first version\n\n* start testing\n\n* First level of support\n\n* save current state\n\n* first run\n\n* progress and fix license\n\n* register first level support\n\n* new level support\n\n* new level of support\n\n* save current status\n\n* next level\n\n* next stage\n\n* vector support\n\n* next step\n\n* rising edge\n\n* signed / unsigned support\n\n* comparison operators\n\n* multiply operator\n\n* next level of support\n\n* more support\n\n* save current state\n\n* save docs\n\n* abs support\n\n* new operators\n\n* adding the API at C++ / Python level\n\n* refine git igonre\n\n* support in najaeda\n\n* new level of support\n\n* more details for errors\n\n* new level of support\n\n* rework testing and regress\n\n* new level of support and missing include\n\n* updates\n\n* new level of support and windows build\n\n* adding boost\n\n* VHDL RAM inference\n\n* more support\n\n* avance support\n\n* next level\n\n* add exit support\n\n* save current step\n\n* advance VHDL loading support\n\n* Fix building\n\n* Fix build\n\n* coverage and cleaning\n\n* coverage",
          "timestamp": "2026-09-25T13:49:13+02:00",
          "tree_id": "787d7b4f5bc1a4cd21081d8f0b213baab665c851",
          "url": "https://github.com/najaeda/naja/commit/788edb57f439f6088928452cda371ddd417a2a87"
        },
        "date": 1790337108226,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 186398.24272628684,
            "unit": "ns/iter",
            "extra": "iterations: 3712\ncpu: 186395.2376077586 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 284087.0291262133,
            "unit": "ns/iter",
            "extra": "iterations: 2472\ncpu: 284060.45428802597 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1639632.9223529364,
            "unit": "ns/iter",
            "extra": "iterations: 425\ncpu: 1639528.9976470582 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17342092.076923348,
            "unit": "ns/iter",
            "extra": "iterations: 39\ncpu: 17337273.82051282 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1323.6947699769787,
            "unit": "ns/iter",
            "extra": "iterations: 528621\ncpu: 1323.5287947319534 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 13009.081070575516,
            "unit": "ns/iter",
            "extra": "iterations: 54027\ncpu: 13005.044089066565 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 140780.87917840894,
            "unit": "ns/iter",
            "extra": "iterations: 4966\ncpu: 140770.71445831648 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 419098.58770882897,
            "unit": "ns/iter",
            "extra": "iterations: 1676\ncpu: 418982.8287589501 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 375210.80525751796,
            "unit": "ns/iter",
            "extra": "iterations: 1864\ncpu: 375126.6625536481 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 311853.0464078509,
            "unit": "ns/iter",
            "extra": "iterations: 2241\ncpu: 311782.89959839394 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 68148932.62500021,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 68143168.62500003 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 21142.772212373588,
            "unit": "ns/iter",
            "extra": "iterations: 33281\ncpu: 21139.415612511624 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 375882.6474076906,
            "unit": "ns/iter",
            "extra": "iterations: 1852\ncpu: 375842.9940604661 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "70af6f95d14f21c0126cc1bfd69b55becb0cf356",
          "message": "B0.7.25 (#452)\n\n* prepare for 0.7.25\n\n* update cibuildwheel\n\n* add documentation for najaeda Python package\n\n* export getLibraries\n\n* enhance doc\n\n* allow Parameter with no default value\n\n* coverage\n\n* add VHDL in README\n\n* Fix typo in VHDL input section",
          "timestamp": "2026-09-25T17:35:26+02:00",
          "tree_id": "a1ccaecfb09ba306ed81bbb828aaafbbe54dec76",
          "url": "https://github.com/najaeda/naja/commit/70af6f95d14f21c0126cc1bfd69b55becb0cf356"
        },
        "date": 1790350688634,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 211714.3233404723,
            "unit": "ns/iter",
            "extra": "iterations: 3269\ncpu: 211679.86448455186 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 304985.93439013284,
            "unit": "ns/iter",
            "extra": "iterations: 2271\ncpu: 304906.91765741963 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1785840.9617347508,
            "unit": "ns/iter",
            "extra": "iterations: 392\ncpu: 1785534.1377551022 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17395284.90476178,
            "unit": "ns/iter",
            "extra": "iterations: 42\ncpu: 17393558.166666668 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1202.7115430204287,
            "unit": "ns/iter",
            "extra": "iterations: 582118\ncpu: 1202.586820885113 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 11826.864024719944,
            "unit": "ns/iter",
            "extra": "iterations: 59224\ncpu: 11825.36088410105 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 193911.77155172545,
            "unit": "ns/iter",
            "extra": "iterations: 3712\ncpu: 193889.61206896545 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 478073.74642126577,
            "unit": "ns/iter",
            "extra": "iterations: 1467\ncpu: 478013.4471710974 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 429719.96487985726,
            "unit": "ns/iter",
            "extra": "iterations: 1623\ncpu: 429692.53789279144 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 353648.51635631383,
            "unit": "ns/iter",
            "extra": "iterations: 1987\ncpu: 353609.42224459007 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 67408136.00000213,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 67400323.22222215 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20861.254867539214,
            "unit": "ns/iter",
            "extra": "iterations: 31330\ncpu: 20859.477880625585 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 398705.19061197486,
            "unit": "ns/iter",
            "extra": "iterations: 1747\ncpu: 398532.768173968 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "f96226daaaa29d65500f5e176115b9bb3a0f77ba",
          "message": "B0.7.26 (#453)\n\n* bump to 0.7.26\n\n* accelerate windows wheels\n\n* better diagnostic for slang invalid expression\n\n* clean versions\n\n* update colabs\n\n* VHDL table select\n\n* Add optional schematic viewer extra and import shim\n\n* coverage and cleaning\n\n* simplify versions\n\n* fix MEM reset\n\n* clean the docs",
          "timestamp": "2026-09-28T09:40:09+02:00",
          "tree_id": "ca9f3fee02e172300e5f233851d9b0c17fe12372",
          "url": "https://github.com/najaeda/naja/commit/f96226daaaa29d65500f5e176115b9bb3a0f77ba"
        },
        "date": 1790581686762,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 214652.308181819,
            "unit": "ns/iter",
            "extra": "iterations: 3300\ncpu: 214591.6257575758 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 304737.13630406756,
            "unit": "ns/iter",
            "extra": "iterations: 2289\ncpu: 304615.5412844037 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1732774.7185185144,
            "unit": "ns/iter",
            "extra": "iterations: 405\ncpu: 1732383.2123456793 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 16858777.804878302,
            "unit": "ns/iter",
            "extra": "iterations: 41\ncpu: 16856754.146341473 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1190.2066542540842,
            "unit": "ns/iter",
            "extra": "iterations: 589187\ncpu: 1190.0785251541527 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 11834.400324258851,
            "unit": "ns/iter",
            "extra": "iterations: 59212\ncpu: 11833.550006755382 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 183429.10641587124,
            "unit": "ns/iter",
            "extra": "iterations: 3881\ncpu: 183400.34269518152 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 476249.3671988484,
            "unit": "ns/iter",
            "extra": "iterations: 1378\ncpu: 476142.0827285919 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 422820.919364707,
            "unit": "ns/iter",
            "extra": "iterations: 1637\ncpu: 422766.9920586437 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 350213.0084872605,
            "unit": "ns/iter",
            "extra": "iterations: 2003\ncpu: 350201.51522715925 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 66616421.666667394,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 66604600.66666671 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20820.464299544627,
            "unit": "ns/iter",
            "extra": "iterations: 33571\ncpu: 20819.128503768152 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 400098.1832754131,
            "unit": "ns/iter",
            "extra": "iterations: 1746\ncpu: 399936.3636884484 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "committer": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "distinct": true,
          "id": "52b5c521d84fc476ee81f31929d422f3e7cee534",
          "message": "add doc",
          "timestamp": "2026-09-28T16:49:48+02:00",
          "tree_id": "8f4e7b5cf44337724c970dad41260332abf34ff4",
          "url": "https://github.com/najaeda/naja/commit/52b5c521d84fc476ee81f31929d422f3e7cee534"
        },
        "date": 1790607144992,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 211988.58191075647,
            "unit": "ns/iter",
            "extra": "iterations: 3339\ncpu: 211967.85115303987 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 302747.3803734283,
            "unit": "ns/iter",
            "extra": "iterations: 2303\ncpu: 302683.95701259223 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1759045.101941719,
            "unit": "ns/iter",
            "extra": "iterations: 412\ncpu: 1758870.8009708747 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 16944555.71428594,
            "unit": "ns/iter",
            "extra": "iterations: 42\ncpu: 16942215.23809523 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1189.7684570314132,
            "unit": "ns/iter",
            "extra": "iterations: 588651\ncpu: 1189.7114572131873 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 11909.7871912202,
            "unit": "ns/iter",
            "extra": "iterations: 58132\ncpu: 11909.009530035093 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 168989.60004904534,
            "unit": "ns/iter",
            "extra": "iterations: 4078\ncpu: 168985.9350171654 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 469187.16243314225,
            "unit": "ns/iter",
            "extra": "iterations: 1496\ncpu: 469134.4772727273 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 419626.02752841276,
            "unit": "ns/iter",
            "extra": "iterations: 1671\ncpu: 419579.18731298595 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 344219.3672371663,
            "unit": "ns/iter",
            "extra": "iterations: 2045\ncpu: 344188.37457212777 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 66476065.444444686,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 66469736.77777782 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20828.872009319246,
            "unit": "ns/iter",
            "extra": "iterations: 33479\ncpu: 20827.483736073336 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 388855.7465251669,
            "unit": "ns/iter",
            "extra": "iterations: 1799\ncpu: 388749.01334071206 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "927b9ca2f39d16be2ef8df74053edd46bf2dc1d1",
          "message": "B0.7.27 (#454)",
          "timestamp": "2026-09-29T19:22:39+02:00",
          "tree_id": "fba6ef3f90441729316400eb4257fa3c5a4ade13",
          "url": "https://github.com/najaeda/naja/commit/927b9ca2f39d16be2ef8df74053edd46bf2dc1d1"
        },
        "date": 1790702711722,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 190224.56194808774,
            "unit": "ns/iter",
            "extra": "iterations: 3737\ncpu: 190155.73320845602 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 282383.9033057951,
            "unit": "ns/iter",
            "extra": "iterations: 2420\ncpu: 282305.5685950414 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1645453.2383177513,
            "unit": "ns/iter",
            "extra": "iterations: 428\ncpu: 1644806.8411214957 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 16605821.589743858,
            "unit": "ns/iter",
            "extra": "iterations: 39\ncpu: 16603455.84615384 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1283.3315525259152,
            "unit": "ns/iter",
            "extra": "iterations: 546007\ncpu: 1282.9506801194852 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12633.531118229565,
            "unit": "ns/iter",
            "extra": "iterations: 55257\ncpu: 12632.518214886793 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 136557.58879808654,
            "unit": "ns/iter",
            "extra": "iterations: 5017\ncpu: 136541.97887183586 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 419577.6117084797,
            "unit": "ns/iter",
            "extra": "iterations: 1674\ncpu: 419467.8560334531 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 379016.87486455933,
            "unit": "ns/iter",
            "extra": "iterations: 1846\ncpu: 378895.584507042 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 314978.7962128054,
            "unit": "ns/iter",
            "extra": "iterations: 2218\ncpu: 314910.68394950376 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 67981706.37500079,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 67968456.62499984 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 21920.50981852005,
            "unit": "ns/iter",
            "extra": "iterations: 32235\ncpu: 21917.25639832478 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 384657.52438378957,
            "unit": "ns/iter",
            "extra": "iterations: 1825\ncpu: 384561.5539726028 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "c929be71c192da7dcfae7dea51d1a18f52afa0d0",
          "message": "Regress/pulp eda benchmarks (#456)\n\n* bump najaeda version and add support for SV\n\n* add SV support\n\n* more support\n\n* copy primitives\n\n* improve coverage",
          "timestamp": "2026-10-02T13:29:10+02:00",
          "tree_id": "abf57190dfb22fe2acf042ee47a0b7fbed483085",
          "url": "https://github.com/najaeda/naja/commit/c929be71c192da7dcfae7dea51d1a18f52afa0d0"
        },
        "date": 1790940696155,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 214525.60483871528,
            "unit": "ns/iter",
            "extra": "iterations: 3100\ncpu: 214472.61258064516 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 306578.7953651142,
            "unit": "ns/iter",
            "extra": "iterations: 2287\ncpu: 306523.5553126366 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1662146.7919799415,
            "unit": "ns/iter",
            "extra": "iterations: 399\ncpu: 1661927.9774436099 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 16553172.499999931,
            "unit": "ns/iter",
            "extra": "iterations: 42\ncpu: 16551682.61904762 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1207.978311288989,
            "unit": "ns/iter",
            "extra": "iterations: 587679\ncpu: 1207.8923902334432 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 11799.99646534994,
            "unit": "ns/iter",
            "extra": "iterations: 58846\ncpu: 11798.696207048897 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 173972.73479201517,
            "unit": "ns/iter",
            "extra": "iterations: 4159\ncpu: 173957.80379899018 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 476192.54786150076,
            "unit": "ns/iter",
            "extra": "iterations: 1473\ncpu: 476159.2036659877 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 428001.91793894145,
            "unit": "ns/iter",
            "extra": "iterations: 1572\ncpu: 427966.5610687024 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 350505.6896896773,
            "unit": "ns/iter",
            "extra": "iterations: 1998\ncpu: 350496.56656656665 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 66619432.11111066,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 66616724.444444366 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20861.000894534227,
            "unit": "ns/iter",
            "extra": "iterations: 33537\ncpu: 20860.396099830043 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 396135.6210822617,
            "unit": "ns/iter",
            "extra": "iterations: 1755\ncpu: 396016.2626780313 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "oyvind.harboe@zylin.com",
            "name": "Øyvind Harboe",
            "username": "oharboe"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "20d67a27d95ac8a5fcc1a3657646887aa1a4f328",
          "message": "bazel: plain bazel_deps from BCR, no host tools (#457)\n\n* bazel: plain bazel_deps, no cmake, no host tools\n\nMake naja's Bazel build ready to be a Bazel Central Registry module:\nMODULE.bazel now contains only bazel_deps.\n\n- slang: sv-lang (native Bazel build) instead of building slang's\n  CMake project through rules_foreign_cc.\n- TBB, Boost, fmt, spdlog, bison/m4: the BCR onetbb, boost.*, fmt,\n  spdlog and bison modules instead of pkg-config/python3-config/PATH\n  probing repository rules, vendored tarballs and the vendored spdlog.\n- Python: the rules_python toolchain for libpython and for the Python\n  test runners.\n- cpptrace, tomlplusplus: dropped; nothing in naja's Bazel build uses\n  them directly.\n- NajaVersion.h: the git hash comes from the module version\n  (<release>-<date>-<commit> for registry builds) instead of a\n  repository rule running git.\n- SV frontend tests: resolve the benchmarks path before changing the\n  current directory, instead of baking the workspace's absolute path in\n  with a repository rule.\n- Export FF_scan.lib for downstream test suites.\n\nModules not on BCR yet (naja-if, naja-verilog, and sv-lang at the slang\ncommit naja is developed against) are served by an in-tree registry in\nbazel/registry/ with BCR's layout, listed ahead of BCR in .bazelrc, so\npublishing them is a copy into a BCR pull request. The naja-verilog\nentry's overlay uses the BCR bison/flex modules. The submodule/Bazel pin\ncheck now reads the registry.\n\nBazel CI no longer installs any host packages.\n\nCo-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>\n\n* bazel: export TBB and zlib from naja_runtime\n\nBinaries that link naja_runtime and also depend on TBB or zlib\nthemselves (kepler-formal) must use the runtime's single statically\nlinked copy; cc_shared_library refuses to link them twice. Match by\npackage, since BCR zlib's target behind @zlib//:z is in a subpackage.\n\nCo-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>\n\n* bazel: sv-lang registry entry depends on boost.regex\n\nslang's WaiverManager uses boost::regex header-only; with the host\ntoolchain it was silently picked up from /usr/include.\n\nCo-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>\n\n* bazel: build under hermetic toolchains; keep libpython in dynamic links\n\n- bison 3.8.2.bcr.10 in the in-tree registry: BCR's bcr.9 with gnulib's\n  wrapper headers on -I rather than -isystem. Toolchains that pass libc\n  headers as -isystem (hermetic-llvm) otherwise shadow them and bison\n  fails to compile. naja and naja-verilog ask for it.\n- pyloader: re-own libpython's linker inputs. current_py_cc_libs\n  forwards a toolchain target's CcInfo, which cc_binary's dynamic_deps\n  filtering cannot see, so a binary linking naja_runtime dynamically\n  silently dropped libpython.\n- update_source.py keeps source.json fields it does not compute\n  (mirror_urls, patch_strip).\n\nCo-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>\n\n* bazel: python_libs advertises CcInfo\n\ncc_shared_library's graph aspect only visits rules that advertise\nCcInfo; without provides=[CcInfo] the re-owned libpython was still\ndropped from binaries that link naja_runtime dynamically.\n\nCo-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>\n\n* bazel: load rules_cc/rules_shell rules explicitly\n\nBazel 9 removed the native cc_* and sh_* rules; a module consumed\nunder Bazel 9 must load them.\n\nCo-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>\n\n* CLAUDE.md: Bazel invariants, registry workflow and known traps\n\nSo agents (and people) keep the Bazel build BCR-ready: what may go in\nMODULE.bazel, how the in-tree registry works, how to publish to BCR, and\nthe toolchain/linking traps already hit once.\n\nCo-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>\n\n* bazel: sv-lang 11.0.0-20260701-b60d729d.bcr.1 builds with gcc 11\n\nslang's BS_thread_pool.hpp has `class SLANG_EXPORT [[nodiscard]]\nthis_thread`; with SLANG_EXPORT expanding to a GNU attribute, gcc 11\nrejects it, which broke the ubuntu-22.04 Bazel CI job. slang is built\nas a static library, and upstream defines SLANG_STATIC_DEFINE for static\nbuilds, which makes SLANG_EXPORT empty. The overlay now does the same.\n\nAlso replace the overlay/MODULE.bazel symlinks with copies: BCR rejects\nsymlinks in registry entries.\n\nCo-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>\n\n* bazel: publish-to-bcr templates, BCR maintainers, current presubmit platforms\n\n- .bcr/ holds the publish-to-bcr templates for naja itself, with\n  xtofalex and nanocoh as the module's BCR maintainers.\n- naja-if and naja-verilog list the same maintainers.\n- naja-if, naja-verilog presubmit: debian13, ubuntu2204, ubuntu2404,\n  macos_arm64 on Bazel 8.x and 9.x (debian11 is past end of life).\n\nCo-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>\n\n* bazel: naja-verilog needs only BCR's bison 3.8.2.bcr.9\n\nnaja-verilog builds with the bison already on BCR, so its BCR entry\ndoesn't wait on bison 3.8.2.bcr.10. naja itself still asks for bcr.10\n(needed with the hermetic llvm toolchain), and the highest version in\nthe module graph wins.\n\nCo-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>\n\n* bazel: take unreleased modules from their BCR pull requests\n\nbison 3.8.2.bcr.10, sv-lang 11.0.0-20260701-b60d729d.bcr.1, naja-if and\nnaja-verilog are submitted to BCR, one pull request each\n(bazelbuild/bazel-central-registry#10879, #10882, #10881, #10885).\n.bazelrc lists each PR's commit as a registry ahead of BCR, so the\nin-tree registry goes away; drop a line as its PR merges.\ncheck_submodule_bazel_sync.py reads source.json from those registries.\n\nCo-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>\n\n---------\n\nCo-authored-by: Claude Opus 5.5 <noreply@anthropic.com>",
          "timestamp": "2026-10-03T14:18:57+02:00",
          "tree_id": "0c79bbd067a4aa67ce30c1a006ecb0334d6105f5",
          "url": "https://github.com/najaeda/naja/commit/20d67a27d95ac8a5fcc1a3657646887aa1a4f328"
        },
        "date": 1791030107516,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 211825.3422818681,
            "unit": "ns/iter",
            "extra": "iterations: 3278\ncpu: 211811.4694935937 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 306838.7907180375,
            "unit": "ns/iter",
            "extra": "iterations: 2284\ncpu: 306811.33844133094 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1797485.5612745353,
            "unit": "ns/iter",
            "extra": "iterations: 408\ncpu: 1794224.3627450985 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17788833.950000085,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17784515.550000004 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1187.6253826374027,
            "unit": "ns/iter",
            "extra": "iterations: 588024\ncpu: 1187.5251554358329 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 11857.02217283538,
            "unit": "ns/iter",
            "extra": "iterations: 58946\ncpu: 11853.743901197702 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 175647.14489699906,
            "unit": "ns/iter",
            "extra": "iterations: 4272\ncpu: 175634.64208801492 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 472789.29687502695,
            "unit": "ns/iter",
            "extra": "iterations: 1472\ncpu: 472749.3892663045 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 424398.45151513757,
            "unit": "ns/iter",
            "extra": "iterations: 1650\ncpu: 424321.089696969 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 349424.73668492254,
            "unit": "ns/iter",
            "extra": "iterations: 2009\ncpu: 349398.99502239964 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 67313974.77778323,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 67305519.1111111 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20817.902608798708,
            "unit": "ns/iter",
            "extra": "iterations: 33617\ncpu: 20816.712734628316 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 396744.48868768755,
            "unit": "ns/iter",
            "extra": "iterations: 1768\ncpu: 396682.50791856303 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "b05f17bf656a3dc9292d649abb7ac2baf6ae4c49",
          "message": "expand PULP regress (#458)",
          "timestamp": "2026-10-03T17:58:13+02:00",
          "tree_id": "7990a754f1d4838fbf36e21db84d6ddc1a8bcacc",
          "url": "https://github.com/najaeda/naja/commit/b05f17bf656a3dc9292d649abb7ac2baf6ae4c49"
        },
        "date": 1791043237812,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 189465.32772434934,
            "unit": "ns/iter",
            "extra": "iterations: 3744\ncpu: 189458.1030982906 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 282753.2061896893,
            "unit": "ns/iter",
            "extra": "iterations: 2488\ncpu: 282658.5836012862 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1727241.412621304,
            "unit": "ns/iter",
            "extra": "iterations: 412\ncpu: 1726820.805825243 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17658532.743589662,
            "unit": "ns/iter",
            "extra": "iterations: 39\ncpu: 17656681.56410257 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1135.6156531245724,
            "unit": "ns/iter",
            "extra": "iterations: 607342\ncpu: 1135.594986021055 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 11444.441903695611,
            "unit": "ns/iter",
            "extra": "iterations: 60787\ncpu: 11443.979008669618 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 114521.49291288311,
            "unit": "ns/iter",
            "extra": "iterations: 6279\ncpu: 114506.11084567603 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 411738.40235293994,
            "unit": "ns/iter",
            "extra": "iterations: 1700\ncpu: 411690.21588235267 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 366167.2744380647,
            "unit": "ns/iter",
            "extra": "iterations: 1913\ncpu: 366142.90538421384 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 295203.25242308533,
            "unit": "ns/iter",
            "extra": "iterations: 2373\ncpu: 295161.8672566374 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 67849206.88888481,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 67837432.55555564 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 23852.956521737924,
            "unit": "ns/iter",
            "extra": "iterations: 29578\ncpu: 23848.7481574143 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 302199.78442469164,
            "unit": "ns/iter",
            "extra": "iterations: 2324\ncpu: 302110.81583472004 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "5fb0566a83f48b4f0ffb6b6e7365d5243fb5caa1",
          "message": "Expand pulp sv regress (#459)\n\n* expand PULP regress\n\n* ITA regress\n\n* cleaning",
          "timestamp": "2026-10-04T11:13:03+02:00",
          "tree_id": "7cb0f3144e22468f7caaf4ca9f33ce41e414c32d",
          "url": "https://github.com/najaeda/naja/commit/5fb0566a83f48b4f0ffb6b6e7365d5243fb5caa1"
        },
        "date": 1791105296662,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 129772.87820044164,
            "unit": "ns/iter",
            "extra": "iterations: 5468\ncpu: 129766.18544257498 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 197834.9055319131,
            "unit": "ns/iter",
            "extra": "iterations: 3525\ncpu: 197819.23460992903 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1190704.802653426,
            "unit": "ns/iter",
            "extra": "iterations: 603\ncpu: 1190526.109452736 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 12644133.018181946,
            "unit": "ns/iter",
            "extra": "iterations: 55\ncpu: 12642735.163636368 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 702.8261165727466,
            "unit": "ns/iter",
            "extra": "iterations: 988001\ncpu: 702.7551885068941 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 7452.259936595754,
            "unit": "ns/iter",
            "extra": "iterations: 96839\ncpu: 7451.324042999202 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 80020.43808911704,
            "unit": "ns/iter",
            "extra": "iterations: 9336\ncpu: 80011.64342330767 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 316005.05454544665,
            "unit": "ns/iter",
            "extra": "iterations: 2365\ncpu: 315955.1167019027 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 262288.6638815012,
            "unit": "ns/iter",
            "extra": "iterations: 2633\ncpu: 262255.0053171283 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 210124.54459664205,
            "unit": "ns/iter",
            "extra": "iterations: 3285\ncpu: 210100.73637747372 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 46283579.83333349,
            "unit": "ns/iter",
            "extra": "iterations: 12\ncpu: 46280631.00000013 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 18094.17082066993,
            "unit": "ns/iter",
            "extra": "iterations: 41125\ncpu: 18091.431732522786 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 224866.63096632186,
            "unit": "ns/iter",
            "extra": "iterations: 3436\ncpu: 204557.5488940972 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "noam.chn1@gmail.com",
            "name": "Noam Cohen",
            "username": "nanocoh"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "931afedebbd6269bb5e1f27a2cc2cc1a73dfc37f",
          "message": "Do not infer a memory for an array written from several always blocks (#455)\n\n* Model divmod outputs with generic truth tables\n\n* Add Python sequential model API\n\n* Add instance-parameterized truth tables\n\n* Share Naja runtime with Python extension in Bazel\n\n* Fix Bazel Python test runtime loading\n\n* Link TBB into Bazel shared runtime\n\n* Keep mixed FF/latch Liberty cells loadable\n\n* Fix Liberty state cells getting constant truth tables\n\n* Preserve timing for opaque Liberty state cells\n\n* test: improve Naja API coverage\n\n* test: cover remaining modeling branches\n\n* Add shared Python runtime SDK and safe parameter lifetimes\n\n* Keep the upstream 0.7.26 version for the SDK provider\n\nThe fix7 merge carried its 0.7.24.dev0 version onto main. Use the\nversion from main instead so the provider built from this source\nmatches the release it is based on.\n\nCo-Authored-By: Claude Fable 5.1 <noreply@anthropic.com>\n\n* Do not infer a memory for an array written from several always blocks\n\nMemory inference matched one sequential block per array. When an unpacked\narray was written by one always block per element, as a generate loop\nproduces, the first block became the memory's write port and the others\nwere skipped as already inferred. The generic sequential lowering then\nbuilt a flop for each skipped block, and that flop drove the memory's read\ndata net a second time. Every cone through such a net was unverifiable.\n\nCount the sequential blocks that write each candidate array first. An\narray with more than one writer block is left to the generic sequential\nlowering for all of its writers, with a warning naming the array and the\nnumber of blocks. Arrays with a single writer block are unchanged.\n\nReported in keplertech/kepler-formal#250 on Systolic_MAC_with_DFT, whose\nself compare covered 15 of 24 outputs. It now proves 24 of 24.\n\n* Reduce the Python shared-runtime SDK to the shipped headers\n\nInstall the headers matching the wheel's libraries under najaeda/sdk/include\nand keep the Windows export change. A native consumer such as kepler-formal\nresolves the installed libraries, validates the build and swaps the DNL\nsingleton itself, so remove sdk.py, the CMake package and build identity\nfiles, the _C_API capsule, NLUniverse::getRuntimeIdentity, DNL::exchange\nand their tests.\n\n---------\n\nCo-authored-by: Claude Fable 5.1 <noreply@anthropic.com>",
          "timestamp": "2026-10-04T23:30:32+02:00",
          "tree_id": "7d9210b05222c5974e42d11db67a0c59a0f0f9de",
          "url": "https://github.com/najaeda/naja/commit/931afedebbd6269bb5e1f27a2cc2cc1a73dfc37f"
        },
        "date": 1791149563534,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 163102.02304797605,
            "unit": "ns/iter",
            "extra": "iterations: 4252\ncpu: 163096.60112888052 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 226854.7734426189,
            "unit": "ns/iter",
            "extra": "iterations: 3050\ncpu: 226816.75081967216 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1312736.3867925112,
            "unit": "ns/iter",
            "extra": "iterations: 530\ncpu: 1312671.8471698111 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 13260369.566038026,
            "unit": "ns/iter",
            "extra": "iterations: 53\ncpu: 13259292.622641511 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 996.0229662617091,
            "unit": "ns/iter",
            "extra": "iterations: 701725\ncpu: 995.8511339912355 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 9808.353570627787,
            "unit": "ns/iter",
            "extra": "iterations: 71304\ncpu: 9807.798622798147 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 105046.29012627267,
            "unit": "ns/iter",
            "extra": "iterations: 6573\ncpu: 105026.41776966372 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 328000.34509985807,
            "unit": "ns/iter",
            "extra": "iterations: 2153\ncpu: 327917.0176497909 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 293878.0443514588,
            "unit": "ns/iter",
            "extra": "iterations: 2390\ncpu: 293776.8397489544 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 244789.4808495748,
            "unit": "ns/iter",
            "extra": "iterations: 2872\ncpu: 244792.7409470753 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 52600292.69999791,
            "unit": "ns/iter",
            "extra": "iterations: 10\ncpu: 52595257.09999995 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 16801.147886289684,
            "unit": "ns/iter",
            "extra": "iterations: 40876\ncpu: 16800.74909482336 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 298749.04282590834,
            "unit": "ns/iter",
            "extra": "iterations: 2335\ncpu: 298632.3957173741 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "50306aa8662347000ee43bfce0ebcf6cf44720b9",
          "message": "Expand pulp verification (#460)",
          "timestamp": "2026-10-05T13:35:20+02:00",
          "tree_id": "f3ac55a2dd0c29221989eb5c6e22b77db2a62cf3",
          "url": "https://github.com/najaeda/naja/commit/50306aa8662347000ee43bfce0ebcf6cf44720b9"
        },
        "date": 1791200247713,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 156542.69211986335,
            "unit": "ns/iter",
            "extra": "iterations: 4505\ncpu: 156523.98534961155 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 242063.6766917286,
            "unit": "ns/iter",
            "extra": "iterations: 2926\ncpu: 242005.83185235813 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1460304.5386179446,
            "unit": "ns/iter",
            "extra": "iterations: 492\ncpu: 1460044.5386178864 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 15040655.680850927,
            "unit": "ns/iter",
            "extra": "iterations: 47\ncpu: 15039904.212765958 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 944.983289394837,
            "unit": "ns/iter",
            "extra": "iterations: 739291\ncpu: 944.970855860548 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 9711.134535925132,
            "unit": "ns/iter",
            "extra": "iterations: 72122\ncpu: 9710.951873214832 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 94118.68953314702,
            "unit": "ns/iter",
            "extra": "iterations: 7347\ncpu: 94117.29848917926 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 343998.3067061103,
            "unit": "ns/iter",
            "extra": "iterations: 2028\ncpu: 343967.1809664695 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 320695.63620465505,
            "unit": "ns/iter",
            "extra": "iterations: 2287\ncpu: 320681.8408395277 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 247487.39449541687,
            "unit": "ns/iter",
            "extra": "iterations: 2834\ncpu: 247482.1379675368 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 54639883.10000332,
            "unit": "ns/iter",
            "extra": "iterations: 10\ncpu: 54637602.89999993 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20071.100453625353,
            "unit": "ns/iter",
            "extra": "iterations: 35051\ncpu: 20070.97352429319 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 251289.8298023517,
            "unit": "ns/iter",
            "extra": "iterations: 2785\ncpu: 251226.0290843755 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "00c09dfef5c6b7d9e65c8a347fdcc8f8fdeb48b7",
          "message": "add parameterized FF modeling (#461)\n\n* add parameterized FF modeling\n\n* cleaning for Python 3.9\n\n* coverage\n\n* update action\n\n* retry bison download",
          "timestamp": "2026-10-06T10:19:14+02:00",
          "tree_id": "127da9811feac43ee096904a1b1861e7171fb9cf",
          "url": "https://github.com/najaeda/naja/commit/00c09dfef5c6b7d9e65c8a347fdcc8f8fdeb48b7"
        },
        "date": 1791274912612,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 190317.69205388956,
            "unit": "ns/iter",
            "extra": "iterations: 3637\ncpu: 190282.53038218315 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 285603.6487499954,
            "unit": "ns/iter",
            "extra": "iterations: 2400\ncpu: 285562.5200000001 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1760995.518610382,
            "unit": "ns/iter",
            "extra": "iterations: 403\ncpu: 1760628.965260546 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 21881889.84375028,
            "unit": "ns/iter",
            "extra": "iterations: 32\ncpu: 21879826.28125 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1023.8194011968802,
            "unit": "ns/iter",
            "extra": "iterations: 670237\ncpu: 1023.7977446783748 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 10504.139440580155,
            "unit": "ns/iter",
            "extra": "iterations: 67570\ncpu: 10503.680198312853 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 107534.41913725421,
            "unit": "ns/iter",
            "extra": "iterations: 6375\ncpu: 107527.57380392154 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 430117.6815131243,
            "unit": "ns/iter",
            "extra": "iterations: 1639\ncpu: 430083.7199511901 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 382307.5542826021,
            "unit": "ns/iter",
            "extra": "iterations: 1833\ncpu: 382288.21603927965 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 311130.06405694253,
            "unit": "ns/iter",
            "extra": "iterations: 2248\ncpu: 311103.8972419928 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 74649410.12499793,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 74642562.12499999 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 19988.042047428524,
            "unit": "ns/iter",
            "extra": "iterations: 35127\ncpu: 19985.539641870968 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 319344.2730177994,
            "unit": "ns/iter",
            "extra": "iterations: 2194\ncpu: 319260.50638101227 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "oyvind.harboe@zylin.com",
            "name": "Øyvind Harboe",
            "username": "oharboe"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "43bd82b315ae0c16b8d847f0c94290aa8e0550ef",
          "message": "bazel: sv-lang 12.0.0, and slang at v12.0 to match (#464)\n\nslang v12.0 contains the commit naja was pinned to (b60d729d), and\nsv-lang 12.0.0 is its BCR release\n(bazelbuild/bazel-central-registry#10977), replacing the unreleased\n11.0.0-20260701-b60d729d.bcr.1, whose pull request is closed.\n\nThe thirdparty/slang submodule moves to v12.0 (545f88b2) with it, as\nci/check_submodule_bazel_sync.py requires. That script now also takes a\nrelease's tag archive URL (refs/tags/<tag>.tar.gz), resolving the tag\nto its commit.\n\n.bazelrc: bison 3.8.2.bcr.10 is on BCR now; sv-lang comes from #10977.",
          "timestamp": "2026-10-08T12:15:46+02:00",
          "tree_id": "9ef0770c9432133bb820dbe88a737e2b841cf7d6",
          "url": "https://github.com/najaeda/naja/commit/43bd82b315ae0c16b8d847f0c94290aa8e0550ef"
        },
        "date": 1791454708185,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 217223.99541844107,
            "unit": "ns/iter",
            "extra": "iterations: 3274\ncpu: 217189.6145387905 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 310214.9381625477,
            "unit": "ns/iter",
            "extra": "iterations: 2264\ncpu: 310120.7009717315 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1696488.8599033013,
            "unit": "ns/iter",
            "extra": "iterations: 414\ncpu: 1696300.224637681 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 19578321.388888437,
            "unit": "ns/iter",
            "extra": "iterations: 36\ncpu: 19575890.16666667 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1210.5120756733922,
            "unit": "ns/iter",
            "extra": "iterations: 578063\ncpu: 1209.2765926897232 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 11897.434589838187,
            "unit": "ns/iter",
            "extra": "iterations: 58867\ncpu: 11895.222773370475 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 181998.56722892684,
            "unit": "ns/iter",
            "extra": "iterations: 4150\ncpu: 181975.7727710845 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 478272.5578370801,
            "unit": "ns/iter",
            "extra": "iterations: 1461\ncpu: 478181.4887063661 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 432236.49875002936,
            "unit": "ns/iter",
            "extra": "iterations: 1600\ncpu: 432208.6037500006 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 352707.6109437539,
            "unit": "ns/iter",
            "extra": "iterations: 1992\ncpu: 352649.42018072295 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 68022397.00000001,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 68017380.99999999 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20555.9939263383,
            "unit": "ns/iter",
            "extra": "iterations: 33423\ncpu: 20552.777907429023 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 401311.8857777746,
            "unit": "ns/iter",
            "extra": "iterations: 1751\ncpu: 401178.4928612375 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "545ebd99803d301bdb46b4f326998945df83aa28",
          "message": "Fix/vhdl full adder (#463)\n\n* elaborate full adders\n\n* load several files in VHDL\n\n* truncate arithmetic\n\n* clean API\n\n* Fix test\n\n* implicit return variables now use procedural replay for whole, bit, and part-select writes, unrolled loops, and last-write-wins behavior in both contexts. Constant arguments are materialized correctly; incomplete results remain rejected.\n\n* Fixed the disjointness analysis to fold selections with active loop-unrolling constants. Constant unpacked elements also establish disjoint ownership.\n\n* extending naja_mem with per-write-port clocks and bit masks\n\n* # Share preprocessor macros across source files in input order.\n    single_unit: bool = False\n\n* reduce gate size\n\n* FIX API call\n\n* regold\n\n* coverage\n\n* coverage\n\n* coverage",
          "timestamp": "2026-10-09T13:02:20+02:00",
          "tree_id": "54d07e5bb9eab280c6ab5128de81b786c6a6e27d",
          "url": "https://github.com/najaeda/naja/commit/545ebd99803d301bdb46b4f326998945df83aa28"
        },
        "date": 1791543898760,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 187797.9037947621,
            "unit": "ns/iter",
            "extra": "iterations: 3742\ncpu: 187773.14831640833 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 284718.4449877792,
            "unit": "ns/iter",
            "extra": "iterations: 2454\ncpu: 284662.3859005704 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1724689.3093525185,
            "unit": "ns/iter",
            "extra": "iterations: 417\ncpu: 1724286.757793765 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17895179.00000011,
            "unit": "ns/iter",
            "extra": "iterations: 39\ncpu: 17894534.897435904 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1091.569166174843,
            "unit": "ns/iter",
            "extra": "iterations: 642113\ncpu: 1091.4712535021097 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 11014.012518010164,
            "unit": "ns/iter",
            "extra": "iterations: 64547\ncpu: 11012.945915379489 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 107386.55470776727,
            "unit": "ns/iter",
            "extra": "iterations: 6553\ncpu: 107372.49381962462 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 412029.1228588207,
            "unit": "ns/iter",
            "extra": "iterations: 1693\ncpu: 411979.48848198476 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 366878.83089770307,
            "unit": "ns/iter",
            "extra": "iterations: 1916\ncpu: 366825.7964509394 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 293827.34372366325,
            "unit": "ns/iter",
            "extra": "iterations: 2374\ncpu: 293819.88205560215 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 66385232.99999911,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 66372356.44444455 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 23004.08717388437,
            "unit": "ns/iter",
            "extra": "iterations: 30204\ncpu: 23003.284266984534 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 301336.1409110011,
            "unit": "ns/iter",
            "extra": "iterations: 2349\ncpu: 301236.6377182 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "583595484305a596ae2c555f0908826467d69377",
          "message": "B0.7.29 (#466)\n\n* init next version\n\n* gits migration\n\n* redesign sources infos\n\n* clean files\n\n* cleaning\n\n* coverage",
          "timestamp": "2026-10-12T23:39:29+02:00",
          "tree_id": "0b1155340bc695cb26b3cf5880b0aa1568c7e4ce",
          "url": "https://github.com/najaeda/naja/commit/583595484305a596ae2c555f0908826467d69377"
        },
        "date": 1791841318121,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 215964.21835839553,
            "unit": "ns/iter",
            "extra": "iterations: 3192\ncpu: 215960.22211779447 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 315120.66403508786,
            "unit": "ns/iter",
            "extra": "iterations: 2280\ncpu: 315086.0942982456 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1802624.7051281896,
            "unit": "ns/iter",
            "extra": "iterations: 390\ncpu: 1802436.6512820509 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 16832588.78571386,
            "unit": "ns/iter",
            "extra": "iterations: 42\ncpu: 16829890.499999996 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1189.6339563624297,
            "unit": "ns/iter",
            "extra": "iterations: 587613\ncpu: 1189.577652298367 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 11801.548478439065,
            "unit": "ns/iter",
            "extra": "iterations: 58624\ncpu: 11801.169828056773 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 180972.28627967954,
            "unit": "ns/iter",
            "extra": "iterations: 3790\ncpu: 180959.33799472297 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 495161.29893993,
            "unit": "ns/iter",
            "extra": "iterations: 1415\ncpu: 495133.9731448764 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 449019.8313021264,
            "unit": "ns/iter",
            "extra": "iterations: 1559\ncpu: 448971.41372674855 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 364006.63783220074,
            "unit": "ns/iter",
            "extra": "iterations: 1919\ncpu: 363953.0552371023 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 66979370.44444302,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 66973216.66666668 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20158.82130407466,
            "unit": "ns/iter",
            "extra": "iterations: 34998\ncpu: 20155.63603634495 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 401773.43465813034,
            "unit": "ns/iter",
            "extra": "iterations: 1737\ncpu: 401665.63327579666 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1346550347@qq.com",
            "name": "Qiming Chu",
            "username": "Emin017"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "b7a72ce03b5b62195b43211f7f4d934101c1849a",
          "message": "Canonicalize sized bit-literal parameter values in the structural Verilog frontend (#465)\n\n* Canonicalize sized bit-literal parameters in the Verilog frontend\n\n* Canonicalize only bit-typed parameters in the Verilog frontend\n\n* Add a shared canonical bit-vector value formatter to NLDB0\n\nNLDB0::formatBitVectorValue() generalizes formatDFFInitValue() to the\ncanonical storage form for resolved bit-vector parameter values,\n\"<width>['s]'b<msb...lsb>\", and documents the contract: the stored\nvalue is the literal itself, not a destination-type conversion, with\nexplicit sizing rules for consumers that apply it at a different width\nand an explicit list of the parameter values that remain\nnon-canonical. formatDFFInitValue() becomes a thin wrapper so the DB0\nsequential primitives keep their existing entry point.\n\n* Fix literal semantics in Verilog parameter canonicalization\n\nAddress review feedback on the sized bit-literal canonicalization:\n\n- Unsized literals ('hF) no longer get a width fabricated from their\n  digit count: their width is context-dependent (at least 32), so they\n  keep their source form.\n- The signed marker is preserved (16'sh000F becomes\n  16'sb0000000000001111) instead of being dropped: sign extension in a\n  wider context would otherwise change the value.\n- Negated literals keep their source form. Unary minus evaluates at the\n  context width (in 16 bits, -4'b0001 is 16'hffff), so the previous\n  literal-width two's complement was not functionally equivalent; its\n  carry-based algorithm also mishandled unknown bits (-4'bx001 is\n  4'bxxxx, not 4'bx111). Since Number::getString() drops the sign of a\n  negated number, the kept source text is rebuilt with its '-' rather\n  than silently losing it.\n\nFormatting now goes through the shared NLDB0::formatBitVectorValue(),\nwhich owns the canonical contract and its validation.\n\n* Extend parameter canonicalization regression coverage\n\nCover the literal shapes called out in review, through both parameter\noverride paths:\n\n- signed literals whose top bit is 1 (4'sb1111) via #(...) and defparam,\n  so the case that motivated preserving the signed marker is exercised;\n- signed literals with unknown digits (4'sb1x01) via both paths;\n- negated literals via defparam, including unknown digits (-4'bx001),\n  which keep their source form with the sign restored (the #(...)\n  expression grammar has no unary minus, so only defparam can carry a\n  negated literal; the sign-restoring branch on the expression path is\n  marked unreachable);\n- binary base with separators, decimal lone-x, x-extension to the\n  declared width, and truncation of extra leftmost digit bits;\n- plain-integer and string overrides on decimal- and string-typed\n  parameters, which keep their source form.\n\nAdd a dump/reload round trip test: the stored canonical values (including\nthe 4'sb1111 signed form) are emitted by SNLVRLDumper as valid Verilog\nand reparse to the identical stored form.",
          "timestamp": "2026-10-13T12:46:12+02:00",
          "tree_id": "074abf6e20f0a1c1e5b6bf4d0eea216baa1224d6",
          "url": "https://github.com/najaeda/naja/commit/b7a72ce03b5b62195b43211f7f4d934101c1849a"
        },
        "date": 1791888524626,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 218107.35903226113,
            "unit": "ns/iter",
            "extra": "iterations: 3100\ncpu: 218057.85774193547 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 314117.0922939049,
            "unit": "ns/iter",
            "extra": "iterations: 2232\ncpu: 314027.15008960577 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1824279.6207792035,
            "unit": "ns/iter",
            "extra": "iterations: 385\ncpu: 1823863.7038961037 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17896006.89999986,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17893288.575 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1223.116113570116,
            "unit": "ns/iter",
            "extra": "iterations: 572052\ncpu: 1222.977143686238 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12129.467193142675,
            "unit": "ns/iter",
            "extra": "iterations: 57869\ncpu: 12128.07990461213 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 183547.8077216159,
            "unit": "ns/iter",
            "extra": "iterations: 3937\ncpu: 183532.8841757683 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 501349.99429387023,
            "unit": "ns/iter",
            "extra": "iterations: 1402\ncpu: 501273.43009985733 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 452345.7280927889,
            "unit": "ns/iter",
            "extra": "iterations: 1552\ncpu: 452293.9168814435 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 366612.34309624135,
            "unit": "ns/iter",
            "extra": "iterations: 1912\ncpu: 366559.28504184127 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 66868833.22222078,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 66858852.888888866 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 19827.68454868517,
            "unit": "ns/iter",
            "extra": "iterations: 35175\ncpu: 19826.028201847937 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 406184.4843027257,
            "unit": "ns/iter",
            "extra": "iterations: 1720\ncpu: 406097.8052326016 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "3da8811de09c15946329e6fc8df6abcac379265c",
          "message": "package-scoped constant used directly as a port connection is elabora… (#467)\n\n* package-scoped constant used directly as a port connection is elaborated as an undriven wire\n\n* fix big: a clocked write to an array element at a variable index (storage[wptr] <= d) is lowered incorrectly. The array is left undriven, and the read port gets two drivers.\n\n* coverage",
          "timestamp": "2026-10-14T21:42:31+02:00",
          "tree_id": "a12004564f9fc5bf46702dfd6a9d1a70068ecdc1",
          "url": "https://github.com/najaeda/naja/commit/3da8811de09c15946329e6fc8df6abcac379265c"
        },
        "date": 1792007102866,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 223542.156638333,
            "unit": "ns/iter",
            "extra": "iterations: 3058\ncpu: 223519.82472204056 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 312690.8728813496,
            "unit": "ns/iter",
            "extra": "iterations: 2242\ncpu: 312598.49152542365 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1802413.5196078399,
            "unit": "ns/iter",
            "extra": "iterations: 408\ncpu: 1802141.6274509812 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17845968.775000643,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17843464.025 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1223.9702391453493,
            "unit": "ns/iter",
            "extra": "iterations: 572497\ncpu: 1223.9130353521502 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12109.644545500967,
            "unit": "ns/iter",
            "extra": "iterations: 57833\ncpu: 12108.887296180372 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 179558.42906299434,
            "unit": "ns/iter",
            "extra": "iterations: 3778\ncpu: 179222.63896241417 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 496568.6924715904,
            "unit": "ns/iter",
            "extra": "iterations: 1408\ncpu: 496514.64985795435 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 451550.3333333183,
            "unit": "ns/iter",
            "extra": "iterations: 1551\ncpu: 451517.3010960676 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 364899.39114581206,
            "unit": "ns/iter",
            "extra": "iterations: 1920\ncpu: 364850.9375000007 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 67933450.77777404,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 67926139.44444448 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 19848.802818901815,
            "unit": "ns/iter",
            "extra": "iterations: 35191\ncpu: 19844.49612116734 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 407816.0458885377,
            "unit": "ns/iter",
            "extra": "iterations: 1678\ncpu: 407760.8092967562 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "2f335d9b2d58a0bea67f73a6d87016fe0ea70bce",
          "message": "new SV support (#468)",
          "timestamp": "2026-10-15T05:04:45+02:00",
          "tree_id": "c46f2437262ab1dd428812a5c8576cd314fbca77",
          "url": "https://github.com/najaeda/naja/commit/2f335d9b2d58a0bea67f73a6d87016fe0ea70bce"
        },
        "date": 1792033634855,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 201878.89619377843,
            "unit": "ns/iter",
            "extra": "iterations: 3468\ncpu: 201810.86562860437 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 285555.44558101497,
            "unit": "ns/iter",
            "extra": "iterations: 2444\ncpu: 285481.8944353519 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1720643.0812808436,
            "unit": "ns/iter",
            "extra": "iterations: 406\ncpu: 1720143.3226600986 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18373889.578947157,
            "unit": "ns/iter",
            "extra": "iterations: 38\ncpu: 18370991.55263158 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1015.7557712802624,
            "unit": "ns/iter",
            "extra": "iterations: 698112\ncpu: 1015.6904780321782 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 10390.350074685291,
            "unit": "ns/iter",
            "extra": "iterations: 67617\ncpu: 10389.96852862445 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 108682.76446217716,
            "unit": "ns/iter",
            "extra": "iterations: 6517\ncpu: 108674.57664569598 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 446366.4929936374,
            "unit": "ns/iter",
            "extra": "iterations: 1570\ncpu: 446342.9426751596 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 402219.80698740244,
            "unit": "ns/iter",
            "extra": "iterations: 1746\ncpu: 402180.1746849942 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 320410.75034418696,
            "unit": "ns/iter",
            "extra": "iterations: 2179\ncpu: 320383.9357503443 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 67645413.55555467,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 67643245.5555555 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20612.181978124805,
            "unit": "ns/iter",
            "extra": "iterations: 34103\ncpu: 20609.854734187593 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 319625.1733830223,
            "unit": "ns/iter",
            "extra": "iterations: 2134\ncpu: 319498.8181818378 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "noam.chn1@gmail.com",
            "name": "Noam Cohen",
            "username": "nanocoh"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "41eec8385a8377dafc395f6be080d0d6831e8496",
          "message": "Liberty: model ff_bank multibit registers (#470)\n\n* Liberty: model ff_bank multibit registers\n\nThe structural Liberty parse kept only `ff` and `latch` groups, so an\n`ff_bank (IQ, IQN, N)` cell loaded with no sequential model and its bundle\noutputs had no truth table. Formal checks then treated every multibit flop\n(for example the asap7 DFFHQNV2X/DFFHQNH2V2X trays created by multibit\nclustering) as opaque.\n\n- YosysLibertyParser: keep `ff_bank` and `latch_bank` groups, their\n  clocked_on/next_state/clear/preset children, and `function` under\n  `bundle` in structural mode.\n- SNLLibertyConstructor: classify `ff_bank` cells as sequential, apply a\n  bundle-level `function` to each member, and expand a bank into N states\n  where bit i reads bundle member i (`D` -> `D<i>`) and output member i maps\n  to state i. Banks that cannot be modeled log a warning and stay loadable.\n- SNLBooleanTree: optional name-to-term aliases so bundle names resolve per\n  bit inside expressions.\n- Test: ff_bank.lib fixture and SNLLibertyConstructorTest1.testFFBank.\n\n* Liberty: cover malformed ff_bank groups\n\nAdd a fixture with a bank that has too few arguments, a non-numeric width,\na zero width, and a next_state that does not resolve. Each cell must stay\nloadable with no sequential model attached.",
          "timestamp": "2026-10-15T17:27:47+02:00",
          "tree_id": "2e346dc26ecdb945a18ff5096949c33e2d44f54b",
          "url": "https://github.com/najaeda/naja/commit/41eec8385a8377dafc395f6be080d0d6831e8496"
        },
        "date": 1792078226075,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 221547.88314961968,
            "unit": "ns/iter",
            "extra": "iterations: 3175\ncpu: 221544.21511811027 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 318726.62699498306,
            "unit": "ns/iter",
            "extra": "iterations: 2193\ncpu: 318675.3556771546 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1810366.0048426471,
            "unit": "ns/iter",
            "extra": "iterations: 413\ncpu: 1810192.7772397096 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18947396.054053064,
            "unit": "ns/iter",
            "extra": "iterations: 37\ncpu: 18943319.189189192 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1230.1175888221587,
            "unit": "ns/iter",
            "extra": "iterations: 571704\ncpu: 1230.0168321369097 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12136.842791037203,
            "unit": "ns/iter",
            "extra": "iterations: 57484\ncpu: 12135.230968617354 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 179266.79091621342,
            "unit": "ns/iter",
            "extra": "iterations: 3831\ncpu: 179251.63456016712 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 501501.5509999898,
            "unit": "ns/iter",
            "extra": "iterations: 1000\ncpu: 501470.8300000006 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 454530.61533460487,
            "unit": "ns/iter",
            "extra": "iterations: 1539\ncpu: 454453.1578947371 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 367691.1222455558,
            "unit": "ns/iter",
            "extra": "iterations: 1906\ncpu: 367669.44753410306 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 67731813.62499514,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 67724914.00000003 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20014.593436435796,
            "unit": "ns/iter",
            "extra": "iterations: 34981\ncpu: 20013.528458305915 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 407779.47267427243,
            "unit": "ns/iter",
            "extra": "iterations: 1720\ncpu: 407592.3837209341 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "66c140523457f6ef0a2259791c363c3e60eb9d64",
          "message": "Fix/procedural concat assignment (#471)\n\n* fix procedural concat assignment\n\n* Fix mem inference\n\n* coverage",
          "timestamp": "2026-10-15T19:45:54+02:00",
          "tree_id": "341b2001142dbddcda7aa0cc878607db16358273",
          "url": "https://github.com/najaeda/naja/commit/66c140523457f6ef0a2259791c363c3e60eb9d64"
        },
        "date": 1792086790915,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 225187.5208007523,
            "unit": "ns/iter",
            "extra": "iterations: 3197\ncpu: 225159.29715358152 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 312175.5987054939,
            "unit": "ns/iter",
            "extra": "iterations: 2163\ncpu: 312064.01987979666 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1822234.967901253,
            "unit": "ns/iter",
            "extra": "iterations: 405\ncpu: 1821814.8567901237 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18090332.297296718,
            "unit": "ns/iter",
            "extra": "iterations: 37\ncpu: 18088224.918918923 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1226.4091437245074,
            "unit": "ns/iter",
            "extra": "iterations: 573180\ncpu: 1226.173491747793 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12187.410945342745,
            "unit": "ns/iter",
            "extra": "iterations: 57577\ncpu: 12186.340274762497 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 152146.0936073078,
            "unit": "ns/iter",
            "extra": "iterations: 4380\ncpu: 152135.1840182648 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 470021.45973155147,
            "unit": "ns/iter",
            "extra": "iterations: 1490\ncpu: 469941.77718120796 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 421810.092002404,
            "unit": "ns/iter",
            "extra": "iterations: 1663\ncpu: 421751.6662657849 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 336183.1233205348,
            "unit": "ns/iter",
            "extra": "iterations: 2084\ncpu: 336130.5590211129 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 67506789.22222164,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 67503172.55555554 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 19877.564625077575,
            "unit": "ns/iter",
            "extra": "iterations: 35234\ncpu: 19874.884316285395 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 404910.42940176703,
            "unit": "ns/iter",
            "extra": "iterations: 1721\ncpu: 404754.773387535 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "a5c2eba33f287c694b66e483e68ba63925cee315",
          "message": "Fix/vhdl conversions (#469)\n\n* improve VHDL loading\n\n* added support\n\n* new VHDL support\n\n* add ROM support\n\n* add ROM support\n\n* more support\n\n* coverage\n\n* fix RAM generation linting issue",
          "timestamp": "2026-10-16T15:11:57+02:00",
          "tree_id": "c94faed871a05dce6a9ef359178ef33b5a5b815c",
          "url": "https://github.com/najaeda/naja/commit/a5c2eba33f287c694b66e483e68ba63925cee315"
        },
        "date": 1792156471301,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 193501.04778760773,
            "unit": "ns/iter",
            "extra": "iterations: 3390\ncpu: 193475.96607669618 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 290669.2380556718,
            "unit": "ns/iter",
            "extra": "iterations: 2407\ncpu: 290639.7852098048 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1685722.858796255,
            "unit": "ns/iter",
            "extra": "iterations: 432\ncpu: 1685468.45138889 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18660553.48648695,
            "unit": "ns/iter",
            "extra": "iterations: 37\ncpu: 18658478 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 995.2221740988982,
            "unit": "ns/iter",
            "extra": "iterations: 699591\ncpu: 995.1530708656909 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 10365.337499636273,
            "unit": "ns/iter",
            "extra": "iterations: 68726\ncpu: 10364.473634432385 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 110928.0119124271,
            "unit": "ns/iter",
            "extra": "iterations: 6212\ncpu: 110926.8147134578 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 443757.4034307538,
            "unit": "ns/iter",
            "extra": "iterations: 1574\ncpu: 443735.0749682334 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 401705.1682996964,
            "unit": "ns/iter",
            "extra": "iterations: 1735\ncpu: 401694.50144092215 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 321337.7172413852,
            "unit": "ns/iter",
            "extra": "iterations: 2175\ncpu: 321327.95908045943 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 68945342.66666597,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 68941132.33333322 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20550.034821296595,
            "unit": "ns/iter",
            "extra": "iterations: 33715\ncpu: 20549.311434079802 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 321512.3703197186,
            "unit": "ns/iter",
            "extra": "iterations: 2190\ncpu: 321384.60136989626 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "d08aaa9c000ebc1ff15c9d2d0958d833ca5de90f",
          "message": "fix SV automatic local initializers (#472)",
          "timestamp": "2026-10-16T23:16:50+02:00",
          "tree_id": "ac0f05fae4f5ca531976f7c2b79b24dc21310aaa",
          "url": "https://github.com/najaeda/naja/commit/d08aaa9c000ebc1ff15c9d2d0958d833ca5de90f"
        },
        "date": 1792185562890,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 220745.41445336348,
            "unit": "ns/iter",
            "extra": "iterations: 3238\ncpu: 220708.994441013 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 309437.5526548666,
            "unit": "ns/iter",
            "extra": "iterations: 2260\ncpu: 309402.74424778763 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1776289.7810945164,
            "unit": "ns/iter",
            "extra": "iterations: 402\ncpu: 1776202.116915423 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17001843.05000008,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 16999779.699999996 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1213.85192325588,
            "unit": "ns/iter",
            "extra": "iterations: 569529\ncpu: 1213.8322052081628 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12033.413053165707,
            "unit": "ns/iter",
            "extra": "iterations: 58346\ncpu: 12032.773129263358 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 170310.32696245765,
            "unit": "ns/iter",
            "extra": "iterations: 4395\ncpu: 170304.1349260522 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 459692.2348684199,
            "unit": "ns/iter",
            "extra": "iterations: 1520\ncpu: 459656.969078947 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 414346.6935388229,
            "unit": "ns/iter",
            "extra": "iterations: 1687\ncpu: 414338.8648488436 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 330808.809749171,
            "unit": "ns/iter",
            "extra": "iterations: 2113\ncpu: 330776.623284429 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 66681777.888886824,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 66676943.555555545 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20058.65885431661,
            "unit": "ns/iter",
            "extra": "iterations: 34774\ncpu: 20057.137660320946 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 405539.297046522,
            "unit": "ns/iter",
            "extra": "iterations: 1727\ncpu: 405427.83381590183 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "db819aef8a1de3f631965d77d8df7a8acec4108a",
          "message": "fix loop variable selects (#473)\n\n* fix loop variable selects\n\n* more fix\n\n* coverage",
          "timestamp": "2026-10-17T10:06:05+02:00",
          "tree_id": "ca8c8402dad98f6ca9902efda6232e300fa581a3",
          "url": "https://github.com/najaeda/naja/commit/db819aef8a1de3f631965d77d8df7a8acec4108a"
        },
        "date": 1792227311733,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 216654.9122532074,
            "unit": "ns/iter",
            "extra": "iterations: 3191\ncpu: 216616.15418364148 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 311280.762149536,
            "unit": "ns/iter",
            "extra": "iterations: 2140\ncpu: 311217.20560747664 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1796832.9262899128,
            "unit": "ns/iter",
            "extra": "iterations: 407\ncpu: 1796639.4717444717 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18442250.66666638,
            "unit": "ns/iter",
            "extra": "iterations: 36\ncpu: 18438702.77777779 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1222.6587564500378,
            "unit": "ns/iter",
            "extra": "iterations: 575964\ncpu: 1222.4464706127474 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12060.574318517865,
            "unit": "ns/iter",
            "extra": "iterations: 58182\ncpu: 12057.840569248221 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 155117.1097108978,
            "unit": "ns/iter",
            "extra": "iterations: 4047\ncpu: 155098.20978502583 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 500443.80500000325,
            "unit": "ns/iter",
            "extra": "iterations: 1000\ncpu: 500327.0520000003 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 451980.4238709786,
            "unit": "ns/iter",
            "extra": "iterations: 1550\ncpu: 451892.4432258065 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 367470.9910620364,
            "unit": "ns/iter",
            "extra": "iterations: 1902\ncpu: 367394.6151419558 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 67649357.7499997,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 67640559.50000003 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20073.65162770681,
            "unit": "ns/iter",
            "extra": "iterations: 34865\ncpu: 20070.724766958254 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 412564.70246405614,
            "unit": "ns/iter",
            "extra": "iterations: 1704\ncpu: 412415.0927230242 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "270df2383a2ce3d3d8e55d88ebd57a3f5377ab24",
          "message": "Fix/sv unpacked array rmw (#474)\n\n* fix loop variable selects\n\n* more fix\n\n* coverage\n\n* fix unpacked array",
          "timestamp": "2026-10-17T14:39:08+02:00",
          "tree_id": "51537e09835d4fca4497c8705bd8a0fb1cd2dbe5",
          "url": "https://github.com/najaeda/naja/commit/270df2383a2ce3d3d8e55d88ebd57a3f5377ab24"
        },
        "date": 1792240900829,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 216041.62074554022,
            "unit": "ns/iter",
            "extra": "iterations: 3085\ncpu: 216023.5173419773 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 312873.4235768746,
            "unit": "ns/iter",
            "extra": "iterations: 2231\ncpu: 312802.27521290904 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1786019.5746268758,
            "unit": "ns/iter",
            "extra": "iterations: 402\ncpu: 1785577.9626865676 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17153495.025640864,
            "unit": "ns/iter",
            "extra": "iterations: 39\ncpu: 17151701.102564108 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1216.4244653939188,
            "unit": "ns/iter",
            "extra": "iterations: 553819\ncpu: 1216.361448415457 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12044.935737433882,
            "unit": "ns/iter",
            "extra": "iterations: 58012\ncpu: 12042.46116320761 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 178296.68074589586,
            "unit": "ns/iter",
            "extra": "iterations: 4451\ncpu: 178281.63086946754 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 496983.824449183,
            "unit": "ns/iter",
            "extra": "iterations: 1407\ncpu: 496958.95735607686 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 451986.5341494714,
            "unit": "ns/iter",
            "extra": "iterations: 1552\ncpu: 451942.21198453545 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 364999.2339089494,
            "unit": "ns/iter",
            "extra": "iterations: 1911\ncpu: 364986.0852956567 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 68035408.44444455,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 68019094.33333343 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20491.196933688778,
            "unit": "ns/iter",
            "extra": "iterations: 34504\ncpu: 20490.350365175058 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 410359.9252164094,
            "unit": "ns/iter",
            "extra": "iterations: 1725\ncpu: 410187.3588405715 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "2ec6c06e7313f1303e330e257b6e4f415fb57cb8",
          "message": "Fix determinism (#475)\n\n* Fix determinism\n\n* fix github run",
          "timestamp": "2026-10-17T16:21:17+02:00",
          "tree_id": "b6f870f4e0ebee97ef86f2b6e052eeaa3ba1d578",
          "url": "https://github.com/najaeda/naja/commit/2ec6c06e7313f1303e330e257b6e4f415fb57cb8"
        },
        "date": 1792247032486,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 224147.95298200776,
            "unit": "ns/iter",
            "extra": "iterations: 3169\ncpu: 224113.5579047018 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 308679.33377541596,
            "unit": "ns/iter",
            "extra": "iterations: 2262\ncpu: 308652.8249336869 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1793149.6608478713,
            "unit": "ns/iter",
            "extra": "iterations: 401\ncpu: 1792786.9376558603 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17676770.589743614,
            "unit": "ns/iter",
            "extra": "iterations: 39\ncpu: 17673025.615384616 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1213.3187187638089,
            "unit": "ns/iter",
            "extra": "iterations: 577255\ncpu: 1212.976894093599 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12067.791159403017,
            "unit": "ns/iter",
            "extra": "iterations: 58073\ncpu: 12066.356069085463 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 175586.1928588231,
            "unit": "ns/iter",
            "extra": "iterations: 4257\ncpu: 175560.38783180638 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 495843.72965320415,
            "unit": "ns/iter",
            "extra": "iterations: 1413\ncpu: 495740.16135881084 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 449719.73873285565,
            "unit": "ns/iter",
            "extra": "iterations: 1531\ncpu: 449681.1966035278 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 367297.65146751,
            "unit": "ns/iter",
            "extra": "iterations: 1908\ncpu: 367274.81603773555 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 67219345.99999812,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 67208540.50000003 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20091.382897712567,
            "unit": "ns/iter",
            "extra": "iterations: 34931\ncpu: 20090.221980475784 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 399885.21883836837,
            "unit": "ns/iter",
            "extra": "iterations: 1773\ncpu: 399668.63056964864 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "c7f04bebddbe0f465641b3e94c27c34260457e23",
          "message": "fix sv function loop cast return (#476)",
          "timestamp": "2026-10-17T18:26:23+02:00",
          "tree_id": "e7bb620359a3b671cf3a9d68f5e9f383e3d50ef7",
          "url": "https://github.com/najaeda/naja/commit/c7f04bebddbe0f465641b3e94c27c34260457e23"
        },
        "date": 1792254542250,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 216659.81855217842,
            "unit": "ns/iter",
            "extra": "iterations: 3191\ncpu: 216618.19586336572 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 305307.25690487324,
            "unit": "ns/iter",
            "extra": "iterations: 2281\ncpu: 305292.99167032 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1761950.286782991,
            "unit": "ns/iter",
            "extra": "iterations: 401\ncpu: 1761672.4364089784 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17010221.700000018,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17007568.275 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1216.0823315618138,
            "unit": "ns/iter",
            "extra": "iterations: 576146\ncpu: 1215.882706813898 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12108.783022384761,
            "unit": "ns/iter",
            "extra": "iterations: 56333\ncpu: 12106.847247616837 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 188423.1189913289,
            "unit": "ns/iter",
            "extra": "iterations: 3807\ncpu: 188393.587339112 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 495585.8223169795,
            "unit": "ns/iter",
            "extra": "iterations: 1407\ncpu: 495565.09381663124 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 448974.35769230424,
            "unit": "ns/iter",
            "extra": "iterations: 1560\ncpu: 448942.1044871788 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 363455.90374608943,
            "unit": "ns/iter",
            "extra": "iterations: 1922\ncpu: 363445.86472424574 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 67135134.77777787,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 67125042.77777773 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20132.974317724773,
            "unit": "ns/iter",
            "extra": "iterations: 34810\ncpu: 20131.458402757817 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 386654.8787882161,
            "unit": "ns/iter",
            "extra": "iterations: 1815\ncpu: 386560.21542699216 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "committer": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "distinct": true,
          "id": "217a85474042f6c5a38e1b6217427ea94f10e51e",
          "message": "add ibex regress",
          "timestamp": "2026-10-18T11:37:08+02:00",
          "tree_id": "a55f9b0edb0fde829d27c2e1868d165e829de3ca",
          "url": "https://github.com/najaeda/naja/commit/217a85474042f6c5a38e1b6217427ea94f10e51e"
        },
        "date": 1792316378692,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 190280.0451647168,
            "unit": "ns/iter",
            "extra": "iterations: 3764\ncpu: 190272.61769394265 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 285633.29745274433,
            "unit": "ns/iter",
            "extra": "iterations: 2434\ncpu: 285612.09408381267 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1732561.5135134785,
            "unit": "ns/iter",
            "extra": "iterations: 407\ncpu: 1732247.1621621617 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17850403.358974557,
            "unit": "ns/iter",
            "extra": "iterations: 39\ncpu: 17848989.02564102 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1089.8556269699966,
            "unit": "ns/iter",
            "extra": "iterations: 640549\ncpu: 1089.7954879330077 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 11181.039026651022,
            "unit": "ns/iter",
            "extra": "iterations: 64725\ncpu: 11180.568173039777 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 109765.06882087844,
            "unit": "ns/iter",
            "extra": "iterations: 6437\ncpu: 109755.64191393505 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 429196.57282741874,
            "unit": "ns/iter",
            "extra": "iterations: 1634\ncpu: 429178.2405140759 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 386230.34065935103,
            "unit": "ns/iter",
            "extra": "iterations: 1820\ncpu: 386205.132417583 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 306036.1488616439,
            "unit": "ns/iter",
            "extra": "iterations: 2284\ncpu: 306012.5586690013 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 67334884.88888876,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 67331734.88888887 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 23588.971750838937,
            "unit": "ns/iter",
            "extra": "iterations: 29523\ncpu: 23587.828743691374 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 294895.3626828723,
            "unit": "ns/iter",
            "extra": "iterations: 2385\ncpu: 294778.4083857291 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "committer": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "distinct": true,
          "id": "faffdae67ad7593870e37f4a010fb65802ed72de",
          "message": "switch ibex to central one",
          "timestamp": "2026-10-18T14:47:48+02:00",
          "tree_id": "57ef33879010d29258732f03be2b8682f6a64290",
          "url": "https://github.com/najaeda/naja/commit/faffdae67ad7593870e37f4a010fb65802ed72de"
        },
        "date": 1792327810073,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 161442.00731428748,
            "unit": "ns/iter",
            "extra": "iterations: 4375\ncpu: 161416.3414857143 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 246816.3918003529,
            "unit": "ns/iter",
            "extra": "iterations: 2805\ncpu: 246752.6606060607 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1512835.333333354,
            "unit": "ns/iter",
            "extra": "iterations: 474\ncpu: 1512745.2552742613 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 15329032.26086917,
            "unit": "ns/iter",
            "extra": "iterations: 46\ncpu: 15327845.23913044 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 938.8911842149423,
            "unit": "ns/iter",
            "extra": "iterations: 742475\ncpu: 938.877924509243 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 9888.763760413847,
            "unit": "ns/iter",
            "extra": "iterations: 69620\ncpu: 9887.08276357369 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 92155.26957001239,
            "unit": "ns/iter",
            "extra": "iterations: 7256\ncpu: 92153.16400220506 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 364832.9690288613,
            "unit": "ns/iter",
            "extra": "iterations: 1905\ncpu: 364778.71338582673 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 331990.2056470496,
            "unit": "ns/iter",
            "extra": "iterations: 2125\ncpu: 331961.69788235315 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 261957.3987101695,
            "unit": "ns/iter",
            "extra": "iterations: 2636\ncpu: 261960.72344461305 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 55938243.4999975,
            "unit": "ns/iter",
            "extra": "iterations: 10\ncpu: 55931410.20000001 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20426.001734455265,
            "unit": "ns/iter",
            "extra": "iterations: 34593\ncpu: 20425.1741681843 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 251580.29756247884,
            "unit": "ns/iter",
            "extra": "iterations: 2749\ncpu: 251535.64496182997 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "92ce68813d614f192dcf88934ddf244863c8e26c",
          "message": "adding memory interface python API (#477)",
          "timestamp": "2026-10-19T08:53:11+02:00",
          "tree_id": "870589170e4e7e181f828d3ebd5ba182407a9ddf",
          "url": "https://github.com/najaeda/naja/commit/92ce68813d614f192dcf88934ddf244863c8e26c"
        },
        "date": 1792392948019,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 221320.85931308096,
            "unit": "ns/iter",
            "extra": "iterations: 3028\ncpu: 221285.17404227212 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 315323.80840488366,
            "unit": "ns/iter",
            "extra": "iterations: 2213\ncpu: 315288.34206958883 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1749808.3017456348,
            "unit": "ns/iter",
            "extra": "iterations: 401\ncpu: 1749286.1521197008 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17193081.53658556,
            "unit": "ns/iter",
            "extra": "iterations: 41\ncpu: 17190332.70731708 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1217.85210017678,
            "unit": "ns/iter",
            "extra": "iterations: 567357\ncpu: 1217.6071732612804 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12038.715855168048,
            "unit": "ns/iter",
            "extra": "iterations: 58164\ncpu: 12036.667148064093 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 164634.67592356857,
            "unit": "ns/iter",
            "extra": "iterations: 3925\ncpu: 164620.28178343957 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 471925.5132616454,
            "unit": "ns/iter",
            "extra": "iterations: 1395\ncpu: 471843.7820788533 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 422850.5685920734,
            "unit": "ns/iter",
            "extra": "iterations: 1662\ncpu: 422747.67749699205 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 371452.0660049593,
            "unit": "ns/iter",
            "extra": "iterations: 2015\ncpu: 371340.9875930518 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 66914675.74999876,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 66910960.249999985 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20080.932377812685,
            "unit": "ns/iter",
            "extra": "iterations: 34885\ncpu: 20078.929339257546 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 394522.4294614776,
            "unit": "ns/iter",
            "extra": "iterations: 1765\ncpu: 394386.52747875295 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "7521bc7df142c84ef2cf22dfdef6b5ca606c1c20",
          "message": "advance VHDL loading (#478)\n\n* advance VHDL loading\n\n* coverage\n\n* clean docker support\n\n* fix rockylinux",
          "timestamp": "2026-10-19T11:02:00+02:00",
          "tree_id": "6295672683c58175d4ba7c5b6ee252165dff8b8b",
          "url": "https://github.com/najaeda/naja/commit/7521bc7df142c84ef2cf22dfdef6b5ca606c1c20"
        },
        "date": 1792401084219,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 219601.7260012691,
            "unit": "ns/iter",
            "extra": "iterations: 3146\ncpu: 219531.13827082008 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 311654.1049575622,
            "unit": "ns/iter",
            "extra": "iterations: 2239\ncpu: 311533.7476552032 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1831656.6553525212,
            "unit": "ns/iter",
            "extra": "iterations: 383\ncpu: 1831123.1984334206 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 19097153.749999367,
            "unit": "ns/iter",
            "extra": "iterations: 36\ncpu: 19095352.166666675 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1214.0511755298312,
            "unit": "ns/iter",
            "extra": "iterations: 576506\ncpu: 1213.6915227248292 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12042.473735658408,
            "unit": "ns/iter",
            "extra": "iterations: 58311\ncpu: 12042.258527550543 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 179255.96951692298,
            "unit": "ns/iter",
            "extra": "iterations: 3871\ncpu: 179231.93076724367 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 498679.672142848,
            "unit": "ns/iter",
            "extra": "iterations: 1400\ncpu: 498640.4207142854 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 452274.5887669452,
            "unit": "ns/iter",
            "extra": "iterations: 1549\ncpu: 452200.56810845743 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 368426.32738719205,
            "unit": "ns/iter",
            "extra": "iterations: 1906\ncpu: 368401.5020986362 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 69039766.625,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 69032386.75000001 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20152.061307511725,
            "unit": "ns/iter",
            "extra": "iterations: 34906\ncpu: 20151.59519853318 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 388595.4941697569,
            "unit": "ns/iter",
            "extra": "iterations: 1801\ncpu: 388491.0832870039 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "5008d391f418cfc045a057a2bba7b441a8c51d9c",
          "message": "clarify gate level verilog (#479)\n\n* clarify gate level verilog\n\n* fix license",
          "timestamp": "2026-10-20T12:41:59+02:00",
          "tree_id": "1466c0e4eeb050f0f61e63daeb1bcc2d866f3a3e",
          "url": "https://github.com/najaeda/naja/commit/5008d391f418cfc045a057a2bba7b441a8c51d9c"
        },
        "date": 1792493344430,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 191892.27715465965,
            "unit": "ns/iter",
            "extra": "iterations: 3388\ncpu: 191867.46605667062 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 284405.72317072435,
            "unit": "ns/iter",
            "extra": "iterations: 2460\ncpu: 284369.7195121952 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1711482.0024154275,
            "unit": "ns/iter",
            "extra": "iterations: 414\ncpu: 1711173.7246376812 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17661185.50000044,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17660614.824999996 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1278.0765220347232,
            "unit": "ns/iter",
            "extra": "iterations: 547228\ncpu: 1277.8931944271849 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12574.395125355926,
            "unit": "ns/iter",
            "extra": "iterations: 55881\ncpu: 12572.721676419547 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 136346.2222882019,
            "unit": "ns/iter",
            "extra": "iterations: 5052\ncpu: 136326.67478226445 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 439099.8177278478,
            "unit": "ns/iter",
            "extra": "iterations: 1602\ncpu: 439068.4719101122 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 396863.0317100919,
            "unit": "ns/iter",
            "extra": "iterations: 1766\ncpu: 396803.7825594567 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 327713.4708646603,
            "unit": "ns/iter",
            "extra": "iterations: 2128\ncpu: 327696.33599624055 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 66852090.874998994,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 66838103.37500007 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 21023.9679575572,
            "unit": "ns/iter",
            "extra": "iterations: 33362\ncpu: 21022.376686049964 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 372977.07397605473,
            "unit": "ns/iter",
            "extra": "iterations: 1879\ncpu: 372859.9116551646 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "6a7e772af6220e2bae6edd5aeaa31b8e335f4f4e",
          "message": "add new primitives for SV lowering (#480)\n\n* add new primitives for SV lowering\n\n* fixes\n\n* regold",
          "timestamp": "2026-10-20T17:40:55+02:00",
          "tree_id": "28fc17e744a5b96ea45e7d5e3ffeef00cc7b7ae6",
          "url": "https://github.com/najaeda/naja/commit/6a7e772af6220e2bae6edd5aeaa31b8e335f4f4e"
        },
        "date": 1792510988569,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 153251.8445331562,
            "unit": "ns/iter",
            "extra": "iterations: 4509\ncpu: 153200.0583277889 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 223072.682600386,
            "unit": "ns/iter",
            "extra": "iterations: 3138\ncpu: 223000.47450605486 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1359596.6192236883,
            "unit": "ns/iter",
            "extra": "iterations: 541\ncpu: 1359498.8798521257 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 13954015.97999978,
            "unit": "ns/iter",
            "extra": "iterations: 50\ncpu: 13952893.1 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 988.2539426386562,
            "unit": "ns/iter",
            "extra": "iterations: 708282\ncpu: 987.80839834981 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 9685.345114488891,
            "unit": "ns/iter",
            "extra": "iterations: 72234\ncpu: 9680.638785059666 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 109415.15257793764,
            "unit": "ns/iter",
            "extra": "iterations: 6672\ncpu: 109261.85611510779 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 343417.5876937912,
            "unit": "ns/iter",
            "extra": "iterations: 2064\ncpu: 343233.1889534886 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 306660.65789474035,
            "unit": "ns/iter",
            "extra": "iterations: 2280\ncpu: 306602.95219298225 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 252440.77039166086,
            "unit": "ns/iter",
            "extra": "iterations: 2783\ncpu: 252384.819259792 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 53511280.8000008,
            "unit": "ns/iter",
            "extra": "iterations: 10\ncpu: 53508176.10000007 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 16164.685804691175,
            "unit": "ns/iter",
            "extra": "iterations: 43613\ncpu: 16162.845206704456 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 285870.5932893214,
            "unit": "ns/iter",
            "extra": "iterations: 2444\ncpu: 285709.92103110935 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "dc206539b9a78c9de516e7dfbff365a6e36cd4d7",
          "message": "Tgraph (#462)\n\n* init tgraph\n\n* tgraph in progress\n\n* add INIT parameter in naja primitives\n\n* avoid conflict with naja.py user script\n\n* ongoing work\n\n* towards first level of CDC support\n\n* add new property types\n\n* Adding NLClock and associated accessors\n\n* cleaning\n\n* missing exported header\n\n* add node roles and capture edges\n\n* run Clock propagation\n\n* TGraph construction\n\n* coverage and testing\n\n* testing and coverage\n\n* update doc",
          "timestamp": "2026-10-20T20:03:12+02:00",
          "tree_id": "3e49112d4b29808ffb386e13409a38047caeeaf0",
          "url": "https://github.com/najaeda/naja/commit/dc206539b9a78c9de516e7dfbff365a6e36cd4d7"
        },
        "date": 1792520472810,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 188650.2708894824,
            "unit": "ns/iter",
            "extra": "iterations: 3710\ncpu: 188617.769541779 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 285375.50911300775,
            "unit": "ns/iter",
            "extra": "iterations: 2469\ncpu: 285338.39246658573 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1705306.9857482736,
            "unit": "ns/iter",
            "extra": "iterations: 421\ncpu: 1705083.771971497 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17733186.526316416,
            "unit": "ns/iter",
            "extra": "iterations: 38\ncpu: 17732393.23684212 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 979.3228270995894,
            "unit": "ns/iter",
            "extra": "iterations: 712688\ncpu: 979.2441306714858 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 10223.153659546828,
            "unit": "ns/iter",
            "extra": "iterations: 67604\ncpu: 10223.079122537125 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 107883.95369211363,
            "unit": "ns/iter",
            "extra": "iterations: 6392\ncpu: 107884.02894242803 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 439913.2826498253,
            "unit": "ns/iter",
            "extra": "iterations: 1585\ncpu: 439895.5911671922 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 395278.49858996953,
            "unit": "ns/iter",
            "extra": "iterations: 1773\ncpu: 395254.19232938543 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 318507.53194381687,
            "unit": "ns/iter",
            "extra": "iterations: 2207\ncpu: 318499.0348889893 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 66007805.11110493,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 66004631.55555561 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 21082.021226485133,
            "unit": "ns/iter",
            "extra": "iterations: 33119\ncpu: 21081.53923729579 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 307128.2574087703,
            "unit": "ns/iter",
            "extra": "iterations: 2261\ncpu: 307040.84829723 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "8420eb9030b014fc17f4a8d5f301d42501493ac4",
          "message": "expose default timescale (#481)",
          "timestamp": "2026-10-21T00:24:26+02:00",
          "tree_id": "dbf6a6f806accbb396704607e67a86d9bab79629",
          "url": "https://github.com/najaeda/naja/commit/8420eb9030b014fc17f4a8d5f301d42501493ac4"
        },
        "date": 1792535328703,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 153259.70562875073,
            "unit": "ns/iter",
            "extra": "iterations: 4175\ncpu: 153184.33772455092 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 219843.33269841276,
            "unit": "ns/iter",
            "extra": "iterations: 3150\ncpu: 219833.61587301583 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1311837.3187614556,
            "unit": "ns/iter",
            "extra": "iterations: 549\ncpu: 1311790.7559198544 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 14717014.784313211,
            "unit": "ns/iter",
            "extra": "iterations: 51\ncpu: 14716179.666666672 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 978.1759514329942,
            "unit": "ns/iter",
            "extra": "iterations: 712583\ncpu: 978.1565628144377 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 9627.19811411642,
            "unit": "ns/iter",
            "extra": "iterations: 72645\ncpu: 9626.26194507537 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 107192.71688467688,
            "unit": "ns/iter",
            "extra": "iterations: 6651\ncpu: 107187.63900165386 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 334235.44770807785,
            "unit": "ns/iter",
            "extra": "iterations: 2247\ncpu: 334206.3279928793 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 304733.17885118315,
            "unit": "ns/iter",
            "extra": "iterations: 2298\ncpu: 304627.88033072214 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 249958.76129489864,
            "unit": "ns/iter",
            "extra": "iterations: 2811\ncpu: 249879.93596584888 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 54075272.30000255,
            "unit": "ns/iter",
            "extra": "iterations: 10\ncpu: 54069438.800000034 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 16461.457334340892,
            "unit": "ns/iter",
            "extra": "iterations: 42376\ncpu: 16460.401689635644 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 285045.72372750513,
            "unit": "ns/iter",
            "extra": "iterations: 2436\ncpu: 284936.14573070436 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "d29fef47eab121ef00cf4786c53774e5468f9a0a",
          "message": "Fix sequential incrementer lowering (#482)\n\n* expose default timescale\n\n* new SV fix",
          "timestamp": "2026-10-21T00:28:55+02:00",
          "tree_id": "53dd954a8b27542fe8fb7ae5773781ea05874b88",
          "url": "https://github.com/najaeda/naja/commit/d29fef47eab121ef00cf4786c53774e5468f9a0a"
        },
        "date": 1792535647293,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 157328.31029151642,
            "unit": "ns/iter",
            "extra": "iterations: 4528\ncpu: 157324.8734540636 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 240615.1079387134,
            "unit": "ns/iter",
            "extra": "iterations: 2872\ncpu: 240614.012534819 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1494980.9159663522,
            "unit": "ns/iter",
            "extra": "iterations: 476\ncpu: 1494942.6344537816 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 15680004.355555687,
            "unit": "ns/iter",
            "extra": "iterations: 45\ncpu: 15679966.911111105 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 936.5023286768891,
            "unit": "ns/iter",
            "extra": "iterations: 746776\ncpu: 936.5039864698377 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 9657.193900319098,
            "unit": "ns/iter",
            "extra": "iterations: 73053\ncpu: 9655.54359163894 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 95674.4977967705,
            "unit": "ns/iter",
            "extra": "iterations: 7489\ncpu: 95673.39591400712 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 323630.3348623776,
            "unit": "ns/iter",
            "extra": "iterations: 2180\ncpu: 323606.15917431173 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 284193.392342525,
            "unit": "ns/iter",
            "extra": "iterations: 2429\ncpu: 284171.0510498143 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 224467.288517211,
            "unit": "ns/iter",
            "extra": "iterations: 3109\ncpu: 224447.44065615945 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 56593123.59999831,
            "unit": "ns/iter",
            "extra": "iterations: 10\ncpu: 56590036.99999996 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20450.710797471256,
            "unit": "ns/iter",
            "extra": "iterations: 34647\ncpu: 20449.764510635858 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 248296.5517116621,
            "unit": "ns/iter",
            "extra": "iterations: 2833\ncpu: 248239.4511119145 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "committer": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "distinct": true,
          "id": "6de56222ca9d561920effcd6b1820c2486c9842e",
          "message": "fix interface memory",
          "timestamp": "2026-10-21T00:46:17+02:00",
          "tree_id": "d77ceebe4db33878004362439a9ad74dc572e9c3",
          "url": "https://github.com/najaeda/naja/commit/6de56222ca9d561920effcd6b1820c2486c9842e"
        },
        "date": 1792538864944,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 113412.34579150738,
            "unit": "ns/iter",
            "extra": "iterations: 6475\ncpu: 113399.46393822395 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 158555.10060975628,
            "unit": "ns/iter",
            "extra": "iterations: 4264\ncpu: 158537.92636022516 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 928230.5220883548,
            "unit": "ns/iter",
            "extra": "iterations: 747\ncpu: 928082.4886211515 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 10156033.538461447,
            "unit": "ns/iter",
            "extra": "iterations: 65\ncpu: 10155702.984615382 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 588.2473616537633,
            "unit": "ns/iter",
            "extra": "iterations: 1205490\ncpu: 588.2161959037408 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 6712.317820006566,
            "unit": "ns/iter",
            "extra": "iterations: 113193\ncpu: 6711.768386737695 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 63184.043169302124,
            "unit": "ns/iter",
            "extra": "iterations: 11258\ncpu: 63177.311867116725 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 226557.698849104,
            "unit": "ns/iter",
            "extra": "iterations: 3128\ncpu: 226516.06042199503 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 209205.69795070228,
            "unit": "ns/iter",
            "extra": "iterations: 3367\ncpu: 209183.7549747549 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 169441.92191680256,
            "unit": "ns/iter",
            "extra": "iterations: 4111\ncpu: 169419.85453660932 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 37903332.722221546,
            "unit": "ns/iter",
            "extra": "iterations: 18\ncpu: 37899443.99999998 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 12073.200190722764,
            "unit": "ns/iter",
            "extra": "iterations: 58724\ncpu: 12071.851457666367 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 217674.07844330728,
            "unit": "ns/iter",
            "extra": "iterations: 3238\ncpu: 217531.25478689763 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "committer": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "distinct": false,
          "id": "b8841055691025a19d51dcb81791e5e43bd5a0db",
          "message": "update branch rules",
          "timestamp": "2026-10-21T01:06:34+02:00",
          "tree_id": "049a5e4a8c1bc617bef22e38f31353bb5a8f129f",
          "url": "https://github.com/najaeda/naja/commit/b8841055691025a19d51dcb81791e5e43bd5a0db"
        },
        "date": 1792545755085,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 210470.18806907785,
            "unit": "ns/iter",
            "extra": "iterations: 3185\ncpu: 210456.45682888545 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 306440.89795027504,
            "unit": "ns/iter",
            "extra": "iterations: 2293\ncpu: 306402.40470998693 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1634912.8210023844,
            "unit": "ns/iter",
            "extra": "iterations: 419\ncpu: 1634779.916467781 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17584895.761904325,
            "unit": "ns/iter",
            "extra": "iterations: 42\ncpu: 17582554.26190476 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1176.5614962707068,
            "unit": "ns/iter",
            "extra": "iterations: 597833\ncpu: 1176.4109090665795 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 11733.4243991381,
            "unit": "ns/iter",
            "extra": "iterations: 60330\ncpu: 11731.835372120011 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 186450.91978346184,
            "unit": "ns/iter",
            "extra": "iterations: 4064\ncpu: 186434.50738188977 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 490973.6519607935,
            "unit": "ns/iter",
            "extra": "iterations: 1428\ncpu: 490889.86624649854 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 446306.2684948879,
            "unit": "ns/iter",
            "extra": "iterations: 1568\ncpu: 446258.3807397955 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 362770.0824317445,
            "unit": "ns/iter",
            "extra": "iterations: 1941\ncpu: 362732.14734672883 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 68071968.11111124,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 68065118.88888878 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20687.098373719724,
            "unit": "ns/iter",
            "extra": "iterations: 33881\ncpu: 20682.416929842682 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 387371.454445479,
            "unit": "ns/iter",
            "extra": "iterations: 1811\ncpu: 387131.7951408238 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "efcad5ae407b1c3e4205612402354eb1f3f1ecaa",
          "message": "use latest naja-if and preserve attributes (#483)",
          "timestamp": "2026-10-21T08:55:40+02:00",
          "tree_id": "262398a734ac9879b54c5b71654ea44719314892",
          "url": "https://github.com/najaeda/naja/commit/efcad5ae407b1c3e4205612402354eb1f3f1ecaa"
        },
        "date": 1792567118274,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 212243.53219927265,
            "unit": "ns/iter",
            "extra": "iterations: 3292\ncpu: 212161.3860874848 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 306903.34165572305,
            "unit": "ns/iter",
            "extra": "iterations: 2283\ncpu: 306791.41217696026 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1727613.6446078282,
            "unit": "ns/iter",
            "extra": "iterations: 408\ncpu: 1726888.083333333 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 16992597.774999753,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 16986046.675 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1200.6289231770818,
            "unit": "ns/iter",
            "extra": "iterations: 583615\ncpu: 1200.1270717853379 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 11945.691577637963,
            "unit": "ns/iter",
            "extra": "iterations: 58689\ncpu: 11944.91131217093 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 160283.44093437734,
            "unit": "ns/iter",
            "extra": "iterations: 4495\ncpu: 160281.07408231377 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 495195.1159830216,
            "unit": "ns/iter",
            "extra": "iterations: 1414\ncpu: 494987.0622347949 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 446578.33290570445,
            "unit": "ns/iter",
            "extra": "iterations: 1559\ncpu: 446357.7402180892 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 361957.07648579445,
            "unit": "ns/iter",
            "extra": "iterations: 1935\ncpu: 361781.1612403098 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 68598644.00000149,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 68594191.00000009 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20829.74988907003,
            "unit": "ns/iter",
            "extra": "iterations: 33805\ncpu: 20829.14855790564 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 385759.85887534026,
            "unit": "ns/iter",
            "extra": "iterations: 1814\ncpu: 385565.7205071418 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "f03fd803157263120cf6cc9e4dfaa2c78210bac7",
          "message": "update README (#486)",
          "timestamp": "2026-10-21T10:09:46+02:00",
          "tree_id": "69258c7e434fbbb1936699184eb9b29ef881e3ca",
          "url": "https://github.com/najaeda/naja/commit/f03fd803157263120cf6cc9e4dfaa2c78210bac7"
        },
        "date": 1792571110836,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 154853.36328915678,
            "unit": "ns/iter",
            "extra": "iterations: 4451\ncpu: 154806.83733992363 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 227859.87764143007,
            "unit": "ns/iter",
            "extra": "iterations: 2934\ncpu: 227769.12747102932 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1307610.9073724728,
            "unit": "ns/iter",
            "extra": "iterations: 529\ncpu: 1307174.7807183366 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 14038290.181818265,
            "unit": "ns/iter",
            "extra": "iterations: 44\ncpu: 14036079.363636369 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1007.3458664925573,
            "unit": "ns/iter",
            "extra": "iterations: 693999\ncpu: 1006.9906844246178 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 9903.106057174871,
            "unit": "ns/iter",
            "extra": "iterations: 70660\ncpu: 9900.537673365414 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 110907.158767777,
            "unit": "ns/iter",
            "extra": "iterations: 6330\ncpu: 110886.59225908374 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 340023.34415899223,
            "unit": "ns/iter",
            "extra": "iterations: 2063\ncpu: 339973.7707222491 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 307500.16192562203,
            "unit": "ns/iter",
            "extra": "iterations: 2285\ncpu: 307400.16061269125 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 254050.88220368113,
            "unit": "ns/iter",
            "extra": "iterations: 2759\ncpu: 254015.22943095298 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 55025427.80000112,
            "unit": "ns/iter",
            "extra": "iterations: 10\ncpu: 55016242.000000134 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 16528.101520687625,
            "unit": "ns/iter",
            "extra": "iterations: 42415\ncpu: 16526.499634563243 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 292239.7364723056,
            "unit": "ns/iter",
            "extra": "iterations: 2421\ncpu: 292067.751755465 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "87706a0418d7f6b0acd4acf6943f0c3d50e95f56",
          "message": "explicit rules (#487)",
          "timestamp": "2026-10-21T10:32:09+02:00",
          "tree_id": "38dee399713aa1eefd71e405db786af107c035ba",
          "url": "https://github.com/najaeda/naja/commit/87706a0418d7f6b0acd4acf6943f0c3d50e95f56"
        },
        "date": 1792577518830,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 212405.37444232716,
            "unit": "ns/iter",
            "extra": "iterations: 3138\ncpu: 212368.8285532186 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 312015.0615316455,
            "unit": "ns/iter",
            "extra": "iterations: 2259\ncpu: 311968.1359008412 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1776382.666666638,
            "unit": "ns/iter",
            "extra": "iterations: 405\ncpu: 1776154.9259259263 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17086768.72500021,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17085671.299999993 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1206.876529595505,
            "unit": "ns/iter",
            "extra": "iterations: 583079\ncpu: 1206.797223017807 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 11938.885580124328,
            "unit": "ns/iter",
            "extra": "iterations: 58565\ncpu: 11938.041680184408 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 170092.69399908395,
            "unit": "ns/iter",
            "extra": "iterations: 4366\ncpu: 170079.37379752632 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 494292.5075034274,
            "unit": "ns/iter",
            "extra": "iterations: 1466\ncpu: 494250.26330150035 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 448346.9859693852,
            "unit": "ns/iter",
            "extra": "iterations: 1568\ncpu: 448317.9674744891 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 362203.51370926603,
            "unit": "ns/iter",
            "extra": "iterations: 1933\ncpu: 362189.7066735645 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 67461366.55555624,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 67459485.88888891 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20768.163535781103,
            "unit": "ns/iter",
            "extra": "iterations: 33803\ncpu: 20767.121793923594 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 388825.1288173561,
            "unit": "ns/iter",
            "extra": "iterations: 1801\ncpu: 388594.1488062082 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "67f9a17b2f6c9d638458838bec1c6b62993d351b",
          "message": "Update NAJA_VERSION to 0.7.31 (#488)",
          "timestamp": "2026-10-21T10:33:38+02:00",
          "tree_id": "59d17344b80824144962ccc0c9c3c3e905e3967e",
          "url": "https://github.com/najaeda/naja/commit/67f9a17b2f6c9d638458838bec1c6b62993d351b"
        },
        "date": 1792578307820,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 215573.22573642508,
            "unit": "ns/iter",
            "extra": "iterations: 3225\ncpu: 215481.1553488372 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 308060.19667105604,
            "unit": "ns/iter",
            "extra": "iterations: 2283\ncpu: 308026.78843626805 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1752374.631067945,
            "unit": "ns/iter",
            "extra": "iterations: 412\ncpu: 1752200.0291262136 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18322908.87179438,
            "unit": "ns/iter",
            "extra": "iterations: 39\ncpu: 18320508.102564104 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1232.6072334412977,
            "unit": "ns/iter",
            "extra": "iterations: 569715\ncpu: 1232.5459150627942 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12214.109903339473,
            "unit": "ns/iter",
            "extra": "iterations: 57314\ncpu: 12213.680025822665 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 156249.2065837216,
            "unit": "ns/iter",
            "extra": "iterations: 3858\ncpu: 156246.1736651116 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 495111.9306930557,
            "unit": "ns/iter",
            "extra": "iterations: 1414\ncpu: 495087.64639321034 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 450721.7370109048,
            "unit": "ns/iter",
            "extra": "iterations: 1559\ncpu: 450673.38806927565 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 363821.34213360667,
            "unit": "ns/iter",
            "extra": "iterations: 1856\ncpu: 363776.8049568967 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 69325711.37500076,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 69317232.37499999 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20727.747034868098,
            "unit": "ns/iter",
            "extra": "iterations: 33641\ncpu: 20726.081656312246 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 385758.14427353523,
            "unit": "ns/iter",
            "extra": "iterations: 1816\ncpu: 385674.45154185046 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "59c2229d4249cb4d963886214ffa28fc63a720ad",
          "message": "Fix constant pattern (#484)\n\n* Fix constant pattern\n\n* coverage",
          "timestamp": "2026-10-21T16:40:01+02:00",
          "tree_id": "c4f6082d1f244567c663b773fa56cdae3c0ae5a5",
          "url": "https://github.com/najaeda/naja/commit/59c2229d4249cb4d963886214ffa28fc63a720ad"
        },
        "date": 1792593733526,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 129443.30908419569,
            "unit": "ns/iter",
            "extra": "iterations: 5416\ncpu: 129437.25720088626 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 199180.6379897792,
            "unit": "ns/iter",
            "extra": "iterations: 3522\ncpu: 198705.38614423622 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1206629.0683760482,
            "unit": "ns/iter",
            "extra": "iterations: 585\ncpu: 1206153.6957264957 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 14668272.62264169,
            "unit": "ns/iter",
            "extra": "iterations: 53\ncpu: 14666074.226415094 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 838.8883807496959,
            "unit": "ns/iter",
            "extra": "iterations: 853222\ncpu: 838.6277721390213 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 9143.922362812058,
            "unit": "ns/iter",
            "extra": "iterations: 73895\ncpu: 9140.900629271257 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 85087.59678988524,
            "unit": "ns/iter",
            "extra": "iterations: 8224\ncpu: 85064.04584143971 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 347148.46813389147,
            "unit": "ns/iter",
            "extra": "iterations: 2181\ncpu: 347034.6382393397 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 281126.46593406895,
            "unit": "ns/iter",
            "extra": "iterations: 2275\ncpu: 280994.4584615381 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 222250.97838486705,
            "unit": "ns/iter",
            "extra": "iterations: 3331\ncpu: 222169.74512158506 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 54637934.89999773,
            "unit": "ns/iter",
            "extra": "iterations: 10\ncpu: 54633111.49999992 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 17139.767908029946,
            "unit": "ns/iter",
            "extra": "iterations: 41057\ncpu: 17134.777358306743 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 202080.4854957913,
            "unit": "ns/iter",
            "extra": "iterations: 3413\ncpu: 202031.8344564859 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "685dfebf146a4c7bdd2f59275d642d08a71e02cd",
          "message": "attempt another source (#489)",
          "timestamp": "2026-10-21T22:09:00+02:00",
          "tree_id": "0602dcaab45b9ee8360aa52bbbde5a932b704062",
          "url": "https://github.com/najaeda/naja/commit/685dfebf146a4c7bdd2f59275d642d08a71e02cd"
        },
        "date": 1792613492137,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 214623.39069197408,
            "unit": "ns/iter",
            "extra": "iterations: 3266\ncpu: 214611.647581139 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 307937.09797889984,
            "unit": "ns/iter",
            "extra": "iterations: 2276\ncpu: 307921.1010544816 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1634699.8753056226,
            "unit": "ns/iter",
            "extra": "iterations: 409\ncpu: 1634617.0880195594 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17252940.14285745,
            "unit": "ns/iter",
            "extra": "iterations: 42\ncpu: 17252077.452380955 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1229.3795928112647,
            "unit": "ns/iter",
            "extra": "iterations: 569613\ncpu: 1229.3176244221956 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12198.826909956466,
            "unit": "ns/iter",
            "extra": "iterations: 57161\ncpu: 12198.079214849284 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 163355.1503931039,
            "unit": "ns/iter",
            "extra": "iterations: 3943\ncpu: 163342.79051483632 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 491175.7040673313,
            "unit": "ns/iter",
            "extra": "iterations: 1426\ncpu: 491143.682328191 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 444612.8946700427,
            "unit": "ns/iter",
            "extra": "iterations: 1576\ncpu: 444580.3426395942 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 359084.4008200906,
            "unit": "ns/iter",
            "extra": "iterations: 1951\ncpu: 359058.3997949772 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 68047053.88888818,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 67819308.22222215 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20688.25674553704,
            "unit": "ns/iter",
            "extra": "iterations: 33726\ncpu: 20686.588270177363 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 386276.11252805206,
            "unit": "ns/iter",
            "extra": "iterations: 1804\ncpu: 386031.5149667436 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "1597bc377cc8ade68d9b825d3ba7f693a4111b1c",
          "message": "PULP frontend fixes (#490)\n\n* PULP frontend fixes\n\n* coverage",
          "timestamp": "2026-10-22T19:05:06+02:00",
          "tree_id": "f83aa01dd939d8fcade96c822340f68c0448076c",
          "url": "https://github.com/najaeda/naja/commit/1597bc377cc8ade68d9b825d3ba7f693a4111b1c"
        },
        "date": 1792689985640,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 109996.41563653799,
            "unit": "ns/iter",
            "extra": "iterations: 6229\ncpu: 109987.28688393002 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 174793.50767018268,
            "unit": "ns/iter",
            "extra": "iterations: 4172\ncpu: 174728.83581016306 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1007224.0196905673,
            "unit": "ns/iter",
            "extra": "iterations: 711\ncpu: 1004915.0942334741 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 10216940.086956544,
            "unit": "ns/iter",
            "extra": "iterations: 69\ncpu: 10215873.20289855 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 613.0461373521297,
            "unit": "ns/iter",
            "extra": "iterations: 1150066\ncpu: 612.9970671248428 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 6986.743587658708,
            "unit": "ns/iter",
            "extra": "iterations: 110685\ncpu: 6984.95414916204 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 66270.88138355628,
            "unit": "ns/iter",
            "extra": "iterations: 11044\ncpu: 66252.50280695404 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 236528.3752953115,
            "unit": "ns/iter",
            "extra": "iterations: 2963\ncpu: 236424.3570705364 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 217136.77942998908,
            "unit": "ns/iter",
            "extra": "iterations: 3228\ncpu: 217050.40830235454 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 176347.95199595334,
            "unit": "ns/iter",
            "extra": "iterations: 3958\ncpu: 176268.98534613437 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 40558763.85714287,
            "unit": "ns/iter",
            "extra": "iterations: 14\ncpu: 40557618.14285714 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 12464.653248962793,
            "unit": "ns/iter",
            "extra": "iterations: 56418\ncpu: 12462.697773760136 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 221437.8565094429,
            "unit": "ns/iter",
            "extra": "iterations: 3157\ncpu: 221287.1960722023 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "noam.chn1@gmail.com",
            "name": "Noam Cohen",
            "username": "nanocoh"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "4e324572976662742e4faa034c92cd74bdbe86a3",
          "message": "functionallity for new naja primitives  (#493)\n\n* Liberty: model ff_bank multibit registers\n\nThe structural Liberty parse kept only `ff` and `latch` groups, so an\n`ff_bank (IQ, IQN, N)` cell loaded with no sequential model and its bundle\noutputs had no truth table. Formal checks then treated every multibit flop\n(for example the asap7 DFFHQNV2X/DFFHQNH2V2X trays created by multibit\nclustering) as opaque.\n\n- YosysLibertyParser: keep `ff_bank` and `latch_bank` groups, their\n  clocked_on/next_state/clear/preset children, and `function` under\n  `bundle` in structural mode.\n- SNLLibertyConstructor: classify `ff_bank` cells as sequential, apply a\n  bundle-level `function` to each member, and expand a bank into N states\n  where bit i reads bundle member i (`D` -> `D<i>`) and output member i maps\n  to state i. Banks that cannot be modeled log a warning and stay loadable.\n- SNLBooleanTree: optional name-to-term aliases so bundle names resolve per\n  bit inside expressions.\n- Test: ff_bank.lib fixture and SNLLibertyConstructorTest1.testFFBank.\n\n* Liberty: cover malformed ff_bank groups\n\nAdd a fixture with a bank that has too few arguments, a non-numeric width,\na zero width, and a next_state that does not resolve. Each cell must stay\nloadable with no sequential model attached.\n\n* Liberty: model latch groups as level-sensitive sequential models\n\nA `latch` group now builds a SequentialModel of kind Latch, with\n`enable` as the control expression and `data_in` as the next state,\nso latch cells carry a model instead of being skipped. Cells that mix\nflip-flop and latch state, or use `latch_bank`, stay loadable without\na model as before.\n\n* Truth tables: describe add/sub, multiply and shift primitives\n\nThe word-level arithmetic primitives had no truth table, so a consumer\nthat expands primitives through tables saw them as opaque cells. Like\nDIVMOD, each output bit now gets a generic table carrying only metadata:\nthe operand width, the output bit, and for shifts the count width and\ndirection. Dependencies list the first operand then the second, most\nsignificant bit first, as the primitive declares them.\n\ngetTruthTable(design, term) returns the table and getTruthTableCount\ncounts one per output bit; the whole-design table stays empty, as for\nDIVMOD. Validation rejects an arithmetic left shift, as the primitive\nfactory does.\n\n* Truth tables: describe read-only memories as ROM lookups\n\nA memory primitive without write ports is a constant table: its read data\nis a pure function of the read address and the INIT contents. Expose it as\na generic ROM truth table per read-data bit (address bits as dependencies,\nthe bit's column as metadata) through the instance-level getTruthTable,\nso consumers model it like a table select with constant rows instead of\ntreating the output as opaque. Contents with x/z digits or disabled INIT\nkeep no table.\n\n* Truth tables: cover ROM literal decoding, ordering and word table misuse\n\nExercise the INIT literal decoder on digit separators, hex letters, the\nprimitive's default contents, and the four malformed forms it rejects; a\ndepth the address bits cannot reach; ROM table ordering; and the word\narithmetic table errors for a non-word design, an input bit, and a Word\nfactory call with another generic type. Mark the two guards no netlist can\nreach, a bus wider than the metadata holds and ROM metadata on another type,\nas excluded from coverage.",
          "timestamp": "2026-10-23T18:23:22+02:00",
          "tree_id": "224a7138b5b67841a98ebe7d1776e3da34344c83",
          "url": "https://github.com/najaeda/naja/commit/4e324572976662742e4faa034c92cd74bdbe86a3"
        },
        "date": 1792774162986,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 114393.22647247219,
            "unit": "ns/iter",
            "extra": "iterations: 6248\ncpu: 114322.61379641485 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 165958.376157683,
            "unit": "ns/iter",
            "extra": "iterations: 4211\ncpu: 165890.75896461649 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 948058.7768240356,
            "unit": "ns/iter",
            "extra": "iterations: 699\ncpu: 948016.1201716735 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 10962028.630769266,
            "unit": "ns/iter",
            "extra": "iterations: 65\ncpu: 10959615.430769231 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 627.9691208682038,
            "unit": "ns/iter",
            "extra": "iterations: 1118069\ncpu: 627.9264204624225 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 6239.487965987472,
            "unit": "ns/iter",
            "extra": "iterations: 109606\ncpu: 6238.659726657307 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 68206.53978767886,
            "unit": "ns/iter",
            "extra": "iterations: 11021\ncpu: 68199.09790400148 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 249208.78534213195,
            "unit": "ns/iter",
            "extra": "iterations: 2879\ncpu: 249177.29454671752 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 224049.1953175186,
            "unit": "ns/iter",
            "extra": "iterations: 3118\ncpu: 224029.50096215535 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 182857.98061305258,
            "unit": "ns/iter",
            "extra": "iterations: 3817\ncpu: 182845.5189939744 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 41041446.53846202,
            "unit": "ns/iter",
            "extra": "iterations: 13\ncpu: 41030916.76923071 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 12645.310202624256,
            "unit": "ns/iter",
            "extra": "iterations: 56015\ncpu: 12644.414549674222 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 233558.2664424166,
            "unit": "ns/iter",
            "extra": "iterations: 2980\ncpu: 233421.8667785773 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "3906b3f9ed03cabbe4954cb1b717ec6db967c8a9",
          "message": "DB0 save and load in naja-if (#491)\n\n* DB0 save and load in naja-if\n\n* fix valgrind\n\n* fix testing\n\n* refactor verilog to gate_level_verilog\n\n* add determinism tests\n\n* missing path\n\n* install naja-schematic\n\n* coverage",
          "timestamp": "2026-10-23T21:52:11+02:00",
          "tree_id": "4df7ddd704dccedb389524b39303b474316c16de",
          "url": "https://github.com/najaeda/naja/commit/3906b3f9ed03cabbe4954cb1b717ec6db967c8a9"
        },
        "date": 1792787708696,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 149015.66450892086,
            "unit": "ns/iter",
            "extra": "iterations: 4480\ncpu: 149000.87924107147 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 219533.67250548425,
            "unit": "ns/iter",
            "extra": "iterations: 3197\ncpu: 219518.31154207073 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1318052.4222221908,
            "unit": "ns/iter",
            "extra": "iterations: 540\ncpu: 1317535.5444444448 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 14035849.918367863,
            "unit": "ns/iter",
            "extra": "iterations: 49\ncpu: 14034460.530612249 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 996.7777823340754,
            "unit": "ns/iter",
            "extra": "iterations: 707212\ncpu: 996.3523413064253 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 9734.007236833566,
            "unit": "ns/iter",
            "extra": "iterations: 72131\ncpu: 9732.291095368144 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 110796.18526610442,
            "unit": "ns/iter",
            "extra": "iterations: 6407\ncpu: 110782.7114093959 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 343523.62915854977,
            "unit": "ns/iter",
            "extra": "iterations: 2044\ncpu: 343472.2871819961 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 311526.0269179081,
            "unit": "ns/iter",
            "extra": "iterations: 2229\ncpu: 311449.4504261999 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 254885.55067199396,
            "unit": "ns/iter",
            "extra": "iterations: 2753\ncpu: 254851.46930621166 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 55847010.29999906,
            "unit": "ns/iter",
            "extra": "iterations: 10\ncpu: 55841269.20000009 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 16172.379844962106,
            "unit": "ns/iter",
            "extra": "iterations: 43215\ncpu: 16171.029526784681 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 480345.90834537934,
            "unit": "ns/iter",
            "extra": "iterations: 1462\ncpu: 479633.82900133095 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "3906b3f9ed03cabbe4954cb1b717ec6db967c8a9",
          "message": "DB0 save and load in naja-if (#491)\n\n* DB0 save and load in naja-if\n\n* fix valgrind\n\n* fix testing\n\n* refactor verilog to gate_level_verilog\n\n* add determinism tests\n\n* missing path\n\n* install naja-schematic\n\n* coverage",
          "timestamp": "2026-10-23T21:52:11+02:00",
          "tree_id": "4df7ddd704dccedb389524b39303b474316c16de",
          "url": "https://github.com/najaeda/naja/commit/3906b3f9ed03cabbe4954cb1b717ec6db967c8a9"
        },
        "date": 1792789322545,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 114245.0069076323,
            "unit": "ns/iter",
            "extra": "iterations: 6225\ncpu: 114215.18730923696 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 167463.2127098363,
            "unit": "ns/iter",
            "extra": "iterations: 4170\ncpu: 167457.24844124704 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1000442.6379065849,
            "unit": "ns/iter",
            "extra": "iterations: 707\ncpu: 1000347.7015558699 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 10079693.132352678,
            "unit": "ns/iter",
            "extra": "iterations: 68\ncpu: 10079115.970588231 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 628.3743088589044,
            "unit": "ns/iter",
            "extra": "iterations: 1116089\ncpu: 628.3294575970198 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 7068.471042578238,
            "unit": "ns/iter",
            "extra": "iterations: 107589\ncpu: 7067.725046240786 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 64720.22158837603,
            "unit": "ns/iter",
            "extra": "iterations: 11219\ncpu: 64716.38015865948 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 247723.04401846608,
            "unit": "ns/iter",
            "extra": "iterations: 2817\ncpu: 247682.05360312373 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 225020.08100288096,
            "unit": "ns/iter",
            "extra": "iterations: 3111\ncpu: 225013.59884281593 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 185506.4106190045,
            "unit": "ns/iter",
            "extra": "iterations: 3748\ncpu: 185488.27267876218 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 40281198.4285693,
            "unit": "ns/iter",
            "extra": "iterations: 14\ncpu: 40279874.35714283 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 12606.39534925892,
            "unit": "ns/iter",
            "extra": "iterations: 55131\ncpu: 12605.40265912101 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 392036.4622811324,
            "unit": "ns/iter",
            "extra": "iterations: 1763\ncpu: 391274.6874644912 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "f228560232ec9a229861b94f28a6edabc7b8f40c",
          "message": "add microwatt in regress (#492)\n\n* add mocrowatt in regress\n\n* install GHDL\n\n* fix flow\n\n* testing\n\n* Revert \"testing\"\n\nThis reverts commit f43678115fc6139b3d49e9158a162c087e2756a1.\n\n* parallel run\n\n* fix\n\n* set timeout",
          "timestamp": "2026-10-24T10:17:42+02:00",
          "tree_id": "7201f9cb890a6e908260e629fa8905ce02c98209",
          "url": "https://github.com/najaeda/naja/commit/f228560232ec9a229861b94f28a6edabc7b8f40c"
        },
        "date": 1792830272518,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 210751.69330911557,
            "unit": "ns/iter",
            "extra": "iterations: 3303\ncpu: 210706.89252194978 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 304336.87961756106,
            "unit": "ns/iter",
            "extra": "iterations: 2301\ncpu: 304250.68622338114 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1774024.3475000027,
            "unit": "ns/iter",
            "extra": "iterations: 400\ncpu: 1773793.8575 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17712560.28947364,
            "unit": "ns/iter",
            "extra": "iterations: 38\ncpu: 17710652.921052624 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1193.3484602040387,
            "unit": "ns/iter",
            "extra": "iterations: 591442\ncpu: 1193.2995390925907 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 11748.29995474266,
            "unit": "ns/iter",
            "extra": "iterations: 59659\ncpu: 11747.027003469717 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 169202.3953608244,
            "unit": "ns/iter",
            "extra": "iterations: 3880\ncpu: 169194.54896907226 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 493929.617857134,
            "unit": "ns/iter",
            "extra": "iterations: 1400\ncpu: 493864.6028571427 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 446454.42098529515,
            "unit": "ns/iter",
            "extra": "iterations: 1563\ncpu: 446443.35124760115 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 361967.55469156057,
            "unit": "ns/iter",
            "extra": "iterations: 1929\ncpu: 361918.2944530849 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 68400325.74999811,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 68392662.49999999 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20507.46488679214,
            "unit": "ns/iter",
            "extra": "iterations: 34759\ncpu: 20505.802871198808 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 648671.9759712416,
            "unit": "ns/iter",
            "extra": "iterations: 1082\ncpu: 648238.4269870056 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "f228560232ec9a229861b94f28a6edabc7b8f40c",
          "message": "add microwatt in regress (#492)\n\n* add mocrowatt in regress\n\n* install GHDL\n\n* fix flow\n\n* testing\n\n* Revert \"testing\"\n\nThis reverts commit f43678115fc6139b3d49e9158a162c087e2756a1.\n\n* parallel run\n\n* fix\n\n* set timeout",
          "timestamp": "2026-10-24T10:17:42+02:00",
          "tree_id": "7201f9cb890a6e908260e629fa8905ce02c98209",
          "url": "https://github.com/najaeda/naja/commit/f228560232ec9a229861b94f28a6edabc7b8f40c"
        },
        "date": 1792832304738,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 113000.94237233883,
            "unit": "ns/iter",
            "extra": "iterations: 6247\ncpu: 112945.41828077477 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 164915.11754927994,
            "unit": "ns/iter",
            "extra": "iterations: 4211\ncpu: 164852.74139159344 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 965375.642857161,
            "unit": "ns/iter",
            "extra": "iterations: 756\ncpu: 965080.824074074 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 9521274.779411847,
            "unit": "ns/iter",
            "extra": "iterations: 68\ncpu: 9520042.602941174 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 586.2248051332923,
            "unit": "ns/iter",
            "extra": "iterations: 1146553\ncpu: 586.2076441298393 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 7072.397936530956,
            "unit": "ns/iter",
            "extra": "iterations: 109912\ncpu: 7070.13414367858 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 60877.41040611219,
            "unit": "ns/iter",
            "extra": "iterations: 11647\ncpu: 60872.741993646436 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 219925.68863009862,
            "unit": "ns/iter",
            "extra": "iterations: 3263\ncpu: 219872.28991725395 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 201543.96975748634,
            "unit": "ns/iter",
            "extra": "iterations: 3505\ncpu: 201489.8185449358 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 160880.9975261155,
            "unit": "ns/iter",
            "extra": "iterations: 3638\ncpu: 160869.04288070337 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 40668372.35714209,
            "unit": "ns/iter",
            "extra": "iterations: 14\ncpu: 40666053.14285722 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 12629.983217862982,
            "unit": "ns/iter",
            "extra": "iterations: 55714\ncpu: 12629.211616469825 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 387617.5228899173,
            "unit": "ns/iter",
            "extra": "iterations: 1813\ncpu: 386962.6784335745 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "a8948aaeea900e3217b2d245773991e3f9db6fed",
          "message": "Update repository references after transfer to keplertech (#494)",
          "timestamp": "2026-10-24T11:21:26+02:00",
          "tree_id": "37f7ddc8f0806e8fa6d566f000b60ca8b1994399",
          "url": "https://github.com/keplertech/naja/commit/a8948aaeea900e3217b2d245773991e3f9db6fed"
        },
        "date": 1792837326402,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 107300.69202898182,
            "unit": "ns/iter",
            "extra": "iterations: 6624\ncpu: 107290.26736111111 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 180066.87897484642,
            "unit": "ns/iter",
            "extra": "iterations: 4214\ncpu: 180035.50308495495 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 950000.2396804198,
            "unit": "ns/iter",
            "extra": "iterations: 751\ncpu: 949832.3888149136 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 9898491.352112642,
            "unit": "ns/iter",
            "extra": "iterations: 71\ncpu: 9896520.985915497 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 596.9637873150906,
            "unit": "ns/iter",
            "extra": "iterations: 1189666\ncpu: 596.8769125115792 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 6632.687304372941,
            "unit": "ns/iter",
            "extra": "iterations: 110542\ncpu: 6632.5034285610855 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 58795.293755204264,
            "unit": "ns/iter",
            "extra": "iterations: 12010\ncpu: 58787.245545378864 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 235401.24233333397,
            "unit": "ns/iter",
            "extra": "iterations: 3000\ncpu: 235382.5330000001 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 214497.967268282,
            "unit": "ns/iter",
            "extra": "iterations: 3269\ncpu: 214467.41480575086 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 177989.15560112687,
            "unit": "ns/iter",
            "extra": "iterations: 3901\ncpu: 177963.46039477052 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 39583007.35714155,
            "unit": "ns/iter",
            "extra": "iterations: 14\ncpu: 39577882.785714276 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 11808.27516835425,
            "unit": "ns/iter",
            "extra": "iterations: 58953\ncpu: 11807.071141417742 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 366530.80609568075,
            "unit": "ns/iter",
            "extra": "iterations: 1903\ncpu: 365797.1650026152 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "391f06020d6cba53973bee50500c36851a135e28",
          "message": "constant port drivers in VHDL (#495)",
          "timestamp": "2026-10-24T23:53:57+02:00",
          "tree_id": "2fb81d10b5b7350be2fec5f7a4313e0863015cf3",
          "url": "https://github.com/keplertech/naja/commit/391f06020d6cba53973bee50500c36851a135e28"
        },
        "date": 1792879012859,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 210787.6087878787,
            "unit": "ns/iter",
            "extra": "iterations: 3300\ncpu: 210782.61060606065 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 304201.38178555126,
            "unit": "ns/iter",
            "extra": "iterations: 2229\ncpu: 304189.6774338269 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1757573.6821516403,
            "unit": "ns/iter",
            "extra": "iterations: 409\ncpu: 1757477.352078239 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17852430.605263166,
            "unit": "ns/iter",
            "extra": "iterations: 38\ncpu: 17850004.342105262 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1187.6702773007382,
            "unit": "ns/iter",
            "extra": "iterations: 590478\ncpu: 1187.5907146413583 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 11739.29364946403,
            "unit": "ns/iter",
            "extra": "iterations: 59680\ncpu: 11736.922754691699 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 181859.66735537117,
            "unit": "ns/iter",
            "extra": "iterations: 3872\ncpu: 181851.6601239668 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 499066.75230006914,
            "unit": "ns/iter",
            "extra": "iterations: 1413\ncpu: 498988.6164189669 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 448375.3525271934,
            "unit": "ns/iter",
            "extra": "iterations: 1563\ncpu: 448359.30198336573 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 362324.5383817316,
            "unit": "ns/iter",
            "extra": "iterations: 1928\ncpu: 362291.5046680492 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 68726281.49999827,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 68726140.2499999 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20248.234288848715,
            "unit": "ns/iter",
            "extra": "iterations: 34641\ncpu: 20245.189486446685 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 649708.5556602399,
            "unit": "ns/iter",
            "extra": "iterations: 1078\ncpu: 649188.721706881 ns\nthreads: 1"
          }
        ]
      }
    ]
  }
}