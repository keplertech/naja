# SPDX-FileCopyrightText: 2024 The Naja authors
# <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import time
import logging
import warnings
import hashlib
import struct
import sys
import os
import tempfile
from enum import Enum
from typing import Union, List, Iterator, Optional
from dataclasses import dataclass

from najaeda import naja

logger = logging.getLogger(__name__)


def consistent_hash(obj):
    def default_serializer(o):
        if isinstance(o, (str, int, float, bool, type(None))):
            return o
        elif isinstance(o, (list, tuple)):
            return [default_serializer(i) for i in o]
        else:
            return str(o)

    def hash_value(value):
        if isinstance(value, int):
            return struct.pack("!q", value)
        else:
            raise TypeError(f"Unsupported type: {type(value)}")

    def hash_object(o):
        if isinstance(o, (list, tuple)):
            return b"".join(hash_object(i) for i in o)
        else:
            return hash_value(o)

    serialized_obj = default_serializer(obj)
    obj_bytes = hash_object(serialized_obj)
    return int(hashlib.sha256(obj_bytes).hexdigest(), 16)


def get_snl_instance_from_id_list(id_list: list) -> naja.SNLInstance:
    design = naja.NLUniverse.get().getTopDesign()
    return design.getInstanceByIDList(id_list)


def get_snl_path_from_id_list(id_list: list) -> naja.SNLPath:
    top = naja.NLUniverse.get().getTopDesign()
    design = top
    path = naja.SNLPath()
    for id in id_list:
        instance = design.getInstanceByID(id)
        assert instance is not None
        path = naja.SNLPath(path, instance)
        assert path.getTailInstance() is not None
        design = instance.getModel()
    if len(id_list) > 0:
        assert path.getTailInstance() is not None
    return path


class Attribute:
    def __init__(self, snlAttribute):
        self.snlAttribute = snlAttribute

    def __str__(self):
        return str(self.snlAttribute)

    def get_name(self):
        """
        :return: the name of the attribute.
        :rtype: str
        """
        return self.snlAttribute.getName()

    def has_value(self):
        """
        :return: True if the attribute has a value.
        :rtype: bool
        """
        return self.snlAttribute.hasValue()

    def get_value(self):
        """
        :return: the value of the attribute.
        :rtype: str
        """
        return self.snlAttribute.getValue()


@dataclass(frozen=True)
class SourceRange:
    file: str
    line: int
    end_line: int
    column: int
    end_column: int


def _source_range_from_snl_source_loc(source_loc):
    if source_loc is None:
        return None
    file, line, column, end_line, end_column = source_loc
    return SourceRange(file, line, end_line, column, end_column)


@dataclass(frozen=True)
class SourceReference:
    """A source range and optional producer, language and representation identifiers."""
    range: SourceRange
    provider: Optional[str] = None
    language: Optional[str] = None
    representation: Optional[str] = None


def _source_reference_from_snl(reference):
    if reference is None:
        return None
    source_range, provider, language, representation = reference
    return SourceReference(_source_range_from_snl_source_loc(source_range),
                           provider or None, language or None, representation or None)


class Equipotential:
    """Class that represents the term and wraps
    some of the snl occurrence API.
    """

    class Mode(Enum):
        """Traversal mode for equipotential construction."""
        STANDARD = naja.SNLEquipotential.Mode.Standard
        TRAVERSE_ASSIGNS = naja.SNLEquipotential.Mode.TraverseAssigns

    def __init__(self, term, mode: Mode = Mode.STANDARD):
        """Construct the equipotential containing ``term``.

        :param Term term: term from which to start the traversal.
        :param Equipotential.Mode mode: traversal mode.
        """
        if not isinstance(mode, Equipotential.Mode):
            raise TypeError("mode must be an Equipotential.Mode")
        path = get_snl_path_from_id_list(term.pathIDs)
        snl_term = get_snl_term_for_ids_with_path(path, term.termID, term.bit)
        if isinstance(snl_term, naja.SNLBusTerm):
            raise ValueError("Equipotential cannot be constructed on bus term")
        if len(term.pathIDs) == 0:
            self.equi = naja.SNLEquipotential(
                snl_term, mode=mode.value)
        else:
            ito = naja.SNLOccurrence(
                path.getHeadPath(), path.getTailInstance().getInstTerm(snl_term)
            )
            self.equi = naja.SNLEquipotential(
                ito, mode=mode.value)

    def __eq__(self, value):
        return self.equi == value.equi

    def dump_dot(self, path: str):
        """Dump the dot file of this equipotential."""
        self.equi.dumpDotFile(path)

    def get_inst_terms(self):
        """Iterate over the instance terminals of this equipotential.

        :return: an iterator over the instance terminals of this equipotential.
        :rtype: Iterator[Term]
        """
        if self.equi is not None:
            for term in self.equi.getInstTermOccurrences():
                path = term.getPath().getInstanceIDs()
                path.append(term.getInstTerm().getInstance().getID())
                yield Term(path,
                    term.getInstTerm().getBitTerm())

    def get_top_terms(self):
        """Iterate over the top terminals of this equipotential.

        :return: an iterator over the top terminals of this equipotential.
        :rtype: Iterator[Term]
        """
        if self.equi is not None:
            for term in self.equi.getTerms():
                yield Term([], term)

    def get_leaf_readers(self, filter=None):
        if self.equi is not None:
            for term in self.equi.getInstTermOccurrences():
                direction = term.getInstTerm().getDirection()
                if (
                    direction != naja.SNLTerm.Direction.Output and
                    term.getInstTerm().getInstance().getModel().isLeaf() and
                    (filter is None or filter(term))
                ):
                    path = term.getPath().getInstanceIDs()
                    path.append(term.getInstTerm().getInstance().getID())
                    yield Term(path, term.getInstTerm().getBitTerm())

    def get_leaf_drivers(self, filter=None):
        if self.equi is not None:
            for term in self.equi.getInstTermOccurrences():
                direction = term.getInstTerm().getDirection()
                if (
                    direction != naja.SNLTerm.Direction.Input and
                    term.getInstTerm().getInstance().getModel().isLeaf() and
                    (filter is None or filter(term))
                ):
                    path = term.getPath().getInstanceIDs()
                    path.append(term.getInstTerm().getInstance().getID())
                    yield Term(path, term.getInstTerm().getBitTerm())

    def get_top_readers(self):
        if self.equi is not None:
            for term in self.equi.getTerms():
                direction = term.getDirection()
                if direction != naja.SNLTerm.Direction.Input:
                    yield Term([], term)

    def get_top_drivers(self):
        if self.equi is not None:
            for term in self.equi.getTerms():
                direction = term.getDirection()
                if direction != naja.SNLTerm.Direction.Output:
                    yield Term([], term)

    def is_const(self) -> bool:
        """Check if this equipotential is a constant generator.

        :return: True if this equipotential is a constant generator.
        :rtype: bool
        """
        return self.is_const0() or self.is_const1() or self.is_constx() or self.is_constz()

    def is_const0(self) -> bool:
        """Check if this equipotential is a constant 0 generator.

        :return: True if this equipotential is a constant 0 generator.
        :rtype: bool
        """
        return self.equi.isConst0()

    def is_const1(self) -> bool:
        """Check if this equipotential is a constant 1 generator.

        :return: True if this equipotential is a constant 1 generator.
        :rtype: bool
        """
        return self.equi.isConst1()

    def is_constx(self) -> bool:
        """Check if this equipotential is a constant X generator."""
        return self.equi.isConstX()

    def is_constz(self) -> bool:
        """Check if this equipotential is a constant Z generator."""
        return self.equi.isConstZ()


