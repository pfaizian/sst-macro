#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringSet.h"
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
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include <algorithm>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "memtraceTools.h"

using namespace llvm;

namespace {
StringSet<> FunctionCallWhiteList = {};

bool functionIsWhiteListed(StringRef const &Str) {
  if (Str.startswith("__kmpc")) { // Whitelist builtin OMP Funcs
    return true;
  }
  return FunctionCallWhiteList.count(Str);
}

// Helper function for getting the size of loads and stores
template <typename MemInst> uint64_t getMemSize(MemInst *I) {
  if (auto VT = llvm::dyn_cast<llvm::VectorType>(I->getType())) {
    return VT->getNumElements();
  } else {
    return 1;
  }
}

struct MemtracePass : public ModulePass {
  AnnotationMap AnnotFuncs;
  StringMap<Function *> ExternalFunctions;
  static char ID;
  enum class MemoryAccessType { Load, Store, MemIntrinsic, Call, Invoke };

  MemtracePass() : ModulePass(ID) {}

  SmallVector<Instruction *, 10> getTaggedInsts(Function &F) {
    SmallVector<Instruction *, 10> Insts;
    Instruction *Ret = nullptr;
    for (auto &I : instructions(F)) {
      auto AK = AnnotFuncs.matchInst(&I);
      if (AK && AK.getValue() != AnnotationKind::Ignore) {
        if (I.mayReadOrWriteMemory() || I.getOpcode() == Instruction::Ret) {
          Insts.push_back(&I);
        }
      } else if (I.getOpcode() == Instruction::Ret) {
        // If we didn't already capture the return then capture it now, but we
        // only want to catpure it if there is at least 1 instrumented
        // instruction
        Ret = &I;
      }
    }

    if (!Insts.empty() && Ret != nullptr) {
      Insts.push_back(Ret);
    }

    return Insts;
  }

  void handleMemReadOrWrite(LoadInst *LD, Value *ThreadID) {
    auto Ptr = LD->getPointerOperand();
    auto NumAddrsLoaded = getMemSize(LD);

    auto &Ctx = LD->getContext();
    auto LoadFunc = ExternalFunctions["Load"];

    auto Bcast = new BitCastInst(Ptr, Type::getInt8PtrTy(Ctx), "", LD);
    auto LoadSize = ConstantInt::get(Ctx, APInt(64, NumAddrsLoaded, false));
    auto Call = CallInst::Create(LoadFunc->getFunctionType(), LoadFunc,
                                 {Bcast, LoadSize, ThreadID}, "", LD);
  }

  void handleMemReadOrWrite(StoreInst *SD, Value *ThreadID) {

    auto Ptr = SD->getPointerOperand();
    auto NumAddrsStored = getMemSize(SD);

    auto &Ctx = SD->getContext();
    auto StoreFunc = ExternalFunctions["Store"];

    auto Bcast = new BitCastInst(Ptr, Type::getInt8PtrTy(Ctx), "", SD);
    auto StoreSize = ConstantInt::get(Ctx, APInt(64, NumAddrsStored, false));
    auto Call = CallInst::Create(StoreFunc->getFunctionType(), StoreFunc,
                                 {Bcast, StoreSize, ThreadID}, "", SD);
  }

  void handleMemReadOrWrite(MemIntrinsic *Mi, Value *ThreadID) {
    // First handle the store
    auto OpSize = Mi->getLength();
    auto WriteDest = Mi->getDest();

    // Do Store
    auto &Ctx = Mi->getContext();
    auto StoreFunc = ExternalFunctions["Store"];
    auto Bcast = new BitCastInst(WriteDest, Type::getInt8PtrTy(Ctx), "", Mi);
    auto Call = CallInst::Create(StoreFunc->getFunctionType(), StoreFunc,
                                 {Bcast, OpSize, ThreadID}, "", Mi);

    // If a transfer type then do the load
    if (auto Mt = dyn_cast<MemTransferInst>(Mi)) {
      auto WriteSrc = Mt->getSource();
      auto LoadFunc = ExternalFunctions["Load"];
      auto Bcast = new BitCastInst(WriteSrc, Type::getInt8PtrTy(Ctx), "", Mt);
      auto Call = CallInst::Create(LoadFunc->getFunctionType(), LoadFunc,
                                   {Bcast, OpSize, ThreadID}, "", Mt);
    }
  }

