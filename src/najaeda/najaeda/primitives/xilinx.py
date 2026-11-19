# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import logging

logger = logging.getLogger(__name__)
try:
    from najaeda import naja
except ImportError:  # pragma: no cover
    import naja
try:
    from . import utils
except ImportError:  # pragma: no cover
    import utils


def constructIBUF(lib):
    ibuf = naja.SNLDesign.createPrimitive(lib, "IBUF")
    i = naja.SNLScalarTerm.create(ibuf, naja.SNLTerm.Direction.Input, "I")
    o = naja.SNLScalarTerm.create(ibuf, naja.SNLTerm.Direction.Output, "O")
    ibuf.addCombinatorialArcs(i, o)


def constructOBUF(lib):
    obuf = naja.SNLDesign.createPrimitive(lib, "OBUF")
    i = naja.SNLScalarTerm.create(obuf, naja.SNLTerm.Direction.Input, "I")
    o = naja.SNLScalarTerm.create(obuf, naja.SNLTerm.Direction.Output, "O")
    obuf.addCombinatorialArcs(i, o)


def constructBUFG(lib):
    bufg = naja.SNLDesign.createPrimitive(lib, "BUFG")
    i = naja.SNLScalarTerm.create(bufg, naja.SNLTerm.Direction.Input, "I")
    o = naja.SNLScalarTerm.create(bufg, naja.SNLTerm.Direction.Output, "O")
    bufg.addCombinatorialArcs(i, o)


def constructDSP48E1(lib):
    dsp48e1 = naja.SNLDesign.createPrimitive(lib, "DSP48E1")
    naja.SNLBusTerm.create(dsp48e1, naja.SNLTerm.Direction.Output, 29, 0, "ACOUT")
    naja.SNLBusTerm.create(dsp48e1, naja.SNLTerm.Direction.Output, 17, 0, "BCOUT")
    naja.SNLScalarTerm.create(dsp48e1, naja.SNLTerm.Direction.Output, "CARRYCASCOUT")
    naja.SNLScalarTerm.create(dsp48e1, naja.SNLTerm.Direction.Input, "MULTSIGNOUT")
    naja.SNLBusTerm.create(dsp48e1, naja.SNLTerm.Direction.Input, 47, 0, "PCOUT")
    naja.SNLScalarTerm.create(dsp48e1, naja.SNLTerm.Direction.Output, "OVERFLOW")
    naja.SNLScalarTerm.create(dsp48e1, naja.SNLTerm.Direction.Output, "PATTERNBDETECT")
    naja.SNLScalarTerm.create(dsp48e1, naja.SNLTerm.Direction.Output, "PATTERNDETECT")
    naja.SNLScalarTerm.create(dsp48e1, naja.SNLTerm.Direction.Output, "UNDERFLOW")
    naja.SNLBusTerm.create(dsp48e1, naja.SNLTerm.Direction.Output, 3, 0, "CARRYOUT")
    naja.SNLBusTerm.create(dsp48e1, naja.SNLTerm.Direction.Output, 47, 0, "P")
    naja.SNLBusTerm.create(dsp48e1, naja.SNLTerm.Direction.Input, 29, 0, "ACIN")
    naja.SNLBusTerm.create(dsp48e1, naja.SNLTerm.Direction.Input, 17, 0, "BCIN")
    naja.SNLScalarTerm.create(dsp48e1, naja.SNLTerm.Direction.Input, "CARRYCASCIN")
    naja.SNLScalarTerm.create(dsp48e1, naja.SNLTerm.Direction.Input, "MULTSIGNIN")
    naja.SNLBusTerm.create(dsp48e1, naja.SNLTerm.Direction.Input, 47, 0, "PCIN")
    naja.SNLBusTerm.create(dsp48e1, naja.SNLTerm.Direction.Input, 3, 0, "ALUMODE")
    naja.SNLBusTerm.create(dsp48e1, naja.SNLTerm.Direction.Input, 2, 0, "CARRYINSEL")
    clk = naja.SNLScalarTerm.create(dsp48e1, naja.SNLTerm.Direction.Input, "CLK")
    naja.SNLBusTerm.create(dsp48e1, naja.SNLTerm.Direction.Input, 4, 0, "INMODE")
    naja.SNLBusTerm.create(dsp48e1, naja.SNLTerm.Direction.Input, 6, 0, "OPMODE")
    naja.SNLBusTerm.create(dsp48e1, naja.SNLTerm.Direction.Input, 29, 0, "A")
    naja.SNLBusTerm.create(dsp48e1, naja.SNLTerm.Direction.Input, 17, 0, "B")
    naja.SNLBusTerm.create(dsp48e1, naja.SNLTerm.Direction.Input, 47, 0, "C")
    naja.SNLScalarTerm.create(dsp48e1, naja.SNLTerm.Direction.Input, "CARRYIN")
    naja.SNLBusTerm.create(dsp48e1, naja.SNLTerm.Direction.Input, 24, 0, "D")
    naja.SNLScalarTerm.create(dsp48e1, naja.SNLTerm.Direction.Input, "CEA1")
    naja.SNLScalarTerm.create(dsp48e1, naja.SNLTerm.Direction.Input, "CEA2")
    naja.SNLScalarTerm.create(dsp48e1, naja.SNLTerm.Direction.Input, "CEAD")
    naja.SNLScalarTerm.create(dsp48e1, naja.SNLTerm.Direction.Input, "CEALUMODE")
    naja.SNLScalarTerm.create(dsp48e1, naja.SNLTerm.Direction.Input, "CEB1")
    naja.SNLScalarTerm.create(dsp48e1, naja.SNLTerm.Direction.Input, "CEB2")
    naja.SNLScalarTerm.create(dsp48e1, naja.SNLTerm.Direction.Input, "CEC")
    naja.SNLScalarTerm.create(dsp48e1, naja.SNLTerm.Direction.Input, "CECARRYIN")
    naja.SNLScalarTerm.create(dsp48e1, naja.SNLTerm.Direction.Input, "CECTRL")
    naja.SNLScalarTerm.create(dsp48e1, naja.SNLTerm.Direction.Input, "CED")
    naja.SNLScalarTerm.create(dsp48e1, naja.SNLTerm.Direction.Input, "CEINMODE")
    naja.SNLScalarTerm.create(dsp48e1, naja.SNLTerm.Direction.Input, "CEM")
    naja.SNLScalarTerm.create(dsp48e1, naja.SNLTerm.Direction.Input, "CEP")
    naja.SNLScalarTerm.create(dsp48e1, naja.SNLTerm.Direction.Input, "RSTA")
    naja.SNLScalarTerm.create(dsp48e1, naja.SNLTerm.Direction.Input, "RSTALLCARRYIN")
    naja.SNLScalarTerm.create(dsp48e1, naja.SNLTerm.Direction.Input, "RSTALUMODE")
    naja.SNLScalarTerm.create(dsp48e1, naja.SNLTerm.Direction.Input, "RSTB")
    naja.SNLScalarTerm.create(dsp48e1, naja.SNLTerm.Direction.Input, "RSTC")
    naja.SNLScalarTerm.create(dsp48e1, naja.SNLTerm.Direction.Input, "RSTCTRL")
    naja.SNLScalarTerm.create(dsp48e1, naja.SNLTerm.Direction.Input, "RSTD")
    naja.SNLScalarTerm.create(dsp48e1, naja.SNLTerm.Direction.Input, "RSTINMODE")
    naja.SNLScalarTerm.create(dsp48e1, naja.SNLTerm.Direction.Input, "RSTM")
    naja.SNLScalarTerm.create(dsp48e1, naja.SNLTerm.Direction.Input, "RSTP")
    naja.SNLParameter.createDecimal(dsp48e1, "ACASCREG", 1)
    naja.SNLParameter.createDecimal(dsp48e1, "ADREG", 1)
    naja.SNLParameter.createString(dsp48e1, "A_INPUT", "DIRECT")
    naja.SNLParameter.createDecimal(dsp48e1, "ALUMODEREG", 1)
    naja.SNLParameter.createDecimal(dsp48e1, "AREG", 1)
    naja.SNLParameter.createDecimal(dsp48e1, "BCASCREG", 1)
    naja.SNLParameter.createString(dsp48e1, "B_INPUT", "DIRECT")
    naja.SNLParameter.createDecimal(dsp48e1, "BREG", 1)
    naja.SNLParameter.createDecimal(dsp48e1, "CARRYINREG", 1)
    naja.SNLParameter.createDecimal(dsp48e1, "CARRYINSELREG", 1)
    naja.SNLParameter.createDecimal(dsp48e1, "CREG", 1)
    naja.SNLParameter.createDecimal(dsp48e1, "DREG", 1)
    naja.SNLParameter.createDecimal(dsp48e1, "INMODEREG", 1)
    naja.SNLParameter.createDecimal(dsp48e1, "MREG", 1)
    naja.SNLParameter.createDecimal(dsp48e1, "OPMODEREG", 1)
    naja.SNLParameter.createDecimal(dsp48e1, "PREG", 1)
    naja.SNLParameter.createBoolean(dsp48e1, "USE_DPORT", False)
    naja.SNLParameter.createString(dsp48e1, "USE_MULT", "MULTIPLY")
    naja.SNLParameter.createString(dsp48e1, "USE_SIMD", "ONE48")
    term_roles = {}
    for name in (
        "CEA1", "CEA2", "CEAD", "CEALUMODE", "CEB1", "CEB2", "CEC",
        "CECARRYIN", "CECTRL", "CED", "CEINMODE", "CEM", "CEP"
    ):
        term_roles[dsp48e1.getScalarTerm(name)] = naja.SNLTermRole.Enable
    for name in (
        "RSTA", "RSTALLCARRYIN", "RSTALUMODE", "RSTB", "RSTC", "RSTCTRL",
        "RSTD", "RSTINMODE", "RSTM", "RSTP"
    ):
        term_roles[dsp48e1.getScalarTerm(name)] = (
            naja.SNLTermRole.SyncReset,
            naja.SNLActiveLevel.High,
        )
    utils.constructSequentialPrimitive(dsp48e1, clk, term_roles)


def constructINV(lib):
    inv = naja.SNLDesign.createPrimitive(lib, "INV")
    i = naja.SNLScalarTerm.create(inv, naja.SNLTerm.Direction.Input, "I")
    o = naja.SNLScalarTerm.create(inv, naja.SNLTerm.Direction.Output, "O")
    naja.SNLDesign.addCombinatorialArcs(i, o)


def constructCARRY4(lib):
    carry4 = naja.SNLDesign.createPrimitive(lib, "CARRY4")
    o = naja.SNLBusTerm.create(carry4, naja.SNLTerm.Direction.Output, 3, 0, "O")
    co = naja.SNLBusTerm.create(carry4, naja.SNLTerm.Direction.Output, 3, 0, "CO")
    di = naja.SNLBusTerm.create(carry4, naja.SNLTerm.Direction.Input, 3, 0, "DI")
    s = naja.SNLBusTerm.create(carry4, naja.SNLTerm.Direction.Input, 3, 0, "S")
    cyinit = naja.SNLScalarTerm.create(carry4, naja.SNLTerm.Direction.Input, "CYINIT")
    ci = naja.SNLScalarTerm.create(carry4, naja.SNLTerm.Direction.Input, "CI")
    o_bits = [b for b in o.getBits()]
    co_bits = [b for b in co.getBits()]
    di_bits = [b for b in di.getBits()]
    s_bits = [b for b in s.getBits()]
    # cyinit and ci are in combinatorial dependency with o and co outputs
    naja.SNLDesign.addCombinatorialArcs([cyinit, ci], [o, co])
    naja.SNLDesign.addCombinatorialArcs(s_bits[0], [o, co])
    naja.SNLDesign.addCombinatorialArcs(s_bits[1], [o_bits[1], o_bits[2], o_bits[3]])
    naja.SNLDesign.addCombinatorialArcs(s_bits[1], [co_bits[1], co_bits[2], co_bits[3]])
    naja.SNLDesign.addCombinatorialArcs(s_bits[2], [o_bits[2], o_bits[3]])
    naja.SNLDesign.addCombinatorialArcs(s_bits[2], [co_bits[2], co_bits[3]])
    naja.SNLDesign.addCombinatorialArcs(s_bits[3], o_bits[3])
    naja.SNLDesign.addCombinatorialArcs(s_bits[3], co_bits[3])
    naja.SNLDesign.addCombinatorialArcs(di_bits[0], [o_bits[1], o_bits[2], o_bits[3]])
    naja.SNLDesign.addCombinatorialArcs(di_bits[0], co)
    naja.SNLDesign.addCombinatorialArcs(di_bits[1], [o_bits[2], o_bits[3]])
    naja.SNLDesign.addCombinatorialArcs(di_bits[1], [co_bits[1], co_bits[2], co_bits[3]])
    naja.SNLDesign.addCombinatorialArcs(di_bits[2], o_bits[3])
    naja.SNLDesign.addCombinatorialArcs(di_bits[2], [co_bits[2], co_bits[3]])
    naja.SNLDesign.addCombinatorialArcs(di_bits[3], co_bits[3])