class Net:
    class Type(Enum):
        STANDARD = naja.SNLNet.Type.Standard
        ASSIGN0 = naja.SNLNet.Type.Assign0
        ASSIGN1 = naja.SNLNet.Type.Assign1
        ASSIGNX = naja.SNLNet.Type.AssignX
        ASSIGNZ = naja.SNLNet.Type.AssignZ
        SUPPLY0 = naja.SNLNet.Type.Supply0
        SUPPLY1 = naja.SNLNet.Type.Supply1

    def __init__(self, path, net=None, net_concat=None):
        if net is not None and net_concat is not None:
            raise ValueError(
                "Only one of `net` or `net_concat` should be provided, not both."
            )
        if isinstance(path, naja.SNLPath):
            if path.size() > 0:
                self.pathIDs = path.getInstanceIDs()
            else:
                self.pathIDs = []
        elif isinstance(path, list):
            self.pathIDs = path.copy()
        if net is not None:
            self.net = net
        elif net_concat is not None:
            self.net_concat = net_concat

    def key(self):
        """Stable, hashable identity for Net."""
        if hasattr(self, "net"):
            return (tuple(self.pathIDs), ("net", self.net))
        else:
            return (tuple(self.pathIDs), ("concat", tuple(self.net_concat)))

    def __eq__(self, other):
        if not isinstance(other, Net):
            return NotImplemented
        return self.key() == other.key()

    def __hash__(self):
        return hash(self.key())

    def __str__(self):
        if self.is_concat():
            net_str = "{" + ",".join(map(str, self.net_concat)) + "}"
            return net_str
        net_str = str(self.net)
        path = get_snl_path_from_id_list(self.pathIDs)
        if path.size() > 0:
            return f"{path}/{net_str}"
        else:
            return net_str

    def delete(self):
        if hasattr(self, "net"):
            self.net.destroy()
        else:
            for net in self.net_concat:
                net.destroy()

    def get_name(self) -> str:
        """
        :return: the name of this Net.
        :rtype: str
        """
        if hasattr(self, "net"):
            return self.net.getName()
        return "{" + ",".join(map(str, self.net_concat)) + "}"

    def set_name(self, name: str):
        """
        :param str name: the name to set for this Net.
        """
        if self.is_concat():
            raise ValueError(
                f"set_name of {self}: cannot set name for a concatenated net. "
                "Use the individual nets instead."
            )
        elif self.is_bus_bit():
            raise ValueError(
                f"set_name of {self}: cannot set name for a bus bit. "
                "Use the bus net instead."
            )
        else:
            # We need to uniquify until parent instance if parent instance is not top.
            path = get_snl_path_from_id_list(self.pathIDs)
            if path.size():
                naja.SNLUniquifier(path.getHeadPath())
            self.net.setName(name)

    def get_msb(self) -> int:
        """
        :return: the most significant bit of the net if it is a bus.
        :rtype: int
        """
        if hasattr(self, "net") and isinstance(self.net, naja.SNLBusNet):
            return self.net.getMSB()
        return None

    def get_lsb(self) -> int:
        """
        :return: the least significant bit of the net if it is a bus.
        :rtype: int
        """
        if hasattr(self, "net") and isinstance(self.net, naja.SNLBusNet):
            return self.net.getLSB()
        return None

    def set_msb(self, msb: int):
        """
        :param int msb: the msb to set for this Net.
        """
        if self.is_concat():
            raise ValueError(
                f"set_msb of {self}: cannot set msb for a concatenated net. "
                "Use the individual nets instead."
            )
        if self.is_bus_bit():
            raise ValueError(
                f"set_msb of {self}: cannot set msb for a bus bit. "
                "Use the bus net instead."
            )
        if not self.is_bus():
            raise ValueError(f"set_msb of {self}: not a bus net")
        path = get_snl_path_from_id_list(self.pathIDs)
        if path.size():
            naja.SNLUniquifier(path.getHeadPath())
        self.net.setMSB(msb)

    def set_lsb(self, lsb: int):
        """
        :param int lsb: the lsb to set for this Net.
        """
        if self.is_concat():
            raise ValueError(
                f"set_lsb of {self}: cannot set lsb for a concatenated net. "
                "Use the individual nets instead."
            )
        if self.is_bus_bit():
            raise ValueError(
                f"set_lsb of {self}: cannot set lsb for a bus bit. "
                "Use the bus net instead."
            )
        if not self.is_bus():
            raise ValueError(f"set_lsb of {self}: not a bus net")
        path = get_snl_path_from_id_list(self.pathIDs)
        if path.size():
            naja.SNLUniquifier(path.getHeadPath())
        self.net.setLSB(lsb)

    def is_bus(self) -> bool:
        """
        :return: True if the net is a bus.
        :rtype: bool
        """
        return hasattr(self, "net") and isinstance(self.net, naja.SNLBusNet)

    def is_bus_bit(self) -> bool:
        """
        :return: True if the net is a bit of a bus.
        :rtype: bool
        """
        return hasattr(self, "net") and isinstance(self.net, naja.SNLBusNetBit)

    def is_scalar(self) -> bool:
        """
        :return: True if the net is a scalar.
        :rtype: bool
        """
        return hasattr(self, "net") and isinstance(self.net, naja.SNLScalarNet)

    def is_bit(self) -> bool:
        """
        :return: True if the net is a bit.
        :rtype: bool
        """
        return self.is_scalar() or self.is_bus_bit()

    def is_concat(self) -> bool:
        """
        :return: True if the net is a concatenation.
        :rtype: bool
        """
        return hasattr(self, "net_concat")

    def is_const(self) -> bool:
        """
        :return: True if the net is a constant generator.
        :rtype: bool
        """
        if hasattr(self, "net"):
            return self.net.isConstant()
        else:
            return all(net.isConstant() for net in self.net_concat)

    def is_const0(self) -> bool:
        """
        :return: True if the net is a constant 0 generator.
        :rtype: bool
        """
        if hasattr(self, "net"):
            return self.net.isConstant0()
        else:
            return all(net.isConstant0() for net in self.net_concat)

    def is_const1(self) -> bool:
        """
        :return: True if the net is a constant 1 generator.
        :rtype: bool
        """
        if hasattr(self, "net"):
            return self.net.isConstant1()
        else:
            return all(net.isConstant1() for net in self.net_concat)

    def is_constx(self) -> bool:
        """:return: True if the net is a constant X generator."""
        if hasattr(self, "net"):
            return self.net.isConstantX()
        return all(net.isConstantX() for net in self.net_concat)

    def is_constz(self) -> bool:
        """:return: True if the net is a constant Z generator."""
        if hasattr(self, "net"):
            return self.net.isConstantZ()
        return all(net.isConstantZ() for net in self.net_concat)

    def set_type(self, net_type: Type):
        """
        :param Type net_type: the type of the net.
        """
        if hasattr(self, "net"):
            self.net.setType(net_type.value)
        else:
            for net in self.net_concat:
                net.setType(net_type.value)

    def get_width(self) -> int:
        """
        :return: the width of the net.
        :rtype: int
        """
        if hasattr(self, "net"):
            return self.net.getWidth()
        return sum(1 for _ in self.net_concat)

    def get_bits(self):
        """Iterate over the bits of this Net.
        The iterator will return itself if the Net is scalar.
        :return: an iterator over the bits of this Net.
        :rtype: Iterator[Net]
        """
        if hasattr(self, "net"):
            if isinstance(self.net, naja.SNLBusNet):
                for bit in self.net.getBits():
                    yield Net(self.pathIDs, bit)
            else:
                yield self
        else:
            for net in self.net_concat:
                yield Net(self.pathIDs, net)

    def get_bit(self, index: int):
        """
        :param int index: the index of the bit to get.
        :return: the Net bit at the given index or None if it does not exist.
        :rtype: Net
        """
        if hasattr(self, "net"):
            if isinstance(self.net, naja.SNLBusNet):
                return Net(self.pathIDs, self.net.getBit(index))
            else:
                return None
        if 0 <= index < len(self.net_concat):
            return Net(self.pathIDs, self.net_concat[index])
        return None

    def get_inst_terms(self):
        """
        :return: an iterator over the instance terminals of the net.
        :rtype: Iterator[Term]
        """
        if hasattr(self, "net_concat"):
            raise ValueError("Cannot get inst terms from a net_concat")
        path = self.pathIDs.copy()
        for term in self.net.getInstTerms():
            path.append(term.getInstance().getID())
            yield Term(path, term.getBitTerm())
            path.pop()

    def count_inst_terms(self) -> int:
        """
        Count the instance terminals of this net.

        :return: the number of instance terminals of this net.
        :rtype: int
        """
        return sum(1 for _ in self.get_inst_terms())

    def get_design_terms(self):
        """Return an iterator over the design terminals of the net.
        This includes only the terminals that are part of the current design.
        The iterator will yield Term objects bit per bit.

        :return: an iterator over the design terminals of the net.
        :rtype: Iterator[Term]
        """
        if hasattr(self, "net_concat"):
            raise ValueError("Cannot get terms from a net_concat")
        for term in self.net.getBitTerms():
            yield Term(self.pathIDs, term)

    def count_design_terms(self) -> int:
        """Count the design terminals of this net.

        :return: the number of design terminals of this net.
        :rtype: int
        """
        return sum(1 for _ in self.get_design_terms())

    def get_terms(self):
        """Return an iterator over the terminals of the net.
        This includes both design and instance terminals.

        :return: an iterator over the terminals of the net.
        :rtype: Iterator[Term]
        """
        yield from self.get_design_terms()
        yield from self.get_inst_terms()

    def get_attributes(self) -> Iterator[Attribute]:
        """Iterate over the attributes of this Net.

        :return: the attributes of this Net.
        :rtype: Iterator[Attribute]
        """
        if hasattr(self, "net"):
            snlnet = self.net
            for attribute in snlnet.getAttributes():
                yield Attribute(attribute)

    def has_source_references(self) -> bool:
        """Return whether this object has a source declaration or any source origins."""
        obj = self.net if hasattr(self, "net") else None
        return bool(obj and obj.hasSourceReferences())

    def get_source_declaration(self) -> Optional[SourceReference]:
        """Return this object's declaration in the loaded input, if available."""
        obj = self.net if hasattr(self, "net") else None
        return _source_reference_from_snl(obj.getSourceDeclaration()) if obj else None

    def get_source_origins(self) -> list[SourceReference]:
        """Return upstream source references in first-insertion order."""
        obj = self.net if hasattr(self, "net") else None
        return [_source_reference_from_snl(ref) for ref in obj.getSourceOrigins()] if obj else []

    def get_source_range(self) -> Optional[SourceRange]:
        """Return the source range of this Net, if available."""
        if hasattr(self, "net"):
            return _source_range_from_snl_source_loc(self.net.getSourceLoc())
        return None

    def count_attributes(self) -> int:
        """Count the attributes of this Net.

        :return: the number of attributes of this Net.
        :rtype: int
        """
        return sum(1 for _ in self.get_attributes())


def get_snl_term_for_ids_with_path(path, termID, bit):
    model = None
    if path.size() == 0:
        model = naja.NLUniverse.get().getTopDesign()
    else:
        model = path.getTailInstance().getModel()
    snl_term = model.getTermByID(termID)
    if bit is None:
        return snl_term
    else:
        return snl_term.getBusTermBit(bit)


class Term:
    class Direction(Enum):
        """Enum for the direction of a term."""
        INPUT = naja.SNLTerm.Direction.Input
        OUTPUT = naja.SNLTerm.Direction.Output
        INOUT = naja.SNLTerm.Direction.InOut

    def __init__(self, path, term):
        self.pathIDs = list(path)
        self.termID = term.getID()
        self.bit: int | None = (
            term.getBit() if isinstance(term, naja.SNLBusTermBit) else None
        )

    def key(self) -> tuple:
        """
        Unique, hashable identity of this Term.
        Stable within a design revision.
        """
        return (
            tuple(self.pathIDs),
            self.termID,
            self.bit,
        )

    def __eq__(self, other):
        if not isinstance(other, Term):
            return NotImplemented
        return self.key() == other.key()

    def __ne__(self, other):
        eq = self.__eq__(other)
        if eq is NotImplemented:
            return NotImplemented
        return not eq

    def __lt__(self, other):
        if not isinstance(other, Term):
            return NotImplemented
        return self.key() < other.key()

    def __le__(self, other):
        if not isinstance(other, Term):
            return NotImplemented
        eq = self.__eq__(other)
        if eq is NotImplemented:
            return NotImplemented
        return eq or self.__lt__(other)

    def __gt__(self, other):
        if not isinstance(other, Term):
            return NotImplemented
        le = self.__le__(other)
        if le is NotImplemented:
            return NotImplemented
        return not le

    def __ge__(self, other):
        if not isinstance(other, Term):
            return NotImplemented
        lt = self.__lt__(other)
        if lt is NotImplemented:
            return NotImplemented
        return not lt

    def __hash__(self):
        termIDs = []
        if self.bit is not None:
            termIDs = [self.termID, self.bit]
        else:
            termIDs = [self.termID, -1]
        return consistent_hash((self.pathIDs, termIDs))

    def __str__(self):
        term_str = ""
        path = get_snl_path_from_id_list(self.pathIDs)
        snl_term = self.get_snl_term()
        snl_term_str = snl_term.getName()
        if snl_term.isUnnamed():
            snl_term_str = "<unnamed>"
        if path.size() == 0:
            term_str = snl_term_str
        else:
            term_str = f"{path}/{snl_term_str}"
        if self.is_bus():
            term_str += f"[{self.get_msb()}:{self.get_lsb()}]"
        bit = self.get_bit_number()
        if bit is not None:
            term_str += f"[{bit}]"
        return term_str

    def __repr__(self) -> str:
        path = get_snl_path_from_id_list(self.pathIDs)
        return f"Term({path}, {self.get_snl_term().getName()})"

    def __make_unique(self):
        path = get_snl_path_from_id_list(self.pathIDs)
        if path.size() > 1:
            path = path.getHeadPath()
            naja.SNLUniquifier(path)

    def get_snl_term(self):
        path = get_snl_path_from_id_list(self.pathIDs)
        model = None
        if len(self.pathIDs) == 0:
            model = naja.NLUniverse.get().getTopDesign()
        else:
            model = path.getTailInstance().getModel()
        snlterm = model.getTermByID(self.termID)
        if self.bit is None:
            return snlterm
        else:
            return snlterm.getBusTermBit(self.bit)

    def __get_role_term(self):
        term = self.get_snl_term()
        if not isinstance(term, naja.SNLBitTerm):
            raise ValueError("Term roles require a scalar term or a bus bit")
        path = get_snl_path_from_id_list(self.pathIDs)
        return path.getTailInstance().getInstTerm(term) if path.size() else term

    def get_role(self):
        """Return the term role, resolved using this instance's parameters."""
        return self.__get_role_term().getRole()

    def get_reset_active_level(self):
        """Return the resolved reset/set active level, or SNLActiveLevel.NA."""
        return self.__get_role_term().getResetActiveLevel()

    def is_clock(self) -> bool:
        """Return whether this bit term is a clock in its instance context."""
        return self.__get_role_term().isClock()

    def is_async_reset(self) -> bool:
        """Return whether this bit term is an asynchronous reset."""
        return self.__get_role_term().isAsyncReset()

    def is_async_set(self) -> bool:
        """Return whether this bit term is an asynchronous set."""
        return self.__get_role_term().isAsyncSet()

    def is_sync_reset(self) -> bool:
        """Return whether this bit term is a synchronous reset."""
        return self.__get_role_term().isSyncReset()

    def is_sync_set(self) -> bool:
        """Return whether this bit term is a synchronous set."""
        return self.__get_role_term().isSyncSet()

    def is_reset(self) -> bool:
        """Return whether this bit term is an asynchronous or synchronous reset."""
        return self.__get_role_term().isReset()

    def is_enable(self) -> bool:
        """Return whether this bit term is an enable."""
        return self.__get_role_term().isEnable()

    def is_data_input(self) -> bool:
        """Return whether this bit term carries input data."""
        return self.__get_role_term().isDataInput()

    def is_data_output(self) -> bool:
        """Return whether this bit term carries output data."""
        return self.__get_role_term().isDataOutput()

    def is_data(self) -> bool:
        """Return whether this bit term carries input or output data."""
        return self.__get_role_term().isData()

    def is_bus(self) -> bool:
        """
        :return: True if the term is a bus.
        :rtype: bool
        """
        return isinstance(self.get_snl_term(), naja.SNLBusTerm)

    def is_bus_bit(self) -> bool:
        """
        :return: True if the term is a bit of a bus.
        :rtype: bool
        """
        return isinstance(self.get_snl_term(), naja.SNLBusTermBit)

    def is_scalar(self) -> bool:
        """
        :return: True if the term is a scalar.
        :rtype: bool
        """
        return isinstance(self.get_snl_term(), naja.SNLScalarTerm)

    def is_bit(self) -> bool:
        """
        :return: True if the term is a bit.
        :rtype: bool
        """
        return self.is_scalar() or self.is_bus_bit()

    def is_sequential(self) -> bool:
        """
        :return: True if the term is a sequential term.
        :rtype: bool
        """
        for term in self.get_instance().get_bit_terms():
            clockRelatedInputs = term.get_clock_related_inputs()
            if self in clockRelatedInputs:
                return True
            clockRelatedOutputs = term.get_clock_related_outputs()
            if self in clockRelatedOutputs:
                return True
            if (len(clockRelatedInputs) > 0 or len(clockRelatedOutputs) > 0) and (term == self):
                return True
        return False

    def get_bit_number(self):
        """
        :return: the bit index of the term if it is a bit.
        :rtype: int or None
        """
        snl_term = self.get_snl_term()
        if isinstance(snl_term, naja.SNLBusTermBit):
            return snl_term.getBit()
        return None

    def get_msb(self) -> int:
        """
        :return: the most significant bit of the term if it is a bus.
        :rtype: int or None
        """
        snl_term = self.get_snl_term()
        if isinstance(snl_term, naja.SNLBusTerm):
            return snl_term.getMSB()
        return None

    def get_lsb(self) -> int:
        """
        :return: the least significant bit of the term if it is a bus.
        :rtype: int or None
        """
        snl_term = self.get_snl_term()
        if isinstance(snl_term, naja.SNLBusTerm):
            return snl_term.getLSB()
        return None

    def set_msb(self, msb: int):
        """
        :param int msb: the msb to set for this Term.
        """
        snl_term = self.get_snl_term()
        if isinstance(snl_term, naja.SNLBusTermBit):
            raise ValueError("set_msb: cannot set msb on a bus bit")
        if not isinstance(snl_term, naja.SNLBusTerm):
            raise ValueError("set_msb: not a bus term")
        self.__make_unique()
        snl_term.setMSB(msb)

    def set_lsb(self, lsb: int):
        """
        :param int lsb: the lsb to set for this Term.
        """
        snl_term = self.get_snl_term()
        if isinstance(snl_term, naja.SNLBusTermBit):
            raise ValueError("set_lsb: cannot set lsb on a bus bit")
        if not isinstance(snl_term, naja.SNLBusTerm):
            raise ValueError("set_lsb: not a bus term")
        self.__make_unique()
        snl_term.setLSB(lsb)

    def get_width(self) -> int:
        """
        :return: the width of the term. 1 if scalar.
        :rtype: int
        """
        return self.get_snl_term().getWidth()

    def get_name(self) -> str:
        """
        :return: the name of the term.
        :rtype: str
        """
        return self.get_snl_term().getName()

    def is_unnamed(self) -> bool:
        """
        :return: True if the term is unnamed.
        :rtype: bool
        """
        return self.get_snl_term().isUnnamed()

    def get_direction(self) -> Direction:
        """
        :return: the direction of the term.
        :rtype: Term.Direction
        """
        snlterm = self.get_snl_term()
        if snlterm.getDirection() == naja.SNLTerm.Direction.Input:
            return Term.Direction.INPUT
        elif snlterm.getDirection() == naja.SNLTerm.Direction.Output:
            return Term.Direction.OUTPUT
        elif snlterm.getDirection() == naja.SNLTerm.Direction.InOut:
            return Term.Direction.INOUT

    def get_attributes(self) -> Iterator[Attribute]:
        """Iterate over the attributes of this Term.

        :return: the attributes of this Term.
        :rtype: Iterator[Attribute]
        """
        snlterm = self.get_snl_term()
        for attribute in snlterm.getAttributes():
            yield Attribute(attribute)

    def has_source_references(self) -> bool:
        """Return whether this object has a source declaration or any source origins."""
        obj = self.get_snl_term()
        return bool(obj and obj.hasSourceReferences())

    def get_source_declaration(self) -> Optional[SourceReference]:
        """Return this object's declaration in the loaded input, if available."""
        obj = self.get_snl_term()
        return _source_reference_from_snl(obj.getSourceDeclaration()) if obj else None

    def get_source_origins(self) -> list[SourceReference]:
        """Return upstream source references in first-insertion order."""
        obj = self.get_snl_term()
        return [_source_reference_from_snl(ref) for ref in obj.getSourceOrigins()] if obj else []

    def get_source_range(self) -> Optional[SourceRange]:
        """Return the source range of this Term, if available."""
        return _source_range_from_snl_source_loc(self.get_snl_term().getSourceLoc())

    def count_attributes(self) -> int:
        """Count the attributes of this Term.

        :return: the number of attributes of this Term.
        :rtype: int
        """
        return sum(1 for _ in self.get_attributes())

    def delete(self):
        """Delete this Term."""
        self.__make_unique()
        snlterm = self.get_snl_term()
        snlterm.destroy()

    def get_combinatorial_inputs(self):
        """Get all combinatorial input terms of this instance.

        :return: a list of combinatorial input terms.
        :rtype: List[Term]
        """
        terms = self.__get_snl_model().getCombinatorialInputs(self.get_snl_term())
        # Convert SNL terms to Term objects
        return [Term(self.pathIDs, term) for term in terms]

    def get_combinatorial_outputs(self):
        """Get all combinatorial output terms of this instance.

        :return: a list of combinatorial output terms.
        :rtype: List[Term]
        """
        terms = self.__get_snl_model().getCombinatorialOutputs(self.get_snl_term())
        # Convert SNL terms to Term objects
        return [Term(self.pathIDs, term) for term in terms]

    def get_clock_related_inputs(self):
        """Get all input terms that are related to the given clock term.

        :param clock_term: the clock term to check for related inputs.
        :return: a list of input terms that are related to the clock term.
        :rtype: List[Term]
        """
        terms = self.__get_snl_model().getClockRelatedInputs(self.get_snl_term())
        # Convert SNL terms to Term objects
        return [Term(self.pathIDs, term) for term in terms]

    def get_clock_related_outputs(self):
        """Get all output terms that are related to the given clock term.

        :param clock_term: the clock term to check for related outputs.
        :return: a list of output terms that are related to the clock term.
        :rtype: List[Term]
        """
        terms = self.__get_snl_model().getClockRelatedOutputs(self.get_snl_term())
        # Convert SNL terms to Term objects
        return [Term(self.pathIDs, term) for term in terms]

    def __get_snl_model(self):
        snlterm = self.get_snl_term()
        return snlterm.getDesign()

    def __get_snl_bitnet(self, bit) -> Net:
        # single bit
        path = get_snl_path_from_id_list(self.pathIDs)
        if path.size():
            instTerm = path.getTailInstance().getInstTerm(bit)
            return instTerm.getNet()
        else:
            return bit.getNet()

    def __get_snl_lower_bitnet(self, bit) -> Net:
        return bit.getNet()

    def __get_snl_busnet(self, snl_nets) -> naja.SNLBusNet:
        # iterate on all elements of the list and check if
        # a full SNLBusNet can be reconstructed
        snl_bus_net = None
        for i in range(len(snl_nets)):
            snl_net = snl_nets[i]
            if not isinstance(snl_net, naja.SNLBusNetBit):
                return None
            bit_bus = snl_net.getBus()
            if bit_bus.getWidth() != len(snl_nets):
                return None
            if snl_bus_net is None:
                snl_bus_net = bit_bus
            if snl_bus_net != bit_bus:
                return None
            if snl_bus_net.getBitAtPosition(i) != snl_net:
                return None
        return snl_bus_net

    def __get_net(self, path, snl_term_net_accessor) -> Net:
        if isinstance(self.get_snl_term(), naja.SNLBusTerm):
            snl_nets = []
            for bit in self.get_snl_term().getBits():
                snl_net = snl_term_net_accessor(bit)
                snl_nets.append(snl_net)
            snl_bus_net = self.__get_snl_busnet(snl_nets)
            if snl_bus_net is not None:
                return Net(path, snl_bus_net)
            else:
                if all(element is not None for element in snl_nets):
                    return Net(path, net_concat=snl_nets)
        else:
            snl_net = snl_term_net_accessor(self.get_snl_term())
            if snl_net is not None:
                return Net(path, snl_net)
        return None

    def get_lower_net(self) -> Net:
        """
        :return: the lower net of the term.
        :rtype: Net
        """
        return self.__get_net(self.pathIDs, self.__get_snl_lower_bitnet)

    def get_upper_net(self) -> Net:
        """
        :return: the upper net of the term.
        :rtype: Net
        :remark: If the term is a top level term, it will return None.
        """
        head_path = self.pathIDs.copy()
        if len(head_path) == 0:
            return None
        # path is one level up
        head_path.pop()
        return self.__get_net(head_path, self.__get_snl_bitnet)

    def get_instance(self):
        """
        :return: the instance of this Term.
        :rtype: Instance
        """
        return Instance(self.pathIDs)

    def get_flat_fanout(self, filter=None):
        return self.get_equipotential().get_leaf_readers(filter=filter)

    def count_flat_fanout(self, filter=None):
        return sum(1 for _ in self.get_flat_fanout(filter=filter))

    def get_equipotential(
        self,
        mode: Equipotential.Mode = Equipotential.Mode.STANDARD
    ) -> Equipotential:
        """
        :param Equipotential.Mode mode: traversal mode.
        :return: the Equipotential of this Term.
        :rtype: Equipotential
        """
        return Equipotential(self, mode=mode)

    def is_input(self) -> bool:
        """
        :return: True if the term is an input.
        :rtype: bool
        """
        snlterm = self.get_snl_term()
        return snlterm.getDirection() == naja.SNLTerm.Direction.Input

    def is_output(self) -> bool:
        """
        :return: True if the term is an output.
        :rtype: bool
        """
        snlterm = self.get_snl_term()
        return snlterm.getDirection() == naja.SNLTerm.Direction.Output

    def get_bits(self):
        """
        :return: an iterator over the bits of the term.
            If the term is scalar, it will return an iterator over itself.
        :rtype: Iterator[Term]
        """
        snl_term = self.get_snl_term()
        if isinstance(snl_term, naja.SNLBusTerm):
            for bit in snl_term.getBits():
                yield Term(self.pathIDs, bit)
        else:
            yield self

    def get_bit(self, index: int):
        """
        :param int index: the index of the bit to get.
        :return: the Term bit at the given index or None if it does not exist.
        :rtype: Term or None
        """
        snl_term = self.get_snl_term()
        if isinstance(snl_term, naja.SNLBusTerm):
            return Term(
                self.pathIDs,
                snl_term.getBusTermBit(index),
            )
        return None

    def disconnect_upper_net(self):
        """Disconnect this term from its upper net."""
        if self.get_instance().is_top():
            raise ValueError("Cannot disconnect the upper net of a top level term")
        path = get_snl_path_from_id_list(self.pathIDs)
        self.__make_unique()
        inst = path.getTailInstance()
        for bit in self.get_snl_term().getBits():
            iterm = inst.getInstTerm(bit)
            iterm.setNet(None)

    def disconnect_lower_net(self):
        """Disconnect this term from its lower net."""
        self.__make_unique()
        for bit in self.get_snl_term().getBits():
            bit.setNet(None)

    def connect_upper_net(self, net: Net):
        """Connect this term to the given upper Net.

        :param Net net: the upper Net to connect to.
        """
        if self.get_instance().is_top():
            raise ValueError("Cannot connect the upper net of a top level term")
        if self.get_width() != net.get_width():
            raise ValueError("Width mismatch")
        self.__make_unique()
        path = get_snl_path_from_id_list(self.pathIDs)
        inst = path.getTailInstance()
        for bterm, bnet in zip(
            self.get_snl_term().getBits(),
            net.net.getBits(),
        ):
            iterm = inst.getInstTerm(bterm)
            iterm.setNet(bnet)

    def connect_lower_net(self, net: Net):
        """Connect this term to the given lower Net.

        :param Net net: the lower Net to connect to.
        """
        if self.get_width() != net.get_width():
            raise ValueError("Width mismatch")
        self.__make_unique()
        for bterm, bnet in zip(
            self.get_snl_term().getBits(),
            net.net.getBits(),
        ):
            bterm.setNet(bnet)

    def get_truth_table(self):
        # check the index of the output
        if not self.is_output():
            raise ValueError("Truth table can only be computed for output terms")
        # get the instance
        if self.is_bus():
            raise ValueError("Truth table cannot be computed for bus terms")
        inst = self.get_instance()
        # get the model
        model = inst._Instance__get_snl_model()
        for term in model.getTerms():
            if term.getDirection() == naja.SNLTerm.Direction.Output:
                if term.getName() == self.get_name():
                    # get the truth table
                    tt = model.getTruthTableByOutputID(term.getID())
                    if tt is not None:
                        return model.getTruthTableByOutputID(term.getID())
        return None


def get_instance_by_path(names: list):
    return get_top().get_child_instance(names)


class Instance:
    """Class that represents an instance in the design hierarchy.
    """

    def __init__(self, path=naja.SNLPath()):
        self.inst = None
        self.revisionCount = 0
        self.SNLID = [0] * 6
        if isinstance(path, naja.SNLPath):
            if path.size():
                self.pathIDs = path.getInstanceIDs()
                self.revisionCount = path.getTailInstance().getModel().getRevisionCount()
                self.inst = path.getTailInstance()
            else:
                self.pathIDs = []
        elif isinstance(path, list):
            self.pathIDs = path.copy()
            if path:
                self.inst = get_snl_instance_from_id_list(path)
                self.revisionCount = self.inst.getModel().getRevisionCount()
        if self.inst:
            self.SNLID = self.inst.getModel().getNLID()

    def __eq__(self, other) -> bool:
        return self.pathIDs == other.pathIDs

    def __str__(self):
        if self.is_top():
            top = self.__get_snl_model()
            if top is not None:
                return top.getName()
            else:
                return ""
        else:
            path = get_snl_path_from_id_list(self.pathIDs)
            return str(path)

    def __repr__(self) -> str:
        path = get_snl_path_from_id_list(self.pathIDs)
        return f"Instance({path})"

    def __hash__(self):
        return consistent_hash(self.pathIDs)

    def get_leaf_children(self):
        """Iterate over the leaf children of this Instance.
        Equivalent to the underlying leaves of the instanciation tree.

        :return: an iterator over the leaf children Instance of this Instance.
        :rtype: Iterator[Instance]
        """
        initial_path = get_snl_path_from_id_list(self.pathIDs)
        for inst in self.__get_snl_model().getInstances():
            if inst.getModel().isLeaf():
                yield Instance(naja.SNLPath(initial_path, inst))
            path = naja.SNLPath(initial_path, inst)
            stack = [[inst, path]]
            while stack:
                current = stack.pop()
                current_inst = current[0]
                current_path = current[1]
                for inst_child in current_inst.getModel().getInstances():
                    path_child = naja.SNLPath(current_path, inst_child)
                    if inst_child.getModel().isLeaf():
                        yield Instance(path_child)
                    stack.append([inst_child, path_child])

    def is_top(self) -> bool:
        """
        :return: True if this is the top design.
        :rtype: bool
        """
        return not self.pathIDs

    def is_assign(self) -> bool:
        """(assign a=b) will create an instance of assign connecting
        the wire a to the output of the assign and b to the input.

        :return: True if this is an assign (represented by an unnamed assign instance).
        :rtype: bool
        """
        return self.__get_snl_model().isAssign()

    def is_add(self) -> bool:
        """
        :return: True if this model is an NLDB0 addition primitive.
        :rtype: bool
        """
        model = self.__get_snl_model()
        return model.isAddSub() and not model.isSubtract()

    def is_sub(self) -> bool:
        """
        :return: True if this model is an NLDB0 subtraction primitive.
        :rtype: bool
        """
        model = self.__get_snl_model()
        return model.isSubtract()

    def is_mul(self) -> bool:
        """
        :return: True if this model is an NLDB0 multiplication primitive.
        :rtype: bool
        """
        model = self.__get_snl_model()
        return model.isMultiply()

    def is_shift(self) -> bool:
        """
        :return: True if this model is an NLDB0 shift primitive.
        :rtype: bool
        """
        model = self.__get_snl_model()
        return model.isShift()

    def is_divmod(self) -> bool:
        """
        :return: True if this model is an NLDB0 division/modulo primitive.
        :rtype: bool
        """
        model = self.__get_snl_model()
        return model.isDivMod()

    def is_blackbox(self) -> bool:
        """
        :return: True if this is a blackbox.
        :rtype: bool
        """
        return self.__get_snl_model().isBlackBox()

    def is_leaf(self) -> bool:
        """
        :return: True if this is a leaf.
        :rtype: bool
        """
        return self.__get_snl_model().isLeaf()

    def is_const0(self) -> bool:
        """
        :return: True if this is a constant 0 generator.
        :rtype: bool
        """
        return self.__get_snl_model().isConst0()

    def is_const1(self) -> bool:
        """
        :return: True if this is a constant 1 generator.
        :rtype: bool
        """
        return self.__get_snl_model().isConst1()

    def is_const(self) -> bool:
        """
        :return: True if this is a constant generator.
        :rtype: bool
        """
        return self.__get_snl_model().isConst()

    def is_buf(self) -> bool:
        """
        :return: True if this is a buffer.
        :rtype: bool
        """
        return self.__get_snl_model().isBuf()

    def is_inv(self) -> bool:
        """
        :return: True if this is an inverter.
        :rtype: bool
        """
        return self.__get_snl_model().isInv()

    def is_and(self) -> bool:
        """
        :return: True if this is an AND gate.
        :rtype: bool
        """
        return self.__get_snl_model().isAnd()

    def is_nand(self) -> bool:
        """
        :return: True if this is a NAND gate.
        :rtype: bool
        """
        return self.__get_snl_model().isNand()

    def is_or(self) -> bool:
        """
        :return: True if this is an OR gate.
        :rtype: bool
        """
        return self.__get_snl_model().isOr()

    def is_nor(self) -> bool:
        """
        :return: True if this is a NOR gate.
        :rtype: bool
        """
        return self.__get_snl_model().isNor()

    def is_xor(self) -> bool:
        """
        :return: True if this is an XOR gate.
        :rtype: bool
        """
        return self.__get_snl_model().isXor()

    def is_xnor(self) -> bool:
        """
        :return: True if this is an XNOR gate.
        :rtype: bool
        """
        return self.__get_snl_model().isXnor()

    def __get_snl_model(self):
        if self.is_top():
            return naja.NLUniverse.get().getTopDesign()
        if (
            self.inst.getModel().getRevisionCount() != self.revisionCount or
            self.inst.getModel().getNLID() != self.SNLID
        ):
            self.inst = get_snl_instance_from_id_list(self.pathIDs)
            self.revisionCount = self.inst.getModel().getRevisionCount()
            self.SNLID = self.inst.getModel().getNLID()

        return self.inst.getModel()

    def __get_leaf_snl_object(self):
        if self.is_top():
            return naja.NLUniverse.get().getTopDesign()
        return get_snl_instance_from_id_list(self.pathIDs)

    def __find_snl_model(self, name: str) -> naja.SNLDesign:
        u = naja.NLUniverse.get()
        for db in u.getUserDBs():
            for lib in db.getLibraries():
                found_model = lib.getSNLDesign(name)
                if found_model is not None:
                    return found_model
        return None

    def dump_full_dot(self, path: str):
        """Dump the full dot file of this instance."""
        self.__get_snl_model().dumpFullDotFile(path)

    def dump_context_dot(self, path: str):
        self.__get_snl_model().dumpContextDotFile(path)

    def get_child_instance(self, names: Union[str, list]):
        """
        :param names: the name of the child instance
            or the path to the child Instance as a list of names.
        :return: the child Instance at the given path or None if it does not exist.
        :rtype: Instance or None
        """
        if isinstance(names, str):
            names = [names]
        if not names:
            raise ValueError("Names argument cannot be empty")
        model = self.__get_snl_model()
        path = self.pathIDs.copy()
        for name in names:
            childInst = model.getInstance(name)
            if childInst is None:
                return None
            path.append(childInst.getID())
            model = childInst.getModel()
        return Instance(path)

    def get_child_instance_by_id(self, ids: Union[int, list[int]]):
        """
        :param ids: the ID of the child instance
            or the path to the child Instance as a list of IDs.
        :return: the child Instance at the given path or None if it does not exist.
        :rtype: Instance or None
        """
        if isinstance(ids, int):
            ids = [ids]
        if not ids:
            raise ValueError("IDs argument cannot be empty")
        model = self.__get_snl_model()
        path = self.pathIDs.copy()
        for id in ids:
            childInst = model.getInstanceByID(id)
            if childInst is None:
                return None
            path.append(childInst.getID())
            model = childInst.getModel()
        return Instance(path)

    def get_child_instances(self):
        """Iterate over the child instances of this instance.
        Equivalent to go down one level in hierarchy.

        :return: an iterator over the child instances of this instance.
        :rtype: Iterator[Instance]
        """
        path = get_snl_path_from_id_list(self.pathIDs)
        for inst in self.__get_snl_model().getInstances():
            path_child = naja.SNLPath(path, inst)
            yield Instance(path_child)
            # path.pop()

    def count_child_instances(self) -> int:
        """
        :return: the number of child instances of this instance.
        :rtype: int
        """
        return sum(1 for _ in self.__get_snl_model().getInstances())

    def get_nets(self):
        """Iterate over all scalar nets and bus nets.

        :return: an iterator over the nets of this Instance.
        :rtype: Iterator[Net]
        """
        for net in self.__get_snl_model().getNets():
            yield Net(self.pathIDs, net)

    def count_nets(self) -> int:
        """Count the number of scalar nets and bus nets of this Instance.

        :return: the number of nets of this Instance.
        :rtype: int
        """
        return sum(1 for _ in self.get_nets())

    def get_bit_nets(self):
        """Iterate over all scalar nets and bus net bits.

        :return: an iterator over the nets at bit level of this Instance.
        :rtype: Iterator[Net]
        """
        for net in self.__get_snl_model().getNets():
            if isinstance(net, naja.SNLBusNet):
                for bit in net.getBits():
                    yield Net(self.pathIDs, bit)
            else:
                yield Net(self.pathIDs, net)

    def count_bit_nets(self) -> int:
        """Count the number of scalar nets and bus net bits of this Instance.

        :return: the number of bit nets of this Instance.
        :rtype: int
        """
        return sum(1 for _ in self.get_bit_nets())

    def get_net(self, name: str) -> Net:
        """
        :param str name: the name of the Net to get.
        :return: the Net with the given name or None if it does not exist.
        :rtype: Net or None
        """
        net = self.__get_snl_model().getNet(name)
        if net is not None:
            return Net(self.pathIDs, net)
        return None

    def is_primitive(self) -> bool:
        """
        :return: True if this is a primitive.
        :rtype: bool
        """
        return self.__get_snl_model().isPrimitive()

    def is_sequential(self) -> bool:
        """
        :return: True if this is a sequential element.
        :rtype: bool
        """
        return self.__get_snl_model().isSequential()

    def has_modeling(self) -> bool:
        """
        :return: True if this instance has timing modeling.
        :rtype: bool
        """
        return self.__get_snl_model().hasModeling()

    def get_terms(self):
        """Iterate over all scalar terms and bus terms of this Instance.

        :return: the terms of this Instance.
        :rtype: Iterator[Term]
        """
        for term in self.__get_snl_model().getTerms():
            yield Term(self.pathIDs, term)

    def count_terms(self) -> int:
        """Count the number of scalar terms and bus terms of this Instance.

        :return: the number of terms of this Instance.
        :rtype: int
        """
        return sum(1 for _ in self.get_terms())

    def get_bit_terms(self):
        """Iterate over all scalar terms and bus term bits.

        :return: the bit terms of this Instance.
        :rtype: Iterator[Term]
        """
        for term in self.__get_snl_model().getBitTerms():
            yield Term(self.pathIDs, term)

    def count_bit_terms(self) -> int:
        """Count the number of scalar terms and bus term bits of this Instance.

        :return: the number of bit terms of this Instance.
        :rtype: int
        """
        return sum(1 for _ in self.get_bit_terms())

    def get_term(self, name: str) -> Term:
        """
        :param str name: the name of the Term to get.
        :return: the Term with the given name.
        :rtype: Term or None
        """
        term = self.__get_snl_model().getTerm(name)
        if term is not None:
            return Term(self.pathIDs, self.__get_snl_model().getTerm(name))
        return None

    def get_input_terms(self):
        """Iterate over all scalar input terms and bus input terms
        of this Instance.

        :return: the input terms of this Instance.
        :rtype: Iterator[Term]
        """
        for term in self.__get_snl_model().getTerms():
            if term.getDirection() != naja.SNLTerm.Direction.Output:
                yield Term(self.pathIDs, term)

    def count_input_terms(self) -> int:
        """Count the number of scalar input terms and bus input terms
        of this Instance.

        :return: the number of input terms of this Instance.
        :rtype: int
        """
        return sum(1 for _ in self.get_input_terms())

    def get_input_bit_terms(self):
        """Iterate over all scalar input terms and bus input term bits
        of this Instance.

        :return: the bit input terms of this Instance.
        :rtype: Iterator[Term]
        """
        for term in self.__get_snl_model().getTerms():
            if term.getDirection() != naja.SNLTerm.Direction.Output:
                if isinstance(term, naja.SNLBusTerm):
                    for bit in term.getBits():
                        yield Term(self.pathIDs, bit)
                else:
                    yield Term(self.pathIDs, term)

    def count_input_bit_terms(self) -> int:
        """Count the number of scalar input terms and bus input term bits
        of this Instance.

        :return: the number of bit input terms of this Instance.
        :rtype: int
        """
        return sum(1 for _ in self.get_input_bit_terms())

    def get_output_terms(self):
        """Iterate over all scalar output terms and bus output terms
        of this Instance.

        :return: the output terms of this Instance.
        :rtype: Iterator[Term]
        """
        for term in self.__get_snl_model().getTerms():
            if term.getDirection() != naja.SNLTerm.Direction.Input:
                yield Term(self.pathIDs, term)

    def count_output_terms(self) -> int:
        """Count the number of scalar output terms and bus output terms
        of this Instance.

        :return: the number of output terms of this Instance.
        :rtype: int
        """
        return sum(1 for _ in self.get_output_terms())

    def get_output_bit_terms(self):
        """Iterate over all scalar output terms and bus output term bits
        of this Instance.

        :return: the bit output terms of this Instance.
        :rtype: Iterator[Term]
        """
        for term in self.__get_snl_model().getTerms():
            if term.getDirection() != naja.SNLTerm.Direction.Input:
                if isinstance(term, naja.SNLBusTerm):
                    for bit in term.getBits():
                        yield Term(self.pathIDs, bit)
                else:
                    yield Term(self.pathIDs, term)

    def count_output_bit_terms(self) -> int:
        """Count the number of scalar output terms and bus output term bits
        of this Instance.

        :return: the number of bit output terms of this Instance.
        :rtype: int
        """
        return sum(1 for _ in self.get_output_bit_terms())

    def get_attributes(self) -> Iterator[Attribute]:
        """Iterate over the attributes of this Instance.

        :return: the attributes of this Instance.
        :rtype: Iterator[Attribute]
        """
        leaf_object = self.__get_leaf_snl_object()
        for attribute in leaf_object.getAttributes():
            yield Attribute(attribute)

    def has_source_references(self) -> bool:
        """Return whether this object has a source declaration or any source origins."""
        obj = self.__get_leaf_snl_object()
        return bool(obj and obj.hasSourceReferences())

    def get_source_declaration(self) -> Optional[SourceReference]:
        """Return this object's declaration in the loaded input, if available."""
        obj = self.__get_leaf_snl_object()
        return _source_reference_from_snl(obj.getSourceDeclaration()) if obj else None

    def get_source_origins(self) -> list[SourceReference]:
        """Return upstream source references in first-insertion order."""
        obj = self.__get_leaf_snl_object()
        return [_source_reference_from_snl(ref) for ref in obj.getSourceOrigins()] if obj else []

    def get_model_source_declaration(self) -> Optional[SourceReference]:
        """Return the model definition location, independently of this instantiation."""
        return _source_reference_from_snl(self.__get_snl_model().getSourceDeclaration())

    def get_source_range(self) -> Optional[SourceRange]:
        """Return the source range of this Instance, if available."""
        return _source_range_from_snl_source_loc(
            self.__get_leaf_snl_object().getSourceLoc()
        )

    def count_attributes(self) -> int:
        """Count the attributes of this Instance.

        :return: the number of attributes of this Instance.
        :rtype: int
        """
        return sum(1 for _ in self.get_attributes())

    def get_design(self) -> "Instance":
        """
        :return: the Instance containing this instance.
        :rtype: Instance
        """
        path = self.pathIDs.copy()
        if len(self.pathIDs) == 1:
            return get_top()
        path.pop()
        return Instance(path)

    def delete(self):
        """Delete this instance."""
        if self.is_top():
            raise ValueError("Cannot delete the top instance")
        # FIXME: should be upper path ?
        path = get_snl_path_from_id_list(self.pathIDs)
        inst = get_snl_instance_from_id_list(self.pathIDs)
        naja.SNLUniquifier(path)
        inst.destroy()

    def get_name(self) -> str:
        """
        :return: the name of the instance or name of the top is this is the top.
        :rtype: str
        """
        if self.is_top():
            return self.get_model_name()
        else:
            path = get_snl_path_from_id_list(self.pathIDs)
            return path.getTailInstance().getName()

    def set_name(self, name: str):
        """Set the name of this instance.

        :param str name: the new name of the instance.
        """
        if self.is_top():
            topSNLDesign = naja.NLUniverse.get().getTopDesign()
            topSNLDesign.setName(name)
        else:
            path = get_snl_path_from_id_list(self.pathIDs)
            # We need to uniquify until parent instance if parent instance
            # is not top. path.size == 1 for instance under top
            if path.size() > 1:
                naja.SNLUniquifier(path.getHeadPath())
                path = get_snl_path_from_id_list(self.pathIDs)
            inst = path.getTailInstance()
            inst.setName(name)

    def get_model_name(self) -> str:
        """
        :return: the name of the model of the instance
            or name of the top is this is the top.
        :rtype: str
        """
        return self.__get_snl_model().getName()

    def get_model_id(self) -> tuple[int, int, int]:
        """
        :return: the ID of the model of this Instance
            or ID of the top if this is the top.
        """
        model = self.__get_snl_model()
        return model.getDB().getID(), model.getLibrary().getID(), model.getID()

    def create_child_instance(self, model: str, name: str) -> "Instance":
        """Create a child instance with the given model and name.

        :param str model: the name of the model of the instance to create.
        :param str name: the name of the instance to create.
        :return: the created Instance.
        :rtype: Instance
        """
        path = get_snl_path_from_id_list(self.pathIDs)
        if path.size() > 0:
            naja.SNLUniquifier(path)
            path = get_snl_path_from_id_list(self.pathIDs)
        design = self.__get_snl_model()
        new_instance_model = self.__find_snl_model(model)
        if new_instance_model is None:
            raise ValueError(
                f"Cannot create instance {name} in {self}: model {model} cannot be found"
            )
        newSNLInstance = naja.SNLInstance.create(design, new_instance_model, name)
        path = naja.SNLPath(path, newSNLInstance)
        return Instance(path)

    def create_term(self, name: str, direction: Term.Direction) -> Term:
        """Create a Term in this Instance with the given name and direction.

        :param str name: the name of the Term to create.
        :param Term.Direction direction: the direction of the Term to create.
        :return: the created Term.
        """
        path = get_snl_path_from_id_list(self.pathIDs)
        if path.size() > 0:
            naja.SNLUniquifier(path)
            path = get_snl_path_from_id_list(self.pathIDs)
        design = self.__get_snl_model()
        newSNLTerm = naja.SNLScalarTerm.create(design, direction.value, name)
        return Term(path.getInstanceIDs(), newSNLTerm)

    def create_output_term(self, name: str) -> Term:
        """Create an output Term in this Instance with the given name.

        :param str name: the name of the Term to create.
        :return: the created Term.
        :rtype: Term
        """
        return self.create_term(name, Term.Direction.OUTPUT)

    def create_input_term(self, name: str) -> Term:
        """Create an input Term in this Instance with the given name.

        :param str name: the name of the Term to create.
        :return: the created Term.
        :rtype: Term
        """
        return self.create_term(name, Term.Direction.INPUT)

    def create_inout_term(self, name: str) -> Term:
        """Create an inout Term in this Instance with the given name.

        :param str name: the name of the Term to create.
        :return: the created Term.
        :rtype: Term
        """
        return self.create_term(name, Term.Direction.INOUT)

    def create_bus_term(self, name: str, msb: int, lsb: int, direction: Term.Direction) -> Term:
        """Create a bus Term in this Instance with the given name, msb, lsb and direction.

        :param str name: the name of the Term to create.
        :param int msb: the most significant bit of the Term to create.
        :param int lsb: the least significant bit of the Term to create.
        :param Term.Direction direction: the direction of the Term to create.
        :return: the created Term.
        """
        path = get_snl_path_from_id_list(self.pathIDs)
        if path.size() > 0:
            naja.SNLUniquifier(path)
        design = self.__get_snl_model()
        newSNLTerm = naja.SNLBusTerm.create(design, direction.value, msb, lsb, name)
        return Term(self.pathIDs, newSNLTerm)

    def create_inout_bus_term(self, name: str, msb: int, lsb: int) -> Term:
        """Create an inout bus Term in this Instance with the given name, msb and lsb.

        :param str name: the name of the Term to create.
        :param int msb: the most significant bit of the Term to create.
        :param int lsb: the least significant bit of the Term to create.
        :return: the created Term.
        :rtype: Term
        """
        return self.create_bus_term(name, msb, lsb, Term.Direction.INOUT)

    def create_output_bus_term(self, name: str, msb: int, lsb: int) -> Term:
        """Create an output bus Term in this Instance with the given name, msb and lsb.

        :param str name: the name of the Term to create.
        :param int msb: the most significant bit of the Term to create.
        :param int lsb: the least significant bit of the Term to create.
        :return: the created Term.
        :rtype: Term
        """
        return self.create_bus_term(name, msb, lsb, Term.Direction.OUTPUT)

    def create_input_bus_term(self, name: str, msb: int, lsb: int) -> Term:
        """Create an input bus Term in this Instance with the given name, msb and lsb.

        :param str name: the name of the Term to create.
        :param int msb: the most significant bit of the Term to create.
        :param int lsb: the least significant bit of the Term to create.
        :return: the created Term.
        :rtype: Term
        """
        return self.create_bus_term(name, msb, lsb, Term.Direction.INPUT)

    def create_net(self, name: str) -> Net:
        """Create a scalar Net in this Instance with the given name.

        :param str name: the name of the Net to create.
        :return: the created Net.
        :rtype: Net
        """
        path = get_snl_path_from_id_list(self.pathIDs)
        if path.size():
            naja.SNLUniquifier(path)
            path = get_snl_path_from_id_list(self.pathIDs)
        model = self.__get_snl_model()
        newSNLNet = naja.SNLScalarNet.create(model, name)
        return Net(path, newSNLNet)

    def create_bus_net(self, name: str, msb: int, lsb: int) -> Net:
        """Create a bus Net in this Instance with the given name, msb and lsb.

        :param str name: the name of the Net to create.
        :param int msb: the most significant bit of the Net to create.
        :param int lsb: the least significant bit of the Net to create.
        :return: the created Net.
        :rtype: Net
        """
        path = get_snl_path_from_id_list(self.pathIDs)
        if path.size():
            naja.SNLUniquifier(path)
            path = get_snl_path_from_id_list(self.pathIDs)
        model = self.__get_snl_model()
        newSNLNet = naja.SNLBusNet.create(model, msb, lsb, name)
        return Net(path, newSNLNet)

    def dump_gate_level_verilog(
            self,
            path: str,
            config: "GateLevelVerilogDumpConfig" = None):
        """Dump this instance as gate-level (structural) Verilog.

        This exports the netlist, including when loaded from RTL; it does not
        reconstruct the original behavioral RTL.

        :param str path: the file path where to dump the verilog.
        :param config: the configuration to use when dumping the verilog.
        :rtype: None
        :raises ValueError: if the path does not end with .v.
        :raises FileNotFoundError: if the directory of the path does not exist.
        """
        # path should be a file path of the form "path/to/file.v"
        if not path.endswith(".v"):
            raise ValueError("The path must end with .v")
        dir_path = os.path.dirname(path) or "."
        if not os.path.exists(dir_path):
            raise FileNotFoundError(f"The directory {dir_path} does not exist")
        if config is None:
            config = GateLevelVerilogDumpConfig()
        top_name = get_top().get_name()
        logger.info(
            f"Starting structural Verilog dumping for top '{top_name}' to '{path}'")
        start_time = time.time()
        dump_kwargs = {
            "dumpRTLInfosAsAttributes": config.dumpRTLInfosAsAttributes,
            "dumpAssignsAsInstances": config.dumpAssignsAsInstances,
            "verilatorSplitPackedSignals": config.verilatorSplitPackedSignals,
        }
        if config.dumpRTLInfosAsAttributes:
            dump_kwargs["rtlInfoDumpMode"] = config.rtlInfoDumpMode
        self.__get_snl_model().dumpGateLevelVerilog(
            dir_path,
            os.path.basename(path),
            **dump_kwargs)
        execution_time = time.time() - start_time
        logger.info(
            f"Gate-level Verilog dumping done for top '{top_name}' to '{path}' "
            f"in {execution_time:.2f} seconds")

    def dump_verilog(self, path: str, config: "GateLevelVerilogDumpConfig" = None):
        """Compatibility alias for :meth:`dump_gate_level_verilog`.

        This name will be deprecated soon and emits :class:`FutureWarning`.
        """
        warnings.warn(
            "Instance.dump_verilog will be deprecated soon; "
            "use Instance.dump_gate_level_verilog instead.",
            FutureWarning, stacklevel=2)
        return self.dump_gate_level_verilog(path, config)

    def get_truth_table(self):
        """
        :return: the truth table of the instance.
        :rtype: list[str]
        """
        return self.__get_snl_model().getTruthTable()

    def add_clock_related_inputs(self, clock_term: Term, input_terms: List[Term]):
        """Add input terms that are related to the given clock term.

        :param clock_term: the clock term to check for related inputs.
        :param input_terms: a list of input terms to add.
        :return: None
        """
        snlterms = [term.get_snl_term() for term in input_terms]
        self.__get_snl_model().addInputsToClockArcs(snlterms, clock_term.get_snl_term())

    def add_clock_related_outputs(self, clock_term: Term, output_terms: List[Term]):
        """Add output terms that are related to the given clock term.

        :param clock_term: the clock term to check for related outputs.
        :param output_terms: a list of output terms to add.
        :return: None
        """
        # convert Term objects to SNL terms
        snlterms = [term.get_snl_term() for term in output_terms]
        self.__get_snl_model().addClockToOutputsArcs(clock_term.get_snl_term(), snlterms)

    def add_combinatorial_arcs(self, input_terms: List[Term], output_terms: List[Term]):
        """Add input terms that are combinatorial inputs for the given output term.

        :param output_term: the output term to check for combinatorial inputs.
        :param input_terms: a list of input terms to add.
        :return: None
        """
        self.__get_snl_model().addCombinatorialArcs(
            [term.get_snl_term() for term in input_terms],
            [term.get_snl_term() for term in output_terms])


def __get_top_db() -> naja.NLDB:
    if naja.NLUniverse.get() is None:
        naja.NLUniverse.create()
    if naja.NLUniverse.get().getTopDB() is None:
        db = naja.NLDB.create(naja.NLUniverse.get())
        naja.NLUniverse.get().setTopDB(db)
    return naja.NLUniverse.get().getTopDB()


def reset():
    """Reset the environment by deleting everything.
    :rtype: None
    """
    u = naja.NLUniverse.get()
    if u is not None:
        u.destroy()


def get_top():
    """
    :return: the top Instance.
    :rtype: Instance
    """
    return Instance(naja.SNLPath())


def create_top(name: str) -> Instance:
    """Create a top instance with the given name.

    :param str name: the name of the top instance to create.
    :return: the created top Instance.
    :rtype: Instance
    """
    # init
    db = __get_top_db()
    # create top design
    lib = naja.NLLibrary.create(db)
    top = naja.SNLDesign.create(lib, name)
    naja.NLUniverse.get().setTopDesign(top)
    return Instance()


@dataclass
class GateLevelVerilogConfig:
    """Configure gate-level (structural) Verilog loading with :func:`load_gate_level_verilog`.

    For Verilog or SystemVerilog RTL, use :class:`SystemVerilogConfig` with
    :func:`load_system_verilog` instead.
    """

    keep_assigns: bool = True
    # Treat instantiations of unresolved (unknown) modules as AutoBlackBox cells
    # instead of failing the load.
    blackbox_unknown_modules: bool = False
    # Enable Verilog preprocessing (e.g. `define/ifdef/include).
    preprocess_enabled: bool = False
    # How to handle duplicate module names in the same library.
    # Accepted values: "forbid" (default), "first", "last", "verify".
    conflicting_design_name_policy: str = "forbid"
    # Deprecated alias for blackbox_unknown_modules; kept for backward
    # compatibility. Use blackbox_unknown_modules instead.
    allow_unknown_designs: bool = None

    # Explicitly identify src attributes as Yosys provenance.
    yosys_source_attributes: bool = False

    def __post_init__(self):
        self.validate()
        if self.allow_unknown_designs is not None:
            warnings.warn(
                f"{type(self).__name__}.allow_unknown_designs is deprecated; "
                "use blackbox_unknown_modules instead.",
                DeprecationWarning,
                stacklevel=2,
            )
            self.blackbox_unknown_modules = (
                self.blackbox_unknown_modules or self.allow_unknown_designs)

    def validate(self):
        for field_name in (
                "keep_assigns", "blackbox_unknown_modules", "preprocess_enabled",
                "yosys_source_attributes"):
            value = getattr(self, field_name)
            if not isinstance(value, bool):
                raise TypeError(
                    f"{type(self).__name__}.{field_name} must be a bool "
                    f"(got {type(value).__name__})")
        allowed = {"forbid", "first", "last", "verify"}
        if not isinstance(self.conflicting_design_name_policy, str):
            raise TypeError(
                f"{type(self).__name__}.conflicting_design_name_policy must be a str "
                f"(got {type(self.conflicting_design_name_policy).__name__})")
        if self.conflicting_design_name_policy not in allowed:
            raise ValueError(
                "Invalid conflicting_design_name_policy: "
                f"{self.conflicting_design_name_policy!r}. "
                "Expected one of: forbid, first, last, verify."
            )
        if self.allow_unknown_designs is not None:
            if not isinstance(self.allow_unknown_designs, bool):
                raise TypeError(
                    f"{type(self).__name__}.allow_unknown_designs must be a bool or None "
                    f"(got {type(self.allow_unknown_designs).__name__})")


@dataclass
class GateLevelVerilogDumpConfig:
    """Configure gate-level (structural) Verilog export."""

    dumpRTLInfosAsAttributes: bool = False
    rtlInfoDumpMode: str = "CompactAttribute"
    dumpAssignsAsInstances: bool = False
    verilatorSplitPackedSignals: bool = False

    def __post_init__(self):
        allowed = {"None", "VerboseAttributes", "CompactAttribute"}
        if self.rtlInfoDumpMode not in allowed:
            raise ValueError(
                "Invalid rtlInfoDumpMode: "
                f"{self.rtlInfoDumpMode!r}. "
                "Expected one of: None, VerboseAttributes, CompactAttribute."
            )


class VerilogConfig(GateLevelVerilogConfig):
    """Compatibility alias for :class:`GateLevelVerilogConfig`.

    This name will be deprecated soon and emits :class:`FutureWarning`.
    """

    def __post_init__(self):
        warnings.warn(
            "VerilogConfig will be deprecated soon; use GateLevelVerilogConfig instead.",
            FutureWarning, stacklevel=3)
        super().__post_init__()


class VerilogDumpConfig(GateLevelVerilogDumpConfig):
    """Compatibility alias for :class:`GateLevelVerilogDumpConfig`.

    This name will be deprecated soon and emits :class:`FutureWarning`.
    """

    def __post_init__(self):
        warnings.warn(
            "VerilogDumpConfig will be deprecated soon; "
            "use GateLevelVerilogDumpConfig instead.",
            FutureWarning, stacklevel=3)
        super().__post_init__()


@dataclass
class SystemVerilogConfig:
    keep_assigns: bool = True
    elaborated_ast_json_path: str = None
    # Set to None to disable the diagnostics report file (console-only diagnostics).
    diagnostics_report_path: Optional[str] = "naja_sv_diagnostics.log"
    pretty_print_elaborated_ast_json: bool = True
    include_source_info_in_elaborated_ast_json: bool = True
    flist: str = None
    top: str = None
    defines: list = None  # Slang preprocessor defines (e.g. ["SYNTHESIS", "WIDTH=32"])
    suppress_warnings: list = None  # Slang warning names to suppress (e.g. ["sign-conversion"])
    keep_ast_link: bool = False  # Keep live Slang AST to SNL object links when supported.
    # Treat instantiations of unresolved (unknown) modules as AutoBlackBox cells
    # instead of failing the load. Ports are inferred from the instantiation's
    # connections (direction InOut, width from the connected expression).
    blackbox_unknown_modules: bool = False
    # Omit the entire module body if a memory has unsupported multiple writers.
    blackbox_multi_writer_memories: bool = False
    # Share preprocessor macros across source files in input order.
    single_unit: bool = False
    # Fallback for design elements without an explicit time scale.
    default_timescale: Optional[str] = None

    def __post_init__(self):
        self.validate()

    def validate(self):
        for field_name in (
                "keep_assigns",
                "pretty_print_elaborated_ast_json",
                "include_source_info_in_elaborated_ast_json",
                "keep_ast_link",
                "blackbox_unknown_modules",
                "blackbox_multi_writer_memories",
                "single_unit"):
            value = getattr(self, field_name)
            if not isinstance(value, bool):
                raise TypeError(
                    f"SystemVerilogConfig.{field_name} must be a bool "
                    f"(got {type(value).__name__})")

        for field_name in (
                "elaborated_ast_json_path", "diagnostics_report_path", "flist"):
            value = getattr(self, field_name)
            if value is None:
                continue
            if not isinstance(value, (str, os.PathLike)):
                raise TypeError(
                    f"SystemVerilogConfig.{field_name} must be a path string or None "
                    f"(got {type(value).__name__})")
            path = os.fspath(value)
            if not path.strip():
                raise ValueError(
                    f"SystemVerilogConfig.{field_name} must not be empty")

        if self.top is not None:
            if not isinstance(self.top, str):
                raise TypeError(
                    "SystemVerilogConfig.top must be a str or None "
                    f"(got {type(self.top).__name__})")
            if not self.top.strip():
                raise ValueError("SystemVerilogConfig.top must not be empty")

        if self.default_timescale is not None:
            if not isinstance(self.default_timescale, str):
                raise TypeError("SystemVerilogConfig.default_timescale must be a str or None")
            if not self.default_timescale.strip():
                raise ValueError("SystemVerilogConfig.default_timescale must not be empty")

        self._validate_string_list("defines", allow_whitespace=False)
        self._validate_string_list("suppress_warnings", allow_whitespace=False)
        if self.suppress_warnings:
            for index, warning in enumerate(self.suppress_warnings):
                if warning.startswith("-W"):
                    raise ValueError(
                        "SystemVerilogConfig.suppress_warnings"
                        f"[{index}] must be the warning name without a -W/-Wno- "
                        f"prefix; got {warning!r}")

    def _validate_string_list(self, field_name: str, allow_whitespace: bool):
        values = getattr(self, field_name)
        if values is None:
            return
        if not isinstance(values, list):
            raise TypeError(
                f"SystemVerilogConfig.{field_name} must be a list[str] or None "
                f"(got {type(values).__name__})")
        for index, value in enumerate(values):
            if not isinstance(value, str):
                raise TypeError(
                    f"SystemVerilogConfig.{field_name}[{index}] must be a str "
                    f"(got {type(value).__name__})")
            if not value.strip():
                raise ValueError(
                    f"SystemVerilogConfig.{field_name}[{index}] must not be empty")
            if not allow_whitespace and any(char.isspace() for char in value):
                raise ValueError(
                    f"SystemVerilogConfig.{field_name}[{index}] must not contain "
                    f"whitespace; got {value!r}")


def _normalize_input_files(
        files,
        format_name: str,
        *,
        allow_empty: bool = False) -> List[str]:
    """Validate public loader input before native state is mutated."""
    if isinstance(files, (str, os.PathLike)):
        candidates = [files]
    elif isinstance(files, list):
        candidates = files
    else:
        raise TypeError(
            f"{format_name} files must be a path string or list of path strings "
            f"(got {type(files).__name__})")

    if not candidates and not allow_empty:
        raise ValueError(f"No {format_name} input files were provided")

    normalized = []
    for index, candidate in enumerate(candidates):
        if not isinstance(candidate, (str, os.PathLike)):
            raise TypeError(
                f"{format_name} files[{index}] must be a path string "
                f"(got {type(candidate).__name__})")
        path = os.fspath(candidate)
        if not path.strip():
            raise ValueError(f"{format_name} files[{index}] must not be empty")
        normalized.append(path)

    for index, path in enumerate(normalized):
        if not os.path.exists(path):
            raise FileNotFoundError(
                f"{format_name} input file does not exist: {path!r} "
                f"(files[{index}], resolved to {os.path.abspath(path)!r})")
        if not os.path.isfile(path):
            raise ValueError(
                f"{format_name} input path is not a file: {path!r} "
                f"(files[{index}], resolved to {os.path.abspath(path)!r})")
    return normalized


def load_gate_level_verilog(
        files: Union[str, List[str]],
        config: GateLevelVerilogConfig = None) -> Instance:
    """Load gate-level (structural) Verilog netlists into the top design.

    This loader handles structural connectivity, not general behavioral RTL.
    Use :func:`load_system_verilog` for Verilog or SystemVerilog RTL, including
    ``.v`` files, within that frontend's supported subset.

    :param files: a list of gate-level Verilog files to load or a single file.
    :param config: the configuration to use when loading the files.
    :return: the top Instance.
    :rtype: Instance
    :raises TypeError: if files or config have the wrong type.
    :raises ValueError: if no files are provided or a path is empty.
    :raises FileNotFoundError: if an input file does not exist.
    """
    if config is None:
        config = GateLevelVerilogConfig()  # Use default settings
    elif not isinstance(config, GateLevelVerilogConfig):
        raise TypeError(
            "load_gate_level_verilog config must be a GateLevelVerilogConfig or None "
            f"(got {type(config).__name__})")
    # Revalidate in case callers mutated a config after constructing it.
    config.validate()
    files = _normalize_input_files(files, "Verilog")
    start_time = time.time()
    logger.info(f"Starting structural Verilog loading for files: {', '.join(files)}")
    __get_top_db().loadGateLevelVerilog(
        files,
        keep_assigns=config.keep_assigns,
        blackbox_unknown_modules=config.blackbox_unknown_modules,
        preprocess_enabled=config.preprocess_enabled,
        yosys_source_attributes=config.yosys_source_attributes,
        conflicting_design_name_policy=config.conflicting_design_name_policy,
    )
    execution_time = time.time() - start_time
    top = get_top()
    logger.info(
        f"Gate-level Verilog loading done for top '{top.get_name()}' in "
        f"{execution_time:.2f} seconds")
    return top


def load_verilog(
        files: Union[str, List[str]],
        config: GateLevelVerilogConfig = None) -> Instance:
    """Compatibility alias for :func:`load_gate_level_verilog`.

    This name will be deprecated soon and emits :class:`FutureWarning`.
    """
    warnings.warn(
        "load_verilog will be deprecated soon; use load_gate_level_verilog instead.",
        FutureWarning, stacklevel=2)
    if config is not None and not isinstance(config, GateLevelVerilogConfig):
        raise TypeError(
            "load_verilog config must be a VerilogConfig or None "
            f"(got {type(config).__name__})")
    return load_gate_level_verilog(files, config)


def _validate_hdl_library(library):
    if not isinstance(library, str):
        raise TypeError("library must be a str")
    if not library.strip():
        raise ValueError("library must not be empty")


def load_system_verilog(
        files: Union[str, List[str]],
        config: SystemVerilogConfig = None, *, library: str = "DESIGN") -> Instance:
    """Elaborate Verilog or SystemVerilog RTL into the top design using slang.

    Accepts ``.v`` and ``.sv`` sources within the frontend's supported RTL
    subset. This is not a full language implementation or a simulator.

    :param files: a list of SystemVerilog files to load or a single file.
    :param config: the configuration to use when loading the files. Set
        single_unit=True to share macros across files in source order,
        including files supplied through flist (default: False).
    :param library: destination root library name (default "DESIGN").
    :return: the top Instance.
    :rtype: Instance
    :raises TypeError: if files or config have the wrong type.
    :raises ValueError: if no files and no flist are provided, or configuration is empty.
    :raises FileNotFoundError: if an input file or flist does not exist.
    """
    _validate_hdl_library(library)
    if config is None:
        config = SystemVerilogConfig()
    elif not isinstance(config, SystemVerilogConfig):
        raise TypeError(
            "load_system_verilog config must be a SystemVerilogConfig or None "
            f"(got {type(config).__name__})")
    # Revalidate in case callers mutated a config after constructing it.
    config.validate()
    files = _normalize_input_files(files, "SystemVerilog", allow_empty=True)
    if not files and not config.flist:
        raise ValueError(
            "No SystemVerilog inputs were provided; pass at least one file "
            "or set SystemVerilogConfig.flist")
    if config.flist:
        flist_path = os.fspath(config.flist)
        if not os.path.exists(flist_path):
            raise FileNotFoundError(
                "SystemVerilog command file does not exist: "
                f"{flist_path!r} (SystemVerilogConfig.flist, resolved to "
                f"{os.path.abspath(flist_path)!r})")
        if not os.path.isfile(flist_path):
            raise ValueError(
                "SystemVerilogConfig.flist is not a file: "
                f"{flist_path!r} (resolved to {os.path.abspath(flist_path)!r})")
    start_time = time.time()
    if files:
        logger.info(f"Starting SystemVerilog loading for files: {', '.join(files)}")
    else:
        logger.info(f"Starting SystemVerilog loading from flist: {config.flist}")
    if config.top is not None:
        logger.info(f"SystemVerilog loading top override requested: {config.top}")

    effective_flist = config.flist
    temp_flist_path = None
    if config.top is not None or config.single_unit:
        # Forward frontend options through a temporary slang command file.
        with tempfile.NamedTemporaryFile(
                "w", suffix=".f", delete=False, encoding="utf-8") as options_flist:
            temp_flist_path = options_flist.name
            if config.top is not None:
                options_flist.write(f"--top {config.top}\n")
            if config.single_unit:
                options_flist.write("--single-unit\n")
            if config.flist:
                quoted_flist = os.fspath(config.flist).replace(
                    "\\", "\\\\").replace("\"", "\\\"")
                options_flist.write(f"-f \"{quoted_flist}\"\n")
        effective_flist = temp_flist_path

    try:
        __get_top_db().loadSystemVerilog(
            files,
            keep_assigns=config.keep_assigns,
            elaborated_ast_json_path=(
                None if config.elaborated_ast_json_path is None
                else os.fspath(config.elaborated_ast_json_path)),
            diagnostics_report_path=(
                None if config.diagnostics_report_path is None
                else os.fspath(config.diagnostics_report_path)),
            pretty_print_elaborated_ast_json=config.pretty_print_elaborated_ast_json,
            include_source_info_in_elaborated_ast_json=(
                config.include_source_info_in_elaborated_ast_json),
            flist=(
                None if effective_flist is None
                else os.fspath(effective_flist)),
            defines=config.defines,
            blackbox_unknown_modules=config.blackbox_unknown_modules,
            blackbox_multi_writer_memories=config.blackbox_multi_writer_memories,
            library=library,
            suppress_warnings=config.suppress_warnings,
            default_timescale=config.default_timescale,
            keep_ast_link=config.keep_ast_link,
        )
    finally:
        if temp_flist_path and os.path.exists(temp_flist_path):
            os.remove(temp_flist_path)

    execution_time = time.time() - start_time
    top = get_top()
    logger.info(
        f"SystemVerilog loading done for top '{top.get_name()}' in {execution_time:.2f} seconds")
    return top


def load_vhdl(file: Union[str, os.PathLike, List[Union[str, os.PathLike]]],
              top: Optional[str] = None, *,
              diagnostics_report_path: Optional[Union[str, os.PathLike]] =
              "naja_vhdl_diagnostics.log", library: str = "DESIGN") -> Instance:
    """Load VHDL source files into the top design, in any order.

    VHDL loading is experimental and currently supports the bounded ``bit`` and
    ``bit_vector`` subset implemented by the native frontend. Integer and boolean
    generic defaults and generic-map actuals specialize the supported RTL;
    integer values can also specialize vector bounds. Files resolve independently
    of their order. Omit ``top`` for a unique uninstantiated entity, or pass it
    explicitly to select a root.

    :param file: one VHDL source path or a list of paths, in any order.
    :param top: optional top entity name for structural hierarchy.
    :param diagnostics_report_path: report for all warning occurrences, overwritten
        per load; None selects console-only. Console warnings appear once per code.
    :param library: destination root library name (default "DESIGN").
    :return: the top Instance.
    :rtype: Instance
    :raises TypeError: if file or top has the wrong type.
    :raises ValueError: if file or top is empty, or file is not a regular file.
    :raises FileNotFoundError: if file does not exist.
    """
    _validate_hdl_library(library)
    if diagnostics_report_path is not None:
        if not isinstance(diagnostics_report_path, (str, os.PathLike)):
            raise TypeError("load_vhdl diagnostics_report_path must be a path string or None")
        diagnostics_report_path = os.fspath(diagnostics_report_path)
        if not isinstance(diagnostics_report_path, str):
            raise TypeError("load_vhdl diagnostics_report_path must be a path string or None")
        if not diagnostics_report_path.strip():
            raise ValueError(
                "load_vhdl diagnostics_report_path must not be empty; "
                "use None for console-only")
    batch = isinstance(file, list)
    files = file if batch else [file]
    if not files:
        raise ValueError("VHDL files must not be empty")
    paths = []
    for item in files:
        if not isinstance(item, (str, os.PathLike)):
            raise TypeError(
                "VHDL file must be a path string or list of paths "
                f"(got {type(item).__name__})")
        path = os.fspath(item)
        if not isinstance(path, str):
            raise TypeError("VHDL file must be a path string")
        if not path.strip():
            raise ValueError("VHDL file must not be empty")
        paths.append(path)
    if top is not None:
        if not isinstance(top, str):
            raise TypeError(
                "load_vhdl top must be a str or None "
                f"(got {type(top).__name__})")
        if not top.strip():
            raise ValueError("load_vhdl top must not be empty")
    for path in paths:
        if not os.path.exists(path):
            raise FileNotFoundError(
                f"VHDL input file does not exist: {path!r} "
                f"(resolved to {os.path.abspath(path)!r})")
        if not os.path.isfile(path):
            raise ValueError(
                f"VHDL input path is not a file: {path!r} "
                f"(resolved to {os.path.abspath(path)!r})")

    start_time = time.time()
    logger.info(f"Starting VHDL loading for files: {paths}")
    if top is not None:
        logger.info(f"VHDL loading top override requested: {top}")
    __get_top_db().loadVHDL(
        paths if batch else paths[0], top=top,
        diagnostics_report_path=diagnostics_report_path, library=library)
    execution_time = time.time() - start_time
    loaded_top = get_top()
    logger.info(
        f"VHDL loading done for top '{loaded_top.get_name()}' in "
        f"{execution_time:.2f} seconds")
    return loaded_top


def load_liberty(files: Union[str, List[str]]):
    """Load liberty files.

    :param files: a list of liberty files to load or a single file.
    :rtype: None
    :raises TypeError: if files has the wrong type.
    :raises ValueError: if no files are provided or a path is empty.
    :raises FileNotFoundError: if an input file does not exist.
    """
    files = _normalize_input_files(files, "Liberty")
    logger.info(f"Loading liberty files: {', '.join(files)}")
    __get_top_db().loadLibertyPrimitives(files)


def load_primitives(name: str):
    """Loads a primitive library embedded in najaeda.

    Currently supported libraries are:

    - xilinx
    - yosys

    :param str name: the name of the primitives library to load.
    :raises ValueError: if the name is not recognized.
    :rtype: None
    """
    if name == "xilinx":
        from najaeda.primitives import xilinx
        xilinx.load(__get_top_db())
    elif name == "yosys":
        from najaeda.primitives import yosys
        yosys.load(__get_top_db())
    else:
        raise ValueError(f"Unknown primitives library: {name}")


def load_primitives_from_file(file: str):
    """Loads a primitives library from a file.

    :param str file: the path to the primitives library file.

    The file must define a function ``load(db)``.
    """
    logger.info(f"Loading primitives from file: {file}")
    if not os.path.isfile(file):
        raise FileNotFoundError(f"Cannot load primitives from non existing file: {file}")
    import importlib.util
    spec = importlib.util.spec_from_file_location("user_module", file)
    module = importlib.util.module_from_spec(spec)
    sys.modules["user_module"] = module
    spec.loader.exec_module(module)

    if not hasattr(module, "load"):
        raise RuntimeError(f"The file {file} must define a function named `load(db)`")

    db = __get_top_db()
    module.load(db)


def load_naja_if(path: str):
    """Load the Naja IF from the given path."""
    if not os.path.isdir(path):
        raise FileNotFoundError(f"Cannot load Naja IF from non existing directory: {path}")
    logger.info(f"Loading Naja IF from {path}")
    naja.NLDB.loadNajaIF(path)


def dump_naja_if(path: str):
    """Dump the Naja IF to the given path."""
    __get_top_db().dumpNajaIF(path)


def get_primitives_library() -> naja.NLLibrary:
    lib = __get_top_db().getLibrary("PRIMS")
    if lib is None:
        lib = naja.NLLibrary.createPrimitives(__get_top_db(), "PRIMS")
    return lib


def get_model_name(id: tuple[int, int, int]) -> str:
    """
    :param tuple[int, int, int] id: the id of the model.
    :return: the name of the model given its id or None if it does not exist.
    :rtype: str or None
    """
    u = naja.NLUniverse.get()
    if u:
        db = u.getDB(id[0])
        if db:
            lib = db.getLibrary(id[1])
            if lib:
                model = lib.getSNLDesign(id[2])
                if model:
                    return model.getName()
    return None


def apply_dle():
    """Apply the DLE (Dead Logic Elimination) to the top design.
    :rtype: None
    """
    top = naja.NLUniverse.get().getTopDesign()
    if top is not None:
        naja.NLUniverse.get().applyDLE()


def apply_constant_propagation():
    """Apply constant propagation to the top design.
    :rtype: None
    """
    top = naja.NLUniverse.get().getTopDesign()
    if top is not None:
        naja.NLUniverse.get().applyConstantPropagation()


def get_max_fanout() -> list:
    """Get the maximum fanout of the top design.

    :return: the maximum fanout of the top design.
    :rtype: int
    """
    u = naja.NLUniverse.get()
    if u is not None:
        top = naja.NLUniverse.get().getTopDesign()
        if top is not None:
            max_fanout = naja.NLUniverse.get().getMaxFanout()
            result = []
            index = 0
            result.append(max_fanout[0])
            fanouts = []
            for entry in max_fanout[1]:
                fanout = []
                driver = entry[0]
                component = driver.getNetComponent()
                path = driver.getPath().getInstanceIDs()
                if isinstance(component, naja.SNLInstTerm):
                    path.append(component.getInstance().getID())
                    component = component.getBitTerm()
                print(component)
                print(path)
                fanout.append(Term(path, component))
                readers = []
                for item in entry[1]:
                    component = item.getNetComponent()
                    path = item.getPath().getInstanceIDs()
                    if isinstance(component, naja.SNLInstTerm):
                        path.append(component.getInstance().getID())
                        component = component.getBitTerm()
                    readers.append(Term(path, component))
                fanout.append(readers)
                fanouts.append(fanout)
                index += 1
            result.append(fanouts)
            return result
    return [0]


def get_max_logic_level() -> list:
    """Get the maximum logic level of the top design.

    :return: the maximum logic level of the top design.
    :rtype: int
    """
    u = naja.NLUniverse.get()
    if u is not None:
        top = naja.NLUniverse.get().getTopDesign()
        if top is not None:
            max_logic_level = naja.NLUniverse.get().getMaxLogicLevel()
            result = []
            index = 0
            for entry in max_logic_level:
                if index == 0:
                    result.append(max_logic_level[0])
                else:
                    paths = []
                    for path in entry:
                        llpath = []
                        for item in path:
                            component = item.getNetComponent()
                            pathIDs = item.getPath().getInstanceIDs()
                            if isinstance(component, naja.SNLInstTerm):
                                pathIDs.append(component.getInstance().getID())
                                component = component.getBitTerm()
                            llpath.append(Term(pathIDs, component))
                        paths.append(llpath)
                    result.append(paths)
                index += 1
            return result
    return [0]