  // Handles both CallInst and InvokeInst
  template <typename CallTypeInst>
  void checkCallForInstrumentation(CallTypeInst *CTI, Value* ThreadID) {
    Function const *TargetFunc = CTI->getCalledFunction();

    if (TargetFunc->isIntrinsic()){ 
      if(auto MI = dyn_cast<MemIntrinsic>(CTI)){
        handleMemReadOrWrite(MI, ThreadID); 
      } 
      return;
    }
    
    // If func is whitelisted then ignore
    if( functionIsWhiteListed(TargetFunc->getName())) {
      return;
    }

    // If the function is annotated then ignore
    if (AnnotFuncs.matchFunc(TargetFunc)) {
      return;
    }

    std::string CurrentFunction = CTI->getFunction()->getName();
    std::string CalledFunction = TargetFunc->getName();

    std::stringstream error;
    error << "Function(" << CalledFunction
          << ") was not instrumented. Called "
             "from: "
          << CurrentFunction << " either mark it or add it to the whitelist\n";

    report_fatal_error(error.str());
  }


  Value *getOMPThreadID(Instruction *I) {
    auto OmpNumThreads = ExternalFunctions["omp_get_thread_num"];
    return CallInst::Create(OmpNumThreads->getFunctionType(), OmpNumThreads, "",
                            I);
  }

  void startTracing(Instruction *I) {
    auto StartTracing = ExternalFunctions["start_trace"];
    CallInst::Create(StartTracing->getFunctionType(), StartTracing, "", I);
  }

  void stopTracing(Instruction *I) {
    auto StopTracing = ExternalFunctions["stop_trace"];
    CallInst::Create(StopTracing->getFunctionType(), StopTracing, "", I);
  }

  void runOnFunction(Function &F) {
    auto taggedInsts = getTaggedInsts(F);
    if (taggedInsts.empty()) {
      return;
    } else {
      errs() << "\n" << F.getName() << "\n";
    }

    startTracing(taggedInsts.front());
    auto ThreadID = getOMPThreadID(taggedInsts.front());

    for (auto I : taggedInsts) {
      switch (I->getOpcode()) {
      case Instruction::Load:
        handleMemReadOrWrite(dyn_cast<LoadInst>(I), ThreadID);
        break;
      case Instruction::Store:
        handleMemReadOrWrite(dyn_cast<StoreInst>(I), ThreadID);
        break;
      case Instruction::Call:
        checkCallForInstrumentation(dyn_cast<CallInst>(I), ThreadID);
        break;
      case Instruction::Invoke:
        checkCallForInstrumentation(dyn_cast<InvokeInst>(I), ThreadID);
        break;
      case Instruction::Ret:
        stopTracing(I);
        break;
      default:
        break;
      }
    }
  }

  bool runOnModule(Module &M) override {
    AnnotFuncs = parseAnnotations(M);

    ExternalFunctions =
        declareSSTFunctionsInModule(M, AnnotationKind::Memtrace);

    for (Function &F : M.functions()) {
      runOnFunction(F);

      // Hack in a function call at the end of main for a bit
      if (F.getName() == "main") {
        for (auto &I : instructions(F)) {
          if (auto Ret = dyn_cast<ReturnInst>(&I)) {
            auto Dump = ExternalFunctions["Dump"];
            CallInst::Create(Dump->getFunctionType(), Dump, "", Ret);
          }
        }
      }
    }

    return true;
  }
};

} // namespace

char MemtracePass::ID = 0;
static RegisterPass<MemtracePass> X("sst-memtrace", "SSTMAC Memtrace Pass");
