<!-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS> -->
<!-- SPDX-License-Identifier: Apache-2.0 -->

# SNL content hash, version 1

`NLContentHash.h` defines `ContentHash` (32 bytes), `toHex()` (64 lowercase
hexadecimal characters), and `kContentHashVersion = 1`. Hashes identify the
loaded SNL graph, including its IDs. They are independent of build identity,
frontend implementation, source locations, and serialization files. Equivalent
logic with different names, IDs or hierarchy is deliberately different. Loaders
must produce the same SNL content and IDs to produce the same hash.

## Primitive boundary

Primitive **definitions** are excluded, because tools can load them independently
of a NajaIF snapshot. DB0, libraries of type `InDB0` or `Primitives` (including
all their descendants), and primitive design definitions are omitted from
universe/library traversal. A user DB containing only excluded root libraries is
also omitted; a genuinely empty user DB is included. Standard libraries and
non-primitive designs, including blackboxes, remain content.

An instance still identifies its model by `(DB ID, library ID, design ID)`,
model name and type. For a primitive model its model digest is an **identity-only**
digest: no ports, nets, instances, defaults, attributes or modeling. Explicit
instance parameters, instance-pin identities, connectivity and pin attributes
remain instance content. Changing a primitive definition's truth table or timing
model does not affect a design hash. Changing which primitive an instance uses
does. Different library placement/IDs of a referenced primitive still change the
hash: the ID namespace must remain stable when reloading primitive libraries.
Comparing hashes does not establish equality of independently loaded cell models;
applications requiring that must identify their primitive library separately.

Directly requesting a primitive design or excluded library hash returns its
identity-only digest. Directly requesting one of its terms or nets hashes that
object normally; exclusion applies at the definition/traversal boundary.

## Fixed registry and exclusions

The read-only registry returned by `getContentHashAttributeNames()` is the
lexically ordered, case-sensitive list `dont_touch`, `keep`, `keep_hierarchy`.
Attribute duplicates are retained. The property registry returned by
`getContentHashPropertyNames()` is **empty in version 1**: primitive modeling is
excluded and no other dumpable property is currently registered. The encoding
below reserves a typed representation for future registered properties, but
changing either registry requires a version bump.

Unregistered attributes (including `KEEP`, `LOC`, `IOSTANDARD`), unregistered
dumpable properties, private properties/caches/frontend links, source references,
RTL information, revisions/edit counters, pointer values, name-interning IDs,
producer version/Git hash, paths, file layout and the dump manifest are excluded.
Attributes are accessed through `SNLAttributes` even though their storage is a
private property. This is the sole private-storage exception. No other private
property is inspected. PNL designs in included libraries are rejected explicitly;
this format describes SNL, not physical content.

## Frozen byte encoding

Concatenation below has no padding or delimiters other than those specified.
Every object digest is SHA-256 of `u8(object_tag) || u64(1) || payload`.

* `u8`: one byte. Booleans/presence markers: 0 or 1.
* `u64`: exactly eight bytes, little endian. All IDs, sizes, counts and bit
  indices use this representation. A negative bit index or bound is converted
  modulo 2^64 (sign-extended two's complement), and sorts by its signed value.
* `str`: `u64(number_of_bytes) || bytes`. Names/values use their stored UTF-8
  bytes without normalization, including empty names and embedded NUL bytes.
  The C++ API treats strings as bytes and does not validate UTF-8.
* `ref`: three `u64`s: DB ID, library ID, design ID.
* `seq(tag, records)`: `u8(tag) || u64(record_count) || concatenated_records`.
  Object-child records are exactly 32-byte digests. Other record layouts are
  specified explicitly below.
* `connection`: `u8(0)` if disconnected; otherwise
  `u8(1) || u64(net_id) || u8(is_bus_bit) || u64(bit_index)`.
  Scalar bit indices are zero. The shape marker distinguishes a scalar from
  bit zero of a bus.
* `metadata`: the attributes sequence followed by the properties sequence.

Object tags are fixed:

| Tag | Kind |
| --- | --- |
| 1 | Bit term |
| 2 | Term (scalar, bus or bundle) |
| 3 | Bit net |
| 4 | Net (scalar or bus) |
| 5 | Instance |
| 6 | Non-primitive design |
| 7 | Library |
| 8 | DB |
| 9 | Universe |
| 10 | Primitive model identity |

Sequence tags are fixed:

| Tag | Sequence |
| --- | --- |
| 32 | Attributes |
| 33 | Properties |
| 34 | Terms |
| 35 | Nets |
| 36 | Instances |
| 37 | Designs |
| 38 | Libraries |
| 39 | DBs |
| 40 | Connections |
| 41 | Parameters |
| 42 | Bits |
| 43 | Bundle members |

Object payloads, in exact field order:

* **Bit term**: `u64(id), u64(bit), u8(direction), metadata`.
* **Term**: `u64(id), str(name), u8(direction), u64(width), u8(shape)`;
  shape 0=scalar, 1=bus, 2=bundle. For bus only: `u64(msb), u64(lsb)`.
  Then `seq(42, bit-term digests)`. For bundle only:
  `seq(43, member-term digests)` in semantic member order. Then `metadata`.
* **Bit net**: `u64(id), u64(bit), u8(net_type), metadata`.
* **Net**: `u64(id), str(name), u64(width), u8(shape)` (0=scalar, 1=bus).
  For bus only: `u64(msb), u64(lsb)`. Then `seq(42, bit-net digests), metadata`.
* **Instance**: `u64(id), str(name), ref(model), digest(model), seq(41, parameters),
  seq(40, pin connections), metadata`. Instance parameter records:
  `str(name), str(value)`. Pin records:
  `u64(model_term_id), u64(bit), connection, metadata(inst_term)`.
* **Non-primitive design**: `ref, str(name), u8(design_type), seq(41, parameters),
  seq(34, term digests), seq(35, net digests), seq(36, instance digests),
  seq(40, port connections), metadata`.
  Design parameter records: `str(name), u8(parameter_type), u8(has_default)`
  followed by `str(value)` only if the default exists. Port records:
  `u64(term_id), u64(bit), connection`.
* **Primitive model identity**: `ref, str(name), u8(design_type)` only.
* **Library**: `u64(id), str(name), u8(library_type)`, then for included libraries
  only: `seq(37, included design digests), seq(38, included sublibrary digests),
  metadata`.
* **DB**: `u64(id), seq(38, included root-library digests), metadata`.
* **Universe**: `u8(has_top)` and if present `ref(top)`, then
  `seq(39, included user-DB digests), metadata`.

Top-level design terms use `getTerms()`; bundle-member structure additionally
appears in its bundle digest. Bit sequences and connection sequences sort by
`(term ID, signed bit index)` or `(net ID, signed bit index)`. Other object
sequences sort by numeric ID. Bundle member order is semantic and preserved.
Parameter records sort by name bytes, independently of interned-name order.

`metadata` encodes `seq(32, attribute records)` then `seq(33, property records)`.
Attribute records are `str(name), u8(value_type), str(value)` (0=number, 1=string).
Sort by name bytes, then value type, then value bytes. Numbers retain their stored
text; no numeric normalization occurs. Properties sort by name bytes; records
are `str(name), u64(value_count)`, followed by values in their original order.
Each value is `u8(0), str(value)` for strings or `u8(1), u64(value)` for integers.
All metadata sequences, including empty ones, are emitted.

Enum codes are frozen, regardless of future C++ enum rearrangements:

* Direction: Input=0, Output=1, InOut=2, Undefined=3.
* Net type: Standard=0, Assign0=1, Assign1=2, Supply0=3, Supply1=4,
  AssignX=5, AssignZ=6.
* Design type: Standard=0, UserBlackBox=1, AutoBlackBox=2, Primitive=3.
* Library type: Standard=0, InDB0=1, Primitives=2.
* Parameter type: Decimal=0, Binary=1, Boolean=2, String=3.

A scalar object is a term/net for its public `getContentHash()` method and a
bit-term/bit-net when embedded in its parent's bit sequence. A bus bit's public
method returns its bit digest. Thus changing a scalar's name changes its public
hash; bit digests intentionally do not carry the parent name. Connectivity is
owned by designs/instances, not by individual term/net digests.

## API and lifecycle

C++ `getContentHash() const` is available on `NLUniverse`, `NLDB`, `NLLibrary`,
`SNLDesign`, `SNLInstance`, `SNLTerm` and `SNLNet`, inherited by their bit classes.
`ContentHasher::hash(const T*)` supports these types with one shared design memo.
Pass live, non-null objects, do not mutate them during a computation, and discard
the hasher after editing. No hash is persisted in an SNL object. Cyclic design
hierarchy throws `NLException`. Shared models are hashed once per computation;
pointers are used solely as memo keys and never encoded or ordered.

Raw Python exposes the same methods as lowercase hex strings and
`naja.getContentHashVersion()`, `naja.getContentHashAttributeNames()`,
`naja.getContentHashPropertyNames()` (fresh list copies).
`netlist.get_content_hash()` hashes the current universe and raises
`RuntimeError` if it does not exist. `Instance.get_content_hash()` hashes that
instance's **model design**, including descendants; it does not hash its parent
placement or parent-side wiring. Use raw `SNLInstance.getContentHash()` for that.

## Verification and performance

`NLContentHashTest` checks mutation sensitivity, locality, explicit ID order,
attribute order/types/multiplicity, exclusions, buses, anonymous siblings,
cycles and a 27,000-instance / 108,000-pin timing case. Python checks the same
C++ golden plus raw/high-level parity and a structural Verilog/NajaIF round trip.
The cross-process determinism suite compares hashes alongside live state,
Verilog, DOT, manifests and payload bytes, including fresh-process reloads.

The golden universe contains DB 1, standard library 0 (`logic`), top design 0
(`top`), primitive library 1 and its primitive design 0 (`cell`, input `i`,
output `o`). Top input term 0 (`a`) and instance 0 (`u`) input `i` connect to
scalar net 0 (`wire`); output `o` is disconnected. Top has string attribute
`keep="yes"`; the primitive has a buffer truth table (excluded). Its version-1
hash, independently derived with Python `hashlib.sha256`, is:

```
464123ab0366d80817af5cec91e2d71b6b7bf4cd27a7cf44a9f716246c51df19
```

The timed test records `hash_seconds` as a gtest XML property. On the local macOS
arm64 machine, the optimized source-based Bazel build measured **0.058 seconds**
for 27,000 instances and 108,000 connected pins (plus the small golden fixture).
The existing CMake Debug build measured 0.87 seconds. Use an optimized build for
performance measurements; debug builds include unoptimized SHA-256 and container
code. Timing is informational rather than a CI wall-clock limit.

Any encoding or registry change requires deliberately updating the version,
this specification and the golden tests together. Existing version-1 hashes
must never silently acquire new semantics.