def constructLUT1(lib):
    lut1 = naja.SNLDesign.createPrimitive(lib, "LUT1")
    i0 = naja.SNLScalarTerm.create(lut1, naja.SNLTerm.Direction.Input, "I0")
    o = naja.SNLScalarTerm.create(lut1, naja.SNLTerm.Direction.Output, "O")
    naja.SNLDesign.addCombinatorialArcs(i0, o)
    naja.SNLParameter.createBinary(lut1, "INIT", 2, 0b00)


def constructLUT2(lib):
    lut2 = naja.SNLDesign.createPrimitive(lib, "LUT2")
    i0 = naja.SNLScalarTerm.create(lut2, naja.SNLTerm.Direction.Input, "I0")
    i1 = naja.SNLScalarTerm.create(lut2, naja.SNLTerm.Direction.Input, "I1")
    o = naja.SNLScalarTerm.create(lut2, naja.SNLTerm.Direction.Output, "O")
    naja.SNLDesign.addCombinatorialArcs([i0, i1], o)
    naja.SNLParameter.createBinary(lut2, "INIT", 4, 0x0)


def constructLUT3(lib):
    lut3 = naja.SNLDesign.createPrimitive(lib, "LUT3")
    i0 = naja.SNLScalarTerm.create(lut3, naja.SNLTerm.Direction.Input, "I0")
    i1 = naja.SNLScalarTerm.create(lut3, naja.SNLTerm.Direction.Input, "I1")
    i2 = naja.SNLScalarTerm.create(lut3, naja.SNLTerm.Direction.Input, "I2")
    o = naja.SNLScalarTerm.create(lut3, naja.SNLTerm.Direction.Output, "O")
    naja.SNLDesign.addCombinatorialArcs([i0, i1, i2], o)
    naja.SNLParameter.createBinary(lut3, "INIT", 8, 0x00)


def constructLUT4(lib):
    lut4 = naja.SNLDesign.createPrimitive(lib, "LUT4")
    i0 = naja.SNLScalarTerm.create(lut4, naja.SNLTerm.Direction.Input, "I0")
    i1 = naja.SNLScalarTerm.create(lut4, naja.SNLTerm.Direction.Input, "I1")
    i2 = naja.SNLScalarTerm.create(lut4, naja.SNLTerm.Direction.Input, "I2")
    i3 = naja.SNLScalarTerm.create(lut4, naja.SNLTerm.Direction.Input, "I3")
    o = naja.SNLScalarTerm.create(lut4, naja.SNLTerm.Direction.Output, "O")
    naja.SNLDesign.addCombinatorialArcs([i0, i1, i2, i3], o)
    naja.SNLParameter.createBinary(lut4, "INIT", 16, 0x0000)


def constructLUT5(lib):
    lut5 = naja.SNLDesign.createPrimitive(lib, "LUT5")
    i0 = naja.SNLScalarTerm.create(lut5, naja.SNLTerm.Direction.Input, "I0")
    i1 = naja.SNLScalarTerm.create(lut5, naja.SNLTerm.Direction.Input, "I1")
    i2 = naja.SNLScalarTerm.create(lut5, naja.SNLTerm.Direction.Input, "I2")
    i3 = naja.SNLScalarTerm.create(lut5, naja.SNLTerm.Direction.Input, "I3")
    i4 = naja.SNLScalarTerm.create(lut5, naja.SNLTerm.Direction.Input, "I4")
    o = naja.SNLScalarTerm.create(lut5, naja.SNLTerm.Direction.Output, "O")
    naja.SNLDesign.addCombinatorialArcs([i0, i1, i2, i3, i4], o)
    naja.SNLParameter.createBinary(lut5, "INIT", 32, 0x00000000)


def constructLUT6(lib):
    lut6 = naja.SNLDesign.createPrimitive(lib, "LUT6")
    i0 = naja.SNLScalarTerm.create(lut6, naja.SNLTerm.Direction.Input, "I0")
    i1 = naja.SNLScalarTerm.create(lut6, naja.SNLTerm.Direction.Input, "I1")
    i2 = naja.SNLScalarTerm.create(lut6, naja.SNLTerm.Direction.Input, "I2")
    i3 = naja.SNLScalarTerm.create(lut6, naja.SNLTerm.Direction.Input, "I3")
    i4 = naja.SNLScalarTerm.create(lut6, naja.SNLTerm.Direction.Input, "I4")
    i5 = naja.SNLScalarTerm.create(lut6, naja.SNLTerm.Direction.Input, "I5")
    o = naja.SNLScalarTerm.create(lut6, naja.SNLTerm.Direction.Output, "O")
    naja.SNLDesign.addCombinatorialArcs([i0, i1, i2, i3, i4, i5], o)
    naja.SNLParameter.createBinary(lut6, "INIT", 64, 0x0000000000000000)


def constructMUXF7(lib):
    muxf7 = naja.SNLDesign.createPrimitive(lib, "MUXF7")
    i0 = naja.SNLScalarTerm.create(muxf7, naja.SNLTerm.Direction.Input, "I0")
    i1 = naja.SNLScalarTerm.create(muxf7, naja.SNLTerm.Direction.Input, "I1")
    o = naja.SNLScalarTerm.create(muxf7, naja.SNLTerm.Direction.Output, "O")
    s = naja.SNLScalarTerm.create(muxf7, naja.SNLTerm.Direction.Input, "S")
    naja.SNLDesign.addCombinatorialArcs([i0, i1, s], o)


