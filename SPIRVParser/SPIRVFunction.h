//===- SPIRVFunction.h - Class to represent a SPIR-V function ---*- C++ -*-===//
//
//                     The LLVM/SPIRV Translator
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines Function class for SPIRV.
//
// Copyright (c) 2014 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal with the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimers.
// Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimers in the documentation
// and/or other materials provided with the distribution.
// Neither the names of Advanced Micro Devices, Inc., nor the names of its
// contributors may be used to endorse or promote products derived from this
// Software without specific prior written permission.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH
// THE SOFTWARE.
//
//===----------------------------------------------------------------------===//

#pragma once

#include "SPIRVBasicBlock.h"
#include "SPIRVValue.h"
#include <functional>

namespace SPIRV
{
	class BIFunction;
	class SPIRVDecoder;

	class DLL_EXPORT SPIRVFunctionParameter : public SPIRVValue
	{
	public:
		SPIRVFunctionParameter(SPIRVType* TheType, SPIRVId TheId,
			SPIRVFunction* TheParent, unsigned TheArgNo);
		SPIRVFunctionParameter()
			: SPIRVValue(OpFunctionParameter), ParentFunc(nullptr), ArgNo(0) {}
		void setArgNo(uint32_t value) { ArgNo = value; }
		uint32_t getArgNo() const { return ArgNo; }
		void foreachAttr(std::function<void(SPIRVFuncParamAttrKind)>);
		void addAttr(SPIRVFuncParamAttrKind Kind) {
			addDecorate(new SPIRVDecorate(DecorationFuncParamAttr, this, Kind));
		}
		void setParent(SPIRVFunction* Parent) { ParentFunc = Parent; }
		bool hasAttr(SPIRVFuncParamAttrKind Kind) const {
			return getDecorate(DecorationFuncParamAttr).count(Kind);
		}
		bool isByVal() const { return hasAttr(FunctionParameterAttributeByVal); }
		bool isZext() const { return hasAttr(FunctionParameterAttributeZext); }
		SPIRVCapVec getRequiredCapability() const override {
			if (hasLinkageType() && getLinkageType() == LinkageTypeImport)
				return getVec(CapabilityLinkage);
			return SPIRVCapVec();
		}

	protected:
		void validate() const override {
			SPIRVValue::validate();
			assert(ParentFunc && "Invalid parent function");
		}
		_SPIRV_DEF_ENCDEC2(Type, Id)
		
	private:
		SPIRVFunction* ParentFunc;
		uint32_t ArgNo;
	};

	class DLL_EXPORT SPIRVFunction : public SPIRVValue, public SPIRVComponentExecutionModes
	{
	public:
		// Complete constructor. It does not construct basic blocks.
		SPIRVFunction(SPIRVModule* M, SPIRVTypeFunction* FunctionType, SPIRVId TheId)
			: SPIRVValue(M, 5, OpFunction, FunctionType->getReturnType(), TheId),
			FuncType(FunctionType), FCtrlMask(FunctionControlMaskNone) {
			addAllArguments(TheId + 1);
			validate();
		}

		// Incomplete constructor
		SPIRVFunction()
			: SPIRVValue(OpFunction), FuncType(NULL),
			FCtrlMask(FunctionControlMaskNone) {}

		SPIRVDecoder getDecoder(std::istream& IS) override;
		SPIRVTypeFunction* getFunctionType() const { return FuncType; }
		SPIRVWord getFuncCtlMask() const { return FCtrlMask; }
		size_t getNumBasicBlock() const { return BBVec.size(); }
		SPIRVBasicBlock* getBasicBlock(size_t I) const { return BBVec[I]; }
		size_t getNumArguments() const {
			return getFunctionType()->getNumParameters();
		}
		SPIRVId getArgumentId(size_t I) const { return Parameters[I]->getId(); }
		const std::vector<SPIRVId> getVariables() const {
			std::vector<SPIRVId> Ids;
			for (auto Variable : Variables)
				Ids.push_back(Variable->getId());
			return Ids;
		}
		void addVariable(const SPIRVValue* Variable) {
			Variables.push_back(Variable);
		}
		SPIRVFunctionParameter* getArgument(size_t I) const { return Parameters[I]; }
		void foreachArgument(std::function<void(SPIRVFunctionParameter*)> Func) {
			for (size_t I = 0, E = getNumArguments(); I != E; ++I)
				Func(getArgument(I));
		}

		void foreachReturnValueAttr(std::function<void(SPIRVFuncParamAttrKind)>);

		void setFunctionControlMask(SPIRVWord Mask) { FCtrlMask = Mask; }

		void takeExecutionModes(SPIRVForward* Forward) {
			ExecModes = std::move(Forward->ExecModes);
		}

		// Assume BB contains valid Id.
		SPIRVBasicBlock* addBasicBlock(SPIRVBasicBlock* BB) {
			Module->add(BB);
			BB->setParent(this);
			BBVec.push_back(BB);
			return BB;
		}

		void encodeChildren(spv_ostream&) const override;
		void encodeExecutionModes(spv_ostream&) const;
		_SPIRV_DCL_ENCDEC
			void validate() const override {
			SPIRVValue::validate();
			validateFunctionControlMask(FCtrlMask);
			assert(FuncType && "Invalid func type");
		}

		bool shouldFPContractBeDisabled() const {
			// We could find some instructions in LLVM IR which look exactly like an
			// unfused fmuladd. So, we assume that FP_CONTRACT was disabled only if
			// there are something that looks like uncointracted fmul + fadd, but there
			// no calls to @llvm.fmuladd.*
			return FoundUncontractedFMulAdd && !FoundContractedFMulAdd;
		}

		void setUncontractedFMulAddFound(bool Value = true) {
			FoundUncontractedFMulAdd = Value;
		}

		void setContractedFMulAddFound(bool Value = true) {
			FoundContractedFMulAdd = Value;
		}

	private:
		SPIRVFunctionParameter* addArgument(unsigned TheArgNo, SPIRVId TheId) {
			SPIRVFunctionParameter* Arg = new SPIRVFunctionParameter(
				getFunctionType()->getParameterType(TheArgNo), TheId, this, TheArgNo);
			Module->add(Arg);
			Parameters.push_back(Arg);
			return Arg;
		}

		void addAllArguments(SPIRVId FirstArgId) {
			for (size_t I = 0, E = getFunctionType()->getNumParameters(); I != E; ++I)
				addArgument(I, FirstArgId + I);
		}
		bool decodeBB(SPIRVDecoder&);

		SPIRVTypeFunction* FuncType; // Function type
		SPIRVWord FCtrlMask;         // Function control mask

		std::vector<SPIRVFunctionParameter*> Parameters;
		std::vector<const SPIRVValue*> Variables;
		typedef std::vector<SPIRVBasicBlock*> SPIRVLBasicBlockVector;
		SPIRVLBasicBlockVector BBVec;

		bool FoundUncontractedFMulAdd = false;
		bool FoundContractedFMulAdd = false;
	};

	typedef SPIRVEntryOpCodeOnly<OpFunctionEnd> SPIRVFunctionEnd;
}