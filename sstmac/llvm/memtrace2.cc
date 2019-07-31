#include "llvm/ADT/StringMap.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include <algorithm>
#include <map>
#include <string>
#include <vector>

using namespace llvm;

namespace {

SmallVector<int, 5> getLines(StringRef const &S);
std::map<DIFile *, SmallVector<int, 5>> parseAnnotations(Module &M);
StringMap<Function *> addFunctionDeclsToModule(Module &M);
StringMap<StructType*> getStructTypes(Module &M);

enum class MemoryAccessType { Load, Store, MemIntrinsic, Call, Invoke };

struct MemtracePass : public ModulePass {
  std::map<DIFile *, SmallVector<int, 5>> AnnotFuncs;
  static char ID;
  StringMap<Function *> SSTFunctions;
  StringMap<StructType*> NamedTypes;

  MemtracePass() : ModulePass(ID) {}

  bool instMatchesLine(Instruction const &I) {
    if (DILocation *Dl = I.getDebugLoc()) {
      auto const &Pair = AnnotFuncs.find(Dl->getFile());
      if (Pair != AnnotFuncs.end()) {
        auto const &Lines = Pair->second;
        return std::count(Lines.begin(), Lines.end(), Dl->getLine()) > 0;
      }
    }

    return false;
  }

  SmallVector<Instruction *, 10> getTaggedInsts(Function &F) {
    SmallVector<Instruction *, 10> Insts;
    for (auto &I : instructions(F)) {
      if (instMatchesLine(I)) {
        Insts.push_back(&I);
      }
    }

    return Insts;
  }

  void handleMemReadOrWrite(LoadInst *LD, Value *ThreadID) {
    auto &Ctx = LD->getContext();
    auto LoadFunc = SSTFunctions["Load"];

    auto Ptr = LD->getPointerOperand();
    auto NumAddrsLoaded = [&LD] {
      if (auto VT = dyn_cast<VectorType>(LD->getType())) {
        return VT->getNumElements();
      } else {
        return uint64_t(1);
      }
    }();

    auto Bcast = new BitCastInst(Ptr, Type::getInt8PtrTy(Ctx), "", LD);
    auto LoadSize = ConstantInt::get(Ctx, APInt(64, NumAddrsLoaded, false));
    auto Call = CallInst::Create(LoadFunc->getFunctionType(), LoadFunc,
                                 {Bcast, LoadSize, ThreadID}, "", LD);
  }

  void handleMemReadOrWrite(StoreInst *SD, Value *ThreadID) {
    auto &Ctx = SD->getContext();
    auto StoreFunc = SSTFunctions["Store"];

    auto Ptr = SD->getPointerOperand();
    auto NumAddrsStored = [&SD] {
      if (auto VT = dyn_cast<VectorType>(SD->getType())) {
        return VT->getNumElements();
      } else {
        return uint64_t(1);
      }
    }();

    auto Bcast = new BitCastInst(Ptr, Type::getInt8PtrTy(Ctx), "", SD);
    auto StoreSize = ConstantInt::get(Ctx, APInt(64, NumAddrsStored, false));
    auto Call = CallInst::Create(StoreFunc->getFunctionType(), StoreFunc,
                                 {Bcast, StoreSize, ThreadID}, "", SD);
  }

  Value *createOrFindOMPThreadID(Function &F) {
    if (!F.getName().contains(".omp")) {
      return ConstantInt::get(F.getContext(), APInt(32, 0, false));
    }

    errs() << "\tFound omp func: " << F.getName() << "\n";
    Instruction *Ident = nullptr;
    for (auto &I : instructions(F)) {
      if (auto Alloc = dyn_cast<AllocaInst>(&I)) {
        if(Alloc->getType() == dyn_cast<PointerType>(NamedTypes["OMP_ID"])){
          errs() << "\t\t" << *Alloc << "\n";
        }
      }
    }

    return ConstantInt::get(F.getContext(), APInt(32, 0, false));
  }

  void handleMemReadOrWrite(MemIntrinsic const *Mi, Value *ThreadID) {}

  void runOnFunction(Function &F) {
    auto taggedInsts = getTaggedInsts(F);
    if (!taggedInsts.empty()) {
      errs() << "\n" << F.getName() << "\n";
    }

    auto ThreadID = createOrFindOMPThreadID(F);

    for (auto I : getTaggedInsts(F)) {
      if (!I->mayReadOrWriteMemory()) {
        continue;
      }

      switch (I->getOpcode()) {
      case Instruction::Load:
        handleMemReadOrWrite(dyn_cast<LoadInst>(I), ThreadID);
        break;
      case Instruction::Store:
        handleMemReadOrWrite(dyn_cast<StoreInst>(I), ThreadID);
        break;
      default:
        // Can't get MemIntrisic from Opcode
        if (auto Mi = dyn_cast<MemIntrinsic>(I)) {
          handleMemReadOrWrite(Mi, ThreadID);
        }
        break;
      }
    }
  }

  bool runOnModule(Module &M) override {
    NamedTypes = getStructTypes(M);
    AnnotFuncs = parseAnnotations(M);
    if (AnnotFuncs.empty()) {
      return false;
    }

    SSTFunctions = addFunctionDeclsToModule(M);
    for (Function &F : M.functions()) {
      runOnFunction(F);

      // Hack in a function call at the end of main for a bit
      if (F.getName() == "main") {
        for (auto &I : instructions(F)) {
          if (auto Ret = dyn_cast<ReturnInst>(&I)) {
            auto Dump = SSTFunctions["Dump"];
            CallInst::Create(Dump->getFunctionType(), Dump, "", Ret);
          }
        }
      }
    }

    return true;
  }
};

SmallVector<int, 5> getLines(StringRef const &S) {
  auto NumStart = S.find_first_of("{") + 1;
  auto NumEnd = S.find_first_of("}");
  auto Temp = StringRef(S.data() + NumStart, NumEnd - NumStart);

  SmallVector<StringRef, 5> Nums;
  Temp.split(Nums, ",");

  SmallVector<int, 5> Out;
  for (auto const &Num : Nums) {
    Out.push_back(std::stoi(Num));
  }

  return Out;
}

std::map<DIFile *, SmallVector<int, 5>> parseAnnotations(Module &M) {
  std::map<DIFile *, SmallVector<int, 5>> FileLines;
  for (auto const &I : M.globals()) {
    if (I.getName() == "llvm.global.annotations") {
      ConstantArray *CA = dyn_cast<ConstantArray>(I.getOperand(0));

      for (auto OI = CA->op_begin(), End = CA->op_end(); OI != End; ++OI) {
        ConstantStruct *CS = dyn_cast<ConstantStruct>(OI->get());
        Function *FUNC = dyn_cast<Function>(CS->getOperand(0)->getOperand(0));

        GlobalVariable *AnnotationGL =
            dyn_cast<GlobalVariable>(CS->getOperand(1)->getOperand(0));

        StringRef Annotation =
            dyn_cast<ConstantDataArray>(AnnotationGL->getInitializer())
                ->getAsCString();

        if (Annotation.contains("memtrace")) {
          FileLines[FUNC->getSubprogram()->getFile()] = getLines(Annotation);
        }
      }
    }
  }

  return FileLines;
}

StringMap<StructType*> getStructTypes(Module &M) {
  StringMap<StructType*> StructTypes;

  for(auto &T : M.getIdentifiedStructTypes()){
    errs() << *T << "\n";
    if(T->getName().contains("ident_t")){
      StructTypes["OMP_ID"] = T;
    }
  }

  for(auto &T : StructTypes){
    errs() << "Found: " << *T.second << "\n";
  }
    

  return StructTypes;
}

StringMap<Function *> addFunctionDeclsToModule(Module &M) {
  StringMap<Function *> Funcs;

  { // Add SST Functions
    auto IntPtrType8 = Type::getInt8PtrTy(M.getContext());
    auto IntType32 = Type::getInt32Ty(M.getContext());
    auto IntType64 = Type::getInt64Ty(M.getContext());
    auto VoidType = Type::getVoidTy(M.getContext());
    auto AddrTrack =
        FunctionType::get(VoidType, {IntPtrType8, IntType64, IntType32}, false);
    auto Load = Function::Create(AddrTrack, Function::ExternalLinkage,
                                 "sstmac_address_load", M);
    Funcs["Load"] = Load;
    auto Store = Function::Create(AddrTrack, Function::ExternalLinkage,
                                  "sstmac_address_store", M);
    Funcs["Store"] = Store;

    auto AddrsInfoDump = FunctionType::get(VoidType, {}, false);
    auto InfoDump = Function::Create(AddrsInfoDump, Function::ExternalLinkage,
                                     "sstmac_print_address_info", M);
    Funcs["Dump"] = InfoDump;
  }
  return Funcs;
}

} // namespace

char MemtracePass::ID = 0;
static RegisterPass<MemtracePass> X("sst-memtrace", "SSTMAC Memtrace Pass");
