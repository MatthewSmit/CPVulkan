//===- SPIRVToLLVMDbgTran.h - Converts SPIR-V DebugInfo to LLVM -*- C++ -*-===//
//
//                     The LLVM/SPIR-V Translator
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
// Copyright (c) 2018 Intel Corporation. All rights reserved.
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
// Neither the names of Intel Corporation, nor the names of its
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
//
// This file implements translation of debug info from SPIR-V to LLVM metadata
//
//===----------------------------------------------------------------------===//

#ifndef SPIRVTOLLVMDBGTRAN_H
#define SPIRVTOLLVMDBGTRAN_H

#include "SPIRVInstruction.h"
#include "SPIRVModule.h"

#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/DebugLoc.h"

#include <unordered_map>

namespace llvm {
class Module;
class Value;
class Instruction;
class Type;
} // namespace llvm
using namespace llvm;

namespace SPIRV {
class SPIRVToLLVM;
class SPIRVEntry;
class SPIRVFunction;
class SPIRVValue;

class SPIRVToLLVMDbgTran {
public:
  typedef std::vector<SPIRVWord> SPIRVWordVec;

  SPIRVToLLVMDbgTran(SPIRVModule *TBM, Module *TM, SPIRVToLLVM *Reader);
  void addDbgInfoVersion();
  void transDbgInfo(const SPIRVValue *SV, Value *V);
  template <typename T = MDNode>
  T *transDebugInst(const SPIRVExtInst *DebugInst) {
    assert(DebugInst->getExtSetKind() == SPIRVEIS_Debug &&
           "Unexpected extended instruction set");
    auto It = DebugInstCache.find(DebugInst);
    if (It != DebugInstCache.end())
      return static_cast<T *>(It->second);
    MDNode *Res = transDebugInstImpl(DebugInst);
    DebugInstCache[DebugInst] = Res;
    return static_cast<T *>(Res);
  }
  Instruction *transDebugIntrinsic(const SPIRVExtInst *DebugInst,
                                   BasicBlock *BB);
  void finalize();

private:
  DIFile *getFile(const SPIRVId SourceId);
  DIFile *getDIFile(const std::string &FileName);
  DIFile *getDIFile(const SPIRVEntry *E);
  unsigned getLineNo(const SPIRVEntry *E);

  MDNode *transDebugInstImpl(const SPIRVExtInst *DebugInst);

  llvm::DebugLoc transDebugLocation(const SPIRVExtInst *DebugInst);

  llvm::DebugLoc transDebugScope(const SPIRVInstruction *Inst);

  MDNode *transDebugInlined(const SPIRVExtInst *Inst);

  DICompileUnit *transCompileUnit(const SPIRVExtInst *DebugInst);

  DIBasicType *transTypeBasic(const SPIRVExtInst *DebugInst);

  DIDerivedType *transTypeQualifier(const SPIRVExtInst *DebugInst);

  DIType *transTypePointer(const SPIRVExtInst *DebugInst);

  DICompositeType *transTypeArray(const SPIRVExtInst *DebugInst);

  DICompositeType *transTypeVector(const SPIRVExtInst *DebugInst);

  DICompositeType *transTypeComposite(const SPIRVExtInst *DebugInst);

  DINode *transTypeMember(const SPIRVExtInst *DebugInst);

  DINode *transTypeEnum(const SPIRVExtInst *DebugInst);

  DINode *transTemplateParameter(const SPIRVExtInst *DebugInst);
  DINode *transTemplateTemplateParameter(const SPIRVExtInst *DebugInst);
  DINode *transTemplateParameterPack(const SPIRVExtInst *DebugInst);

  MDNode *transTemplate(const SPIRVExtInst *DebugInst);

  DINode *transTypeFunction(const SPIRVExtInst *DebugInst);

  DINode *transTypePtrToMember(const SPIRVExtInst *DebugInst);

  DINode *transLexicalBlock(const SPIRVExtInst *DebugInst);
  DINode *transLexicalBlockDiscriminator(const SPIRVExtInst *DebugInst);

  DINode *transFunction(const SPIRVExtInst *DebugInst);

  DINode *transFunctionDecl(const SPIRVExtInst *DebugInst);

  MDNode *transGlobalVariable(const SPIRVExtInst *DebugInst);

  DINode *transLocalVariable(const SPIRVExtInst *DebugInst);

  DINode *transTypedef(const SPIRVExtInst *DebugInst);

  DINode *transInheritance(const SPIRVExtInst *DebugInst);

  DINode *transImportedEntry(const SPIRVExtInst *DebugInst);

  MDNode *transExpression(const SPIRVExtInst *DebugInst);