def constructMUXF8(lib):
    muxf8 = naja.SNLDesign.createPrimitive(lib, "MUXF8")
    i0 = naja.SNLScalarTerm.create(muxf8, naja.SNLTerm.Direction.Input, "I0")
    i1 = naja.SNLScalarTerm.create(muxf8, naja.SNLTerm.Direction.Input, "I1")
    o = naja.SNLScalarTerm.create(muxf8, naja.SNLTerm.Direction.Output, "O")
    s = naja.SNLScalarTerm.create(muxf8, naja.SNLTerm.Direction.Input, "S")
    naja.SNLDesign.addCombinatorialArcs([i0, i1, s], o)


def constructSRL16E(lib):
    srl16e = naja.SNLDesign.createPrimitive(lib, "SRL16E")
    ce = naja.SNLScalarTerm.create(srl16e, naja.SNLTerm.Direction.Input, "CE")
    clk = naja.SNLScalarTerm.create(srl16e, naja.SNLTerm.Direction.Input, "CLK")
    naja.SNLScalarTerm.create(srl16e, naja.SNLTerm.Direction.Input, "D")
    naja.SNLScalarTerm.create(srl16e, naja.SNLTerm.Direction.Output, "Q")
    naja.SNLScalarTerm.create(srl16e, naja.SNLTerm.Direction.Input, "A0")
    naja.SNLScalarTerm.create(srl16e, naja.SNLTerm.Direction.Input, "A1")
    naja.SNLScalarTerm.create(srl16e, naja.SNLTerm.Direction.Input, "A2")
    naja.SNLScalarTerm.create(srl16e, naja.SNLTerm.Direction.Input, "A3")
    naja.SNLParameter.createBinary(srl16e, "INIT", 16, 0x0000)
    naja.SNLParameter.createBinary(srl16e, "IS_CLK_INVERTED", 1, 0)
    utils.constructSequentialPrimitive(
        srl16e, clk, {ce: naja.SNLTermRole.Enable}
    )


def _set_flip_flop_model(primitive, next_state, clear=None, preset=None):
    state = {"name": "IQ", "next_state": next_state,
             "init_from": {"parameter": "INIT", "bit": 0}}
    if clear is not None:
        state["clear"] = clear
    if preset is not None:
        state["preset"] = preset
    primitive.setSequentialModel(
        clocked_on="C",
        states=[state],
        outputs=[(primitive.getScalarTerm("Q"), "IQ")],
    )


def constructFDCE(lib):
    fdce = naja.SNLDesign.createPrimitive(lib, "FDCE")
    naja.SNLScalarTerm.create(fdce, naja.SNLTerm.Direction.Output, "Q")
    c = naja.SNLScalarTerm.create(fdce, naja.SNLTerm.Direction.Input, "C")
    ce = naja.SNLScalarTerm.create(fdce, naja.SNLTerm.Direction.Input, "CE")
    clr = naja.SNLScalarTerm.create(fdce, naja.SNLTerm.Direction.Input, "CLR")
    naja.SNLScalarTerm.create(fdce, naja.SNLTerm.Direction.Input, "D")
    naja.SNLParameter.createBinary(fdce, "INIT", 1, 0b0)
    utils.constructSequentialPrimitive(
        fdce,
        c,
        {
            ce: naja.SNLTermRole.Enable,
            clr: (naja.SNLTermRole.AsyncReset, naja.SNLActiveLevel.High),
        },
    )
    _set_flip_flop_model(fdce, "(CE & D) | (!CE & IQ)", clear="CLR")


def constructFDPE(lib):
    fdpe = naja.SNLDesign.createPrimitive(lib, "FDPE")
    naja.SNLScalarTerm.create(fdpe, naja.SNLTerm.Direction.Output, "Q")
    c = naja.SNLScalarTerm.create(fdpe, naja.SNLTerm.Direction.Input, "C")
    ce = naja.SNLScalarTerm.create(fdpe, naja.SNLTerm.Direction.Input, "CE")
    pre = naja.SNLScalarTerm.create(fdpe, naja.SNLTerm.Direction.Input, "PRE")
    naja.SNLScalarTerm.create(fdpe, naja.SNLTerm.Direction.Input, "D")
    naja.SNLParameter.createBinary(fdpe, "INIT", 1, 0b1)
    utils.constructSequentialPrimitive(
        fdpe,
        c,
        {
            ce: naja.SNLTermRole.Enable,
            pre: (naja.SNLTermRole.AsyncSet, naja.SNLActiveLevel.High),
        },
    )
    _set_flip_flop_model(fdpe, "(CE & D) | (!CE & IQ)", preset="PRE")


def constructFDRE(lib):
    fdre = naja.SNLDesign.createPrimitive(lib, "FDRE")
    naja.SNLScalarTerm.create(fdre, naja.SNLTerm.Direction.Output, "Q")
    c = naja.SNLScalarTerm.create(fdre, naja.SNLTerm.Direction.Input, "C")
    ce = naja.SNLScalarTerm.create(fdre, naja.SNLTerm.Direction.Input, "CE")
    r = naja.SNLScalarTerm.create(fdre, naja.SNLTerm.Direction.Input, "R")
    naja.SNLScalarTerm.create(fdre, naja.SNLTerm.Direction.Input, "D")
    naja.SNLParameter.createBinary(fdre, "INIT", 1, 0b0)
    utils.constructSequentialPrimitive(
        fdre,
        c,
        {
            ce: naja.SNLTermRole.Enable,
            r: (naja.SNLTermRole.SyncReset, naja.SNLActiveLevel.High),
        },
    )
    _set_flip_flop_model(fdre, "(!R) & ((CE & D) | (!CE & IQ))")


def constructFDSE(lib):
    fdse = naja.SNLDesign.createPrimitive(lib, "FDSE")
    naja.SNLScalarTerm.create(fdse, naja.SNLTerm.Direction.Output, "Q")
    c = naja.SNLScalarTerm.create(fdse, naja.SNLTerm.Direction.Input, "C")
    ce = naja.SNLScalarTerm.create(fdse, naja.SNLTerm.Direction.Input, "CE")
    s = naja.SNLScalarTerm.create(fdse, naja.SNLTerm.Direction.Input, "S")
    naja.SNLScalarTerm.create(fdse, naja.SNLTerm.Direction.Input, "D")
    naja.SNLParameter.createBinary(fdse, "INIT", 1, 0b0)
    utils.constructSequentialPrimitive(
        fdse,
        c,
        {
            ce: naja.SNLTermRole.Enable,
            s: (naja.SNLTermRole.SyncSet, naja.SNLActiveLevel.High),
        },
    )
    _set_flip_flop_model(fdse, "S | ((CE & D) | (!CE & IQ))")


