/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "SVMISelLowering.h"
#include "SVMTargetMachine.h"
#include "SVMMCTargetDesc.h"
#include "SVMMachineFunctionInfo.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Function.h"
#include "llvm/Module.h"
#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/ADT/VectorExtras.h"
#include "llvm/Support/ErrorHandling.h"
using namespace llvm;

#include "SVMGenCallingConv.inc"


SVMTargetLowering::SVMTargetLowering(TargetMachine &TM)
    : TargetLowering(TM, new TargetLoweringObjectFileELF())
{
    // Register classes
    addRegisterClass(MVT::i32, SVM::GPRegRegisterClass);

    // Branches
    setOperationAction(ISD::BRCOND, MVT::Other, Expand);
    setOperationAction(ISD::BRIND, MVT::Other, Expand);
    setOperationAction(ISD::BR_JT, MVT::Other, Expand);
    setOperationAction(ISD::BR_CC, MVT::i32, Custom);

    computeRegisterProperties();
}

const char *SVMTargetLowering::getTargetNodeName(unsigned Opcode) const
{
    switch (Opcode) {
    default: return 0;
    case SVMISD::CALL:          return "SVMISD::CALL";
    case SVMISD::RETURN:        return "SVMISD::RETURN";
    }
}

SDValue SVMTargetLowering::LowerFormalArguments(SDValue Chain,
                                                CallingConv::ID CallConv,
                                                bool isVarArg,
                                                const SmallVectorImpl<ISD::InputArg> &Ins,
                                                DebugLoc dl, SelectionDAG &DAG,
                                                SmallVectorImpl<SDValue> &InVals) const
{
    return Chain;
}

SDValue SVMTargetLowering::LowerCall(SDValue Chain, SDValue Callee,
                                     CallingConv::ID CallConv, bool isVarArg,
                                     bool &isTailCall,
                                     const SmallVectorImpl<ISD::OutputArg> &Outs,
                                     const SmallVectorImpl<SDValue> &OutVals,
                                     const SmallVectorImpl<ISD::InputArg> &Ins,
                                     DebugLoc dl, SelectionDAG &DAG,
                                     SmallVectorImpl<SDValue> &InVals) const
{
    // Let the calling convention assign locations to each operand
    SmallVector<CCValAssign, 16> ArgLocs;
    CCState CCInfo(CallConv, isVarArg, DAG.getMachineFunction(),
        DAG.getTarget(), ArgLocs, *DAG.getContext());
    CCInfo.AnalyzeCallOperands(Outs, CC_SVM);
    
    // How much stack space do we require?
    unsigned ArgsStackSize = CCInfo.getNextStackOffset();

    // For now, assume 4-byte alignment
    ArgsStackSize = (ArgsStackSize + 3) & ~3;

    // Start calling sequence
    Chain = DAG.getCALLSEQ_START(Chain,
        DAG.getIntPtrConstant(ArgsStackSize, true));

    // Resolve address of callee
    // XXX: Is this where we should handle syscalls?
    if (GlobalAddressSDNode *G = dyn_cast<GlobalAddressSDNode>(Callee))
        Callee = DAG.getTargetGlobalAddress(G->getGlobal(), dl, MVT::i32);

    std::vector<SDValue> Ops;
    Ops.push_back(Chain);
    Ops.push_back(Callee);

    // Copy input parameters
    SDValue InFlag;
    for (unsigned i = 0, end = ArgLocs.size(); i != end; i++) {
        CCValAssign &VA = ArgLocs[i];
        ISD::ArgFlagsTy Flags = Outs[i].Flags;
        SDValue Arg = OutVals[i];
        
        // Optional type promotion
        switch (VA.getLocInfo()) {
        default: llvm_unreachable("Unsupported parameter loc");
        case CCValAssign::Full: break;
        case CCValAssign::SExt:
            Arg = DAG.getNode(ISD::SIGN_EXTEND, dl, VA.getLocVT(), Arg);
            break;
        case CCValAssign::ZExt:
            Arg = DAG.getNode(ISD::ZERO_EXTEND, dl, VA.getLocVT(), Arg);
            break;
        case CCValAssign::AExt:
            Arg = DAG.getNode(ISD::ANY_EXTEND, dl, VA.getLocVT(), Arg);
            break;
        case CCValAssign::BCvt:
            Arg = DAG.getNode(ISD::BITCAST, dl, VA.getLocVT(), Arg);
            break;
        }
        
        if (VA.isRegLoc()) {
            // Passed by register
            
            unsigned Reg = VA.getLocReg();
            Chain = DAG.getCopyToReg(Chain, dl, Reg, Arg, InFlag);
            InFlag = Chain.getValue(1);

            // Reference this register, so it doesn't get optimized out
            Ops.push_back(DAG.getRegister(Reg, Arg.getValueType()));
            
        } else {
            llvm_unreachable("Non-register parameters not yet supported");
        }
    }
    
    if (InFlag.getNode())
      Ops.push_back(InFlag);

    SDVTList NodeTys = DAG.getVTList(MVT::Other, MVT::Glue);
    Chain = DAG.getNode(SVMISD::CALL, dl, NodeTys, &Ops[0], Ops.size());
    InFlag = Chain.getValue(1);

    Chain = DAG.getCALLSEQ_END(Chain, DAG.getIntPtrConstant(0, true),
        DAG.getIntPtrConstant(0, true), InFlag);

    return Chain;
}