  SPIRVModule *BM;
  Module *M;
  DIBuilder Builder;
  SPIRVToLLVM *SPIRVReader;
  DICompileUnit *CU;
  bool Enable;
  std::unordered_map<std::string, DIFile *> FileMap;
  std::unordered_map<SPIRVId, DISubprogram *> FuncMap;
  std::unordered_map<const SPIRVExtInst *, MDNode *> DebugInstCache;

  struct SplitFileName {
    SplitFileName(const std::string &FileName);
    std::string BaseName;
    std::string Path;
  };

  DIScope *getScope(const SPIRVEntry *ScopeInst);
  SPIRVExtInst *getDbgInst(const SPIRVId Id);

  template <SPIRVWord OpCode> SPIRVExtInst *getDbgInst(const SPIRVId Id) {
    if (SPIRVExtInst *DI = getDbgInst(Id)) {
      if (DI->getExtOp() == OpCode) {
        return DI;
      }
    }
    return nullptr;
  }
  StringRef getString(const SPIRVId Id);
};
} // namespace SPIRV

using namespace llvm;

namespace SPIRV {
typedef SPIRVMap<dwarf::TypeKind, SPIRVDebug::EncodingTag> DbgEncodingMap;
template <>
inline void DbgEncodingMap::init() {
  add(static_cast<dwarf::TypeKind>(0), SPIRVDebug::Unspecified);
  add(dwarf::DW_ATE_address,           SPIRVDebug::Address);
  add(dwarf::DW_ATE_boolean,           SPIRVDebug::Boolean);
  add(dwarf::DW_ATE_float,             SPIRVDebug::Float);
  add(dwarf::DW_ATE_signed,            SPIRVDebug::Signed);
  add(dwarf::DW_ATE_signed_char,       SPIRVDebug::SignedChar);
  add(dwarf::DW_ATE_unsigned,          SPIRVDebug::Unsigned);
  add(dwarf::DW_ATE_unsigned_char,     SPIRVDebug::UnsignedChar);
}

typedef SPIRVMap<dwarf::Tag, SPIRVDebug::TypeQualifierTag> DbgTypeQulifierMap;
template <>
inline void DbgTypeQulifierMap::init() {
  add(dwarf::DW_TAG_const_type,    SPIRVDebug::ConstType);
  add(dwarf::DW_TAG_volatile_type, SPIRVDebug::VolatileType);
  add(dwarf::DW_TAG_restrict_type, SPIRVDebug::RestrictType);
  add(dwarf::DW_TAG_atomic_type,   SPIRVDebug::AtomicType);
}

typedef SPIRVMap<dwarf::Tag, SPIRVDebug::CompositeTypeTag> DbgCompositeTypeMap;
template <>
inline void DbgCompositeTypeMap::init() {
  add(dwarf::DW_TAG_class_type,     SPIRVDebug::Class);
  add(dwarf::DW_TAG_structure_type, SPIRVDebug::Structure);
  add(dwarf::DW_TAG_union_type,     SPIRVDebug::Union);
}

typedef SPIRVMap<dwarf::LocationAtom, SPIRVDebug::ExpressionOpCode>
  DbgExpressionOpCodeMap;
template <>
inline void DbgExpressionOpCodeMap::init() {
  add(dwarf::DW_OP_deref,         SPIRVDebug::Deref);
  add(dwarf::DW_OP_plus,          SPIRVDebug::Plus);
  add(dwarf::DW_OP_minus,         SPIRVDebug::Minus);
  add(dwarf::DW_OP_plus_uconst,   SPIRVDebug::PlusUconst);
  add(dwarf::DW_OP_bit_piece,     SPIRVDebug::BitPiece);
  add(dwarf::DW_OP_swap,          SPIRVDebug::Swap);
  add(dwarf::DW_OP_xderef,        SPIRVDebug::Xderef);
  add(dwarf::DW_OP_stack_value,   SPIRVDebug::StackValue);
  add(dwarf::DW_OP_constu,        SPIRVDebug::Constu);
  add(dwarf::DW_OP_LLVM_fragment, SPIRVDebug::Fragment);
}

typedef SPIRVMap<dwarf::Tag, SPIRVDebug::ImportedEntityTag>
  DbgImportedEntityMap;
template <>
inline void DbgImportedEntityMap::init() {
  add(dwarf::DW_TAG_imported_module,      SPIRVDebug::ImportedModule);
  add(dwarf::DW_TAG_imported_declaration, SPIRVDebug::ImportedDeclaration);
}

} // namespace SPIRV

#endif // SPIRVTOLLVMDBGTRAN_H