def constructRAM32M(lib):
    ram32m = naja.SNLDesign.createPrimitive(lib, "RAM32M")
    doa = naja.SNLBusTerm.create(ram32m, naja.SNLTerm.Direction.Output, 1, 0, "DOA")
    dob = naja.SNLBusTerm.create(ram32m, naja.SNLTerm.Direction.Output, 1, 0, "DOB")
    doc = naja.SNLBusTerm.create(ram32m, naja.SNLTerm.Direction.Output, 1, 0, "DOC")
    dod = naja.SNLBusTerm.create(ram32m, naja.SNLTerm.Direction.Output, 1, 0, "DOD")
    addra = naja.SNLBusTerm.create(ram32m, naja.SNLTerm.Direction.Input, 4, 0, "ADDRA")
    addrb = naja.SNLBusTerm.create(ram32m, naja.SNLTerm.Direction.Input, 4, 0, "ADDRB")
    addrc = naja.SNLBusTerm.create(ram32m, naja.SNLTerm.Direction.Input, 4, 0, "ADDRC")
    addrd = naja.SNLBusTerm.create(ram32m, naja.SNLTerm.Direction.Input, 4, 0, "ADDRD")
    naja.SNLDesign.addCombinatorialArcs(doa, list(addra.getBits()))
    naja.SNLDesign.addCombinatorialArcs(dob, list(addrb.getBits()))
    naja.SNLDesign.addCombinatorialArcs(doc, list(addrc.getBits()))
    naja.SNLDesign.addCombinatorialArcs(dod, list(addrd.getBits()))
    dia = naja.SNLBusTerm.create(ram32m, naja.SNLTerm.Direction.Input, 1, 0, "DIA")
    dib = naja.SNLBusTerm.create(ram32m, naja.SNLTerm.Direction.Input, 1, 0, "DIB")
    dic = naja.SNLBusTerm.create(ram32m, naja.SNLTerm.Direction.Input, 1, 0, "DIC")
    did = naja.SNLBusTerm.create(ram32m, naja.SNLTerm.Direction.Input, 1, 0, "DID")
    wclk = naja.SNLScalarTerm.create(ram32m, naja.SNLTerm.Direction.Input, "WCLK")
    we = naja.SNLScalarTerm.create(ram32m, naja.SNLTerm.Direction.Input, "WE")
    naja.SNLDesign.addInputsToClockArcs([dia, dib, dic, did, we], wclk)
    utils.setTermRole(wclk, naja.SNLTermRole.Clock)
    utils.setTermRole(
        [addra, addrb, addrc, addrd], naja.SNLTermRole.MemoryReadAddress
    )
    utils.setTermRole([doa, dob, doc, dod], naja.SNLTermRole.MemoryReadData)
    utils.setTermRole([dia, dib, dic, did], naja.SNLTermRole.MemoryWriteData)
    utils.setTermRole(we, naja.SNLTermRole.MemoryWriteEnable)
    naja.SNLParameter.createBinary(ram32m, "INIT_A", 64, 0)
    naja.SNLParameter.createBinary(ram32m, "INIT_B", 64, 0)
    naja.SNLParameter.createBinary(ram32m, "INIT_C", 64, 0)
    naja.SNLParameter.createBinary(ram32m, "INIT_D", 64, 0)


def constructRAM64M(lib):
    ram64m = naja.SNLDesign.createPrimitive(lib, "RAM64M")
    doa = naja.SNLScalarTerm.create(ram64m, naja.SNLTerm.Direction.Output, "DOA")
    dob = naja.SNLScalarTerm.create(ram64m, naja.SNLTerm.Direction.Output, "DOB")
    doc = naja.SNLScalarTerm.create(ram64m, naja.SNLTerm.Direction.Output, "DOC")
    dod = naja.SNLScalarTerm.create(ram64m, naja.SNLTerm.Direction.Output, "DOD")
    addra = naja.SNLBusTerm.create(ram64m, naja.SNLTerm.Direction.Input, 5, 0, "ADDRA")
    addrb = naja.SNLBusTerm.create(ram64m, naja.SNLTerm.Direction.Input, 5, 0, "ADDRB")
    addrc = naja.SNLBusTerm.create(ram64m, naja.SNLTerm.Direction.Input, 5, 0, "ADDRC")
    addrd = naja.SNLBusTerm.create(ram64m, naja.SNLTerm.Direction.Input, 5, 0, "ADDRD")
    naja.SNLDesign.addCombinatorialArcs(doa, list(addra.getBits()))
    naja.SNLDesign.addCombinatorialArcs(dob, list(addrb.getBits()))
    naja.SNLDesign.addCombinatorialArcs(doc, list(addrc.getBits()))
    naja.SNLDesign.addCombinatorialArcs(dod, list(addrd.getBits()))
    dia = naja.SNLScalarTerm.create(ram64m, naja.SNLTerm.Direction.Input, "DIA")
    dib = naja.SNLScalarTerm.create(ram64m, naja.SNLTerm.Direction.Input, "DIB")
    dic = naja.SNLScalarTerm.create(ram64m, naja.SNLTerm.Direction.Input, "DIC")
    did = naja.SNLScalarTerm.create(ram64m, naja.SNLTerm.Direction.Input, "DID")
    wclk = naja.SNLScalarTerm.create(ram64m, naja.SNLTerm.Direction.Input, "WCLK")
    we = naja.SNLScalarTerm.create(ram64m, naja.SNLTerm.Direction.Input, "WE")
    naja.SNLDesign.addInputsToClockArcs([dia, dib, dic, did, we], wclk)
    utils.setTermRole(wclk, naja.SNLTermRole.Clock)
    utils.setTermRole(
        [addra, addrb, addrc, addrd], naja.SNLTermRole.MemoryReadAddress
    )
    utils.setTermRole([doa, dob, doc, dod], naja.SNLTermRole.MemoryReadData)
    utils.setTermRole([dia, dib, dic, did], naja.SNLTermRole.MemoryWriteData)
    utils.setTermRole(we, naja.SNLTermRole.MemoryWriteEnable)
    naja.SNLParameter.createBinary(ram64m, "INIT_A", 64, 0)
    naja.SNLParameter.createBinary(ram64m, "INIT_B", 64, 0)
    naja.SNLParameter.createBinary(ram64m, "INIT_C", 64, 0)
    naja.SNLParameter.createBinary(ram64m, "INIT_D", 64, 0)
    naja.SNLParameter.createBinary(ram64m, "IS_WCLK_INVERTED", 1, 0)


def constructRAMB18E1(lib):
    a_inputs = []
    b_inputs = []
    a_outputs = []
    b_outputs = []
    ramb18e1 = naja.SNLDesign.createPrimitive(lib, "RAMB18E1")
    addra = naja.SNLBusTerm.create(ramb18e1, naja.SNLTerm.Direction.Input, 13, 0, "ADDRARDADDR")
    a_inputs.extend(list(addra.getBits()))
    addrb = naja.SNLBusTerm.create(ramb18e1, naja.SNLTerm.Direction.Input, 13, 0, "ADDRBWRADDR")
    b_inputs.extend(list(addrb.getBits()))
    clka = naja.SNLScalarTerm.create(ramb18e1, naja.SNLTerm.Direction.Input, "CLKARDCLK")
    clkb = naja.SNLScalarTerm.create(ramb18e1, naja.SNLTerm.Direction.Input, "CLKBWRCLK")
    diadi = naja.SNLBusTerm.create(ramb18e1, naja.SNLTerm.Direction.Input, 15, 0, "DIADI")
    a_inputs.extend(list(diadi.getBits()))
    dibdi = naja.SNLBusTerm.create(ramb18e1, naja.SNLTerm.Direction.Input, 15, 0, "DIBDI")
    b_inputs.extend(list(dibdi.getBits()))
    dipadip = naja.SNLBusTerm.create(ramb18e1, naja.SNLTerm.Direction.Input, 1, 0, "DIPADIP")
    a_inputs.extend(list(dipadip.getBits()))
    dipbdip = naja.SNLBusTerm.create(ramb18e1, naja.SNLTerm.Direction.Input, 1, 0, "DIPBDIP")
    b_inputs.extend(list(dipbdip.getBits()))
    doado = naja.SNLBusTerm.create(ramb18e1, naja.SNLTerm.Direction.Output, 15, 0, "DOADO")
    a_outputs.extend(list(doado.getBits()))
    dobdo = naja.SNLBusTerm.create(ramb18e1, naja.SNLTerm.Direction.Output, 15, 0, "DOBDO")
    b_outputs.extend(list(dobdo.getBits()))
    dopadop = naja.SNLBusTerm.create(ramb18e1, naja.SNLTerm.Direction.Output, 1, 0, "DOPADOP")
    a_outputs.extend(list(dopadop.getBits()))
    dopbdop = naja.SNLBusTerm.create(ramb18e1, naja.SNLTerm.Direction.Output, 1, 0, "DOPBDOP")
    b_outputs.extend(list(dopbdop.getBits()))
    enarden = naja.SNLScalarTerm.create(ramb18e1, naja.SNLTerm.Direction.Input, "ENARDEN")
    a_inputs.append(enarden)
    enbwren = naja.SNLScalarTerm.create(ramb18e1, naja.SNLTerm.Direction.Input, "ENBWREN")
    b_inputs.append(enbwren)
    regcear = naja.SNLScalarTerm.create(ramb18e1, naja.SNLTerm.Direction.Input, "REGCEAREGCE")
    a_inputs.append(regcear)
    regceb = naja.SNLScalarTerm.create(ramb18e1, naja.SNLTerm.Direction.Input, "REGCEB")
    b_inputs.append(regceb)
    rstrama = naja.SNLScalarTerm.create(ramb18e1, naja.SNLTerm.Direction.Input, "RSTRAMARSTRAM")
    a_inputs.append(rstrama)
    rstramb = naja.SNLScalarTerm.create(ramb18e1, naja.SNLTerm.Direction.Input, "RSTRAMB")
    b_inputs.append(rstramb)
    rstregar = naja.SNLScalarTerm.create(ramb18e1, naja.SNLTerm.Direction.Input, "RSTREGARSTREG")
    a_inputs.append(rstregar)
    rstregb = naja.SNLScalarTerm.create(ramb18e1, naja.SNLTerm.Direction.Input, "RSTREGB")
    b_inputs.append(rstregb)
    wea = naja.SNLBusTerm.create(ramb18e1, naja.SNLTerm.Direction.Input, 1, 0, "WEA")
    a_inputs.extend(list(wea.getBits()))
    webwe = naja.SNLBusTerm.create(ramb18e1, naja.SNLTerm.Direction.Input, 3, 0, "WEBWE")
    b_inputs.extend(list(webwe.getBits()))
    naja.SNLParameter.createDecimal(ramb18e1, "DOA_REG", 0)
    naja.SNLParameter.createDecimal(ramb18e1, "DOB_REG", 0)
    naja.SNLParameter.createBinary(ramb18e1, "INIT_A", 18, 0x00000)
    naja.SNLParameter.createBinary(ramb18e1, "INIT_B", 18, 0x00000)
    for i in range(64):
        paramName = "INIT_" + hex(i)[2:].zfill(2).upper()
        naja.SNLParameter.createBinary(ramb18e1, paramName, 256, 0)
    for i in range(8):
        paramName = "INITP_" + hex(i)[2:].zfill(2).upper()
        naja.SNLParameter.createBinary(ramb18e1, paramName, 256, 0)
    naja.SNLParameter.createString(ramb18e1, "RAM_MODE", "TDP")
    naja.SNLParameter.createDecimal(ramb18e1, "READ_WIDTH_A", 0)
    naja.SNLParameter.createDecimal(ramb18e1, "READ_WIDTH_B", 0)
    naja.SNLParameter.createBinary(ramb18e1, "SRVAL_A", 18, 0)
    naja.SNLParameter.createBinary(ramb18e1, "SRVAL_B", 18, 0)
    naja.SNLParameter.createString(ramb18e1, "WRITE_MODE_A", "WRITE_FIRST")
    naja.SNLParameter.createString(ramb18e1, "WRITE_MODE_B", "WRITE_FIRST")
    naja.SNLParameter.createDecimal(ramb18e1, "WRITE_WIDTH_A", 0)
    naja.SNLParameter.createDecimal(ramb18e1, "WRITE_WIDTH_B", 0)
    naja.SNLDesign.addInputsToClockArcs(a_inputs, clka)
    naja.SNLDesign.addInputsToClockArcs(b_inputs, clkb)
    naja.SNLDesign.addClockToOutputsArcs(clka, a_outputs)
    naja.SNLDesign.addClockToOutputsArcs(clkb, b_outputs)
    utils.setTermRole(a_inputs + b_inputs, naja.SNLTermRole.DataInput)
    utils.setTermRole(a_outputs + b_outputs, naja.SNLTermRole.DataOutput)
    utils.setTermRole([clka, clkb], naja.SNLTermRole.Clock)
    utils.setTermRole([addra, addrb], naja.SNLTermRole.MemoryReadAddress)
    utils.setTermRole(
        [diadi, dibdi, dipadip, dipbdip], naja.SNLTermRole.MemoryWriteData
    )
    utils.setTermRole(
        [doado, dobdo, dopadop, dopbdop], naja.SNLTermRole.MemoryReadData
    )
    utils.setTermRole([enarden, enbwren, regcear, regceb], naja.SNLTermRole.Enable)
    utils.setTermRole(
        [rstrama, rstramb, rstregar, rstregb],
        naja.SNLTermRole.SyncReset,
        naja.SNLActiveLevel.High,
    )
    utils.setTermRole([wea, webwe], naja.SNLTermRole.MemoryWriteEnable)


def constructRAMB36E1(lib):
    a_inputs = []
    b_inputs = []
    a_outputs = []
    b_outputs = []
    ramb36e1 = naja.SNLDesign.createPrimitive(lib, "RAMB36E1")
    addra = naja.SNLBusTerm.create(ramb36e1, naja.SNLTerm.Direction.Input, 15, 0, "ADDRARDADDR")
    a_inputs.extend(list(addra.getBits()))
    addrb = naja.SNLBusTerm.create(ramb36e1, naja.SNLTerm.Direction.Input, 15, 0, "ADDRBWRADDR")
    b_inputs.extend(list(addrb.getBits()))
    cascadeina = naja.SNLScalarTerm.create(ramb36e1, naja.SNLTerm.Direction.Input, "CASCADEINA")
    a_inputs.append(cascadeina)
    cascadeinb = naja.SNLScalarTerm.create(ramb36e1, naja.SNLTerm.Direction.Input, "CASCADEINB")
    b_inputs.append(cascadeinb)
    cascadeouta = naja.SNLScalarTerm.create(ramb36e1, naja.SNLTerm.Direction.Output, "CASCADEOUTA")
    a_outputs.append(cascadeouta)
    cascadeoutb = naja.SNLScalarTerm.create(ramb36e1, naja.SNLTerm.Direction.Output, "CASCADEOUTB")
    b_outputs.append(cascadeoutb)
    clka = naja.SNLScalarTerm.create(ramb36e1, naja.SNLTerm.Direction.Input, "CLKARDCLK")
    clkb = naja.SNLScalarTerm.create(ramb36e1, naja.SNLTerm.Direction.Input, "CLKBWRCLK")
    dbiterr = naja.SNLScalarTerm.create(ramb36e1, naja.SNLTerm.Direction.Output, "DBITERR")
    a_outputs.append(dbiterr)  # not sure about which kind of modeling to put here...
    diadi = naja.SNLBusTerm.create(ramb36e1, naja.SNLTerm.Direction.Input, 31, 0, "DIADI")
    a_inputs.extend(list(diadi.getBits()))
    dibdi = naja.SNLBusTerm.create(ramb36e1, naja.SNLTerm.Direction.Input, 31, 0, "DIBDI")
    b_inputs.extend(list(dibdi.getBits()))
    dipadip = naja.SNLBusTerm.create(ramb36e1, naja.SNLTerm.Direction.Input, 3, 0, "DIPADIP")
    a_inputs.extend(list(dipadip.getBits()))
    dipbdip = naja.SNLBusTerm.create(ramb36e1, naja.SNLTerm.Direction.Input, 3, 0, "DIPBDIP")
    b_inputs.extend(list(dipbdip.getBits()))
    doado = naja.SNLBusTerm.create(ramb36e1, naja.SNLTerm.Direction.Output, 31, 0, "DOADO")
    a_outputs.extend(list(doado.getBits()))
    dobdo = naja.SNLBusTerm.create(ramb36e1, naja.SNLTerm.Direction.Output, 31, 0, "DOBDO")
    b_outputs.extend(list(dobdo.getBits()))
    dopadop = naja.SNLBusTerm.create(ramb36e1, naja.SNLTerm.Direction.Output, 3, 0, "DOPADOP")
    a_outputs.extend(list(dopadop.getBits()))
    dopbdop = naja.SNLBusTerm.create(ramb36e1, naja.SNLTerm.Direction.Output, 3, 0, "DOPBDOP")
    b_outputs.extend(list(dopbdop.getBits()))
    eccparity = naja.SNLBusTerm.create(ramb36e1, naja.SNLTerm.Direction.Output, 7, 0, "ECCPARITY")
    # not sure about which kind of modeling to put here.
    a_outputs.extend(list(eccparity.getBits()))
    enarden = naja.SNLScalarTerm.create(ramb36e1, naja.SNLTerm.Direction.Input, "ENARDEN")
    a_inputs.append(enarden)
    enbwren = naja.SNLScalarTerm.create(ramb36e1, naja.SNLTerm.Direction.Input, "ENBWREN")
    b_inputs.append(enbwren)
    injectdbiterr = naja.SNLScalarTerm.create(
        ramb36e1, naja.SNLTerm.Direction.Input, "INJECTDBITERR"
    )
    injectsbiterr = naja.SNLScalarTerm.create(
        ramb36e1, naja.SNLTerm.Direction.Input, "INJECTSBITERR"
    )
    a_inputs.append(injectdbiterr)  # not sure about which kind of modeling to put here.
    a_inputs.append(injectsbiterr)  # not sure about which kind of modeling to put here.
    rdaddrecc = naja.SNLBusTerm.create(ramb36e1, naja.SNLTerm.Direction.Output, 8, 0, "RDADDRECC")
    a_outputs.extend(list(rdaddrecc.getBits()))
    regcearegce = naja.SNLScalarTerm.create(ramb36e1, naja.SNLTerm.Direction.Input, "REGCEAREGCE")
    a_inputs.append(regcearegce)
    regceb = naja.SNLScalarTerm.create(ramb36e1, naja.SNLTerm.Direction.Input, "REGCEB")
    b_inputs.append(regceb)
    rstramarstram = naja.SNLScalarTerm.create(
        ramb36e1, naja.SNLTerm.Direction.Input, "RSTRAMARSTRAM"
    )
    a_inputs.append(rstramarstram)
    rstramb = naja.SNLScalarTerm.create(ramb36e1, naja.SNLTerm.Direction.Input, "RSTRAMB")
    b_inputs.append(rstramb)
    rstregarstreg = naja.SNLScalarTerm.create(
        ramb36e1, naja.SNLTerm.Direction.Input, "RSTREGARSTREG"
    )
    a_inputs.append(rstregarstreg)
    rstregb = naja.SNLScalarTerm.create(ramb36e1, naja.SNLTerm.Direction.Input, "RSTREGB")
    b_inputs.append(rstregb)
    sbiterr = naja.SNLScalarTerm.create(ramb36e1, naja.SNLTerm.Direction.Output, "SBITERR")
    a_outputs.append(sbiterr)  # not sure about which kind of modeling to put here.
    wea = naja.SNLBusTerm.create(ramb36e1, naja.SNLTerm.Direction.Input, 3, 0, "WEA")
    a_inputs.extend(list(wea.getBits()))
    webwe = naja.SNLBusTerm.create(ramb36e1, naja.SNLTerm.Direction.Input, 7, 0, "WEBWE")
    b_inputs.extend(list(webwe.getBits()))
    naja.SNLDesign.addInputsToClockArcs(a_inputs, clka)
    naja.SNLDesign.addInputsToClockArcs(b_inputs, clkb)
    naja.SNLDesign.addClockToOutputsArcs(clka, a_outputs)
    naja.SNLDesign.addClockToOutputsArcs(clkb, b_outputs)
    utils.setTermRole(a_inputs + b_inputs, naja.SNLTermRole.DataInput)
    utils.setTermRole(a_outputs + b_outputs, naja.SNLTermRole.DataOutput)
    utils.setTermRole([clka, clkb], naja.SNLTermRole.Clock)
    utils.setTermRole([addra, addrb], naja.SNLTermRole.MemoryReadAddress)
    utils.setTermRole(
        [diadi, dibdi, dipadip, dipbdip], naja.SNLTermRole.MemoryWriteData
    )
    utils.setTermRole(
        [doado, dobdo, dopadop, dopbdop], naja.SNLTermRole.MemoryReadData
    )
    utils.setTermRole(
        [enarden, enbwren, regcearegce, regceb], naja.SNLTermRole.Enable
    )
    utils.setTermRole(
        [rstramarstram, rstramb, rstregarstreg, rstregb],
        naja.SNLTermRole.SyncReset,
        naja.SNLActiveLevel.High,
    )
    utils.setTermRole([wea, webwe], naja.SNLTermRole.MemoryWriteEnable)
    naja.SNLParameter.createDecimal(ramb36e1, "DOA_REG", 0)
    naja.SNLParameter.createDecimal(ramb36e1, "DOB_REG", 0)
    naja.SNLParameter.createBinary(ramb36e1, "INIT_A", 36, 0)
    naja.SNLParameter.createBinary(ramb36e1, "INIT_B", 36, 0)
    for i in range(128):
        paramName = "INIT_" + hex(i)[2:].zfill(2).upper()
        naja.SNLParameter.createBinary(ramb36e1, paramName, 256, 0)
    naja.SNLParameter.createString(ramb36e1, "RAM_EXTENSION_A", "NONE")
    naja.SNLParameter.createString(ramb36e1, "RAM_EXTENSION_B", "NONE")
    for i in range(16):
        paramName = "INITP_" + hex(i)[2:].zfill(2).upper()
        naja.SNLParameter.createBinary(ramb36e1, paramName, 256, 0)
    naja.SNLParameter.createString(ramb36e1, "RAM_MODE", "TDP")
    naja.SNLParameter.createDecimal(ramb36e1, "READ_WIDTH_A", 0)
    naja.SNLParameter.createDecimal(ramb36e1, "READ_WIDTH_B", 0)
    naja.SNLParameter.createDecimal(ramb36e1, "WRITE_WIDTH_A", 0)
    naja.SNLParameter.createDecimal(ramb36e1, "WRITE_WIDTH_B", 0)
    naja.SNLParameter.createBinary(ramb36e1, "SRVAL_A", 36, 0)
    naja.SNLParameter.createBinary(ramb36e1, "SRVAL_B", 36, 0)
    naja.SNLParameter.createString(ramb36e1, "WRITE_MODE_A", "WRITE_FIRST")
    naja.SNLParameter.createString(ramb36e1, "WRITE_MODE_B", "WRITE_FIRST")


def constructPrimitives(lib):
    constructIBUF(lib)
    constructOBUF(lib)
    constructBUFG(lib)
    constructDSP48E1(lib)
    constructINV(lib)
    constructCARRY4(lib)
    constructLUT1(lib)
    constructLUT2(lib)
    constructLUT3(lib)
    constructLUT4(lib)
    constructLUT5(lib)
    constructLUT6(lib)
    constructMUXF7(lib)
    constructMUXF8(lib)
    constructSRL16E(lib)
    constructFDCE(lib)
    constructFDPE(lib)
    constructFDRE(lib)
    constructFDSE(lib)
    constructRAM32M(lib)
    constructRAM64M(lib)
    constructRAMB18E1(lib)
    constructRAMB36E1(lib)


def load(db):
    logger.info("Loading Xilinx primitives")
    lib = naja.NLLibrary.createPrimitives(db, "xilinx")
    constructPrimitives(lib)