SDValue SVMTargetLowering::LowerReturn(SDValue Chain,
                                       CallingConv::ID CallConv, bool isVarArg,
                                       const SmallVectorImpl<ISD::OutputArg> &Outs,
                                       const SmallVectorImpl<SDValue> &OutVals,
                                       DebugLoc dl, SelectionDAG &DAG) const
{
    return DAG.getNode(SVMISD::RETURN, dl, MVT::Other, Chain);
}

static SDValue LowerBR_CC(SDValue Op, SelectionDAG &DAG)
{
    SDValue Chain = Op.getOperand(0);
    ISD::CondCode CC = cast<CondCodeSDNode>(Op.getOperand(1))->get();
    SDValue LHS = Op.getOperand(2);
    SDValue RHS = Op.getOperand(3);
    SDValue Dest = Op.getOperand(4);
    DebugLoc dl = Op.getDebugLoc();

    SVMCC::CondCodes sCC;
    switch (CC) {
    default: llvm_unreachable("Unknown condition code!");
    case ISD::SETNE:  sCC = SVMCC::NE;
    case ISD::SETEQ:  sCC = SVMCC::EQ;
    case ISD::SETGT:  sCC = SVMCC::GT;
    case ISD::SETGE:  sCC = SVMCC::GE;
    case ISD::SETLT:  sCC = SVMCC::LT;
    case ISD::SETLE:  sCC = SVMCC::LE;
    case ISD::SETUGT: sCC = SVMCC::HI;
    case ISD::SETUGE: sCC = SVMCC::HS;
    case ISD::SETULT: sCC = SVMCC::LO;
    case ISD::SETULE: sCC = SVMCC::LS;
    }
    
    SDValue Cmp = DAG.getNode(SVMISD::CMP, dl, MVT::Glue, LHS, RHS);
    SDValue CCR = DAG.getRegister(SVM::CPSR, MVT::i32);
    return DAG.getNode(SVMISD::BRCOND, dl, MVT::Other,
        Chain, Dest, DAG.getConstant(sCC, MVT::i32), CCR, Cmp);
}

SDValue SVMTargetLowering::LowerOperation(SDValue Op, SelectionDAG &DAG) const
{
    switch (Op.getOpcode()) {
    default: llvm_unreachable("Should not custom lower this!");
    case ISD::BR_CC:         return LowerBR_CC(Op, DAG);
    }
}