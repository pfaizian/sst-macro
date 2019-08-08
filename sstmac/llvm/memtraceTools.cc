#include "llvm/ADT/APInt.h"
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
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include <algorithm>
#include <sstream>

#include "memtraceTools.h"

using namespace llvm;

void AnnotationMap::addFunctionAnnotation(Function const *F, AnnotationKind K) {
  auto Iter = FunctionAnnotations.find(F);
  if (Iter == FunctionAnnotations.end()) {
    FunctionAnnotations.insert({F, K});
  } else {
    if (Iter->second != K) {
      report_fatal_error(
          "Trying to set a function annotation with different AnnotationKinds");
    }
  }
}

void AnnotationMap::addSrcLinesAnnotation(Function const *F,
                                          SmallVector<int, 5> &&Lines,
                                          AnnotationKind K) {

  auto File = F->getSubprogram()->getFile();
  auto &LineMap = LineAnnotations[File];
  for (auto const &L : Lines) {
    LineMap[L] = K; // Don't check for collisions right now
  }
}

AnnotationKind AnnotationMap::matchLine(llvm::DILocation const *Dl) const {
  if (!Dl) { // Check that our DebugLocation is valid
    return AnnotationKind::None;
  }

  // If no matching file return None
  auto const *File = Dl->getFile();
  auto FileIter = LineAnnotations.find(File);
  if (FileIter == LineAnnotations.end()) {
    return AnnotationKind::None;
  }

  auto const &AKLineMap = FileIter->second;
  auto LineIter = AKLineMap.find(Dl->getLine());
  if (LineIter == AKLineMap.end()) {
    return AnnotationKind::None;
  }

  return LineIter->second;
}

AnnotationKind AnnotationMap::matchFunc(llvm::Function const *F) const {
  auto FuncIter = FunctionAnnotations.find(F);
  if (FuncIter == FunctionAnnotations.end()) {
    return AnnotationKind::None;
  }

  return FuncIter->second;
}

int AnnotationMap::matchInst(llvm::Instruction const *I) const {
  auto FuncAK = matchFunc(I->getFunction());
  auto LinesAK = matchLine(I->getDebugLoc());

  return FuncAK | LinesAK;
}

StringMap<Function *> declareSSTFunctionsInModule(Module &M, AnnotationKind K) {
  StringMap<Function *> Funcs;

  { // Add SST Functions
    auto IntPtrType8 = Type::getInt8PtrTy(M.getContext());
    auto IntType32 = Type::getInt32Ty(M.getContext());
    auto IntType64 = Type::getInt64Ty(M.getContext());
    auto VoidType = Type::getVoidTy(M.getContext());

    // Add start
    auto StartTrack = FunctionType::get(VoidType, false);
    auto Start = Function::Create(StartTrack, Function::ExternalLinkage,
                                  "sstmac_start_trace", M);
    Funcs["start_trace"] = Start;

    // Add stop
    auto StopTrack = FunctionType::get(VoidType, false);
    auto Stop = Function::Create(StopTrack, Function::ExternalLinkage,
                                 "sstmac_end_trace", M);
    Funcs["stop_trace"] = Stop;

    // Add loads
    auto AddrTrack =
        FunctionType::get(VoidType, {IntPtrType8, IntType64, IntType32}, false);
    auto Load = Function::Create(AddrTrack, Function::ExternalLinkage,
                                 "sstmac_address_load", M);
    Funcs["Load"] = Load;

    // Add stores
    auto Store = Function::Create(AddrTrack, Function::ExternalLinkage,
                                  "sstmac_address_store", M);
    Funcs["Store"] = Store;

    // Dump Info
    auto AddrsInfoDump = FunctionType::get(VoidType, false);
    auto InfoDump = Function::Create(AddrsInfoDump, Function::ExternalLinkage,
                                     "sstmac_print_address_info", M);
    Funcs["Dump"] = InfoDump;

    auto OmpNumThreadsTrack = FunctionType::get(IntType32, false);
    auto OmpNumThreads = Function::Create(
        OmpNumThreadsTrack, Function::ExternalLinkage, "omp_get_thread_num", M);
    Funcs["omp_get_thread_num"] = OmpNumThreads;
  }

  return Funcs;
}

namespace {
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
} // namespace

AnnotationMap parseAnnotations(Module &M) {
  AnnotationMap MyMap;
  for (auto const &I : M.globals()) {
    if (I.getName() == "llvm.global.annotations") {
      ConstantArray *CA = dyn_cast<ConstantArray>(I.getOperand(0));

      for (auto OI = CA->op_begin(), End = CA->op_end(); OI != End; ++OI) {
        ConstantStruct *CS = dyn_cast<ConstantStruct>(OI->get());
        Function *Func = dyn_cast<Function>(CS->getOperand(0)->getOperand(0));

        GlobalVariable *AnnotationGL =
            dyn_cast<GlobalVariable>(CS->getOperand(1)->getOperand(0));

        StringRef Annotation =
            dyn_cast<ConstantDataArray>(AnnotationGL->getInitializer())
                ->getAsCString();

        if (Annotation.contains("memtrace:{")) {
          MyMap.addFunctionAnnotation(Func, AnnotationKind::Ignore);
          MyMap.addSrcLinesAnnotation(Func, getLines(Annotation),
                                      AnnotationKind::Memtrace);
        } else if (Annotation.contains("memtrace:ignore,{")) { // Ignore some
          MyMap.addFunctionAnnotation(Func, AnnotationKind::Ignore);
          MyMap.addSrcLinesAnnotation(Func, getLines(Annotation),
                                      AnnotationKind::Ignore);
        } else if (Annotation.contains("memtrace:ignore")) {
          MyMap.addFunctionAnnotation(Func, AnnotationKind::Ignore);
        }
        if (Annotation.contains("memtrace:all,{")) {
          MyMap.addFunctionAnnotation(Func, AnnotationKind::Memtrace);
          MyMap.addSrcLinesAnnotation(Func, getLines(Annotation),
                                      AnnotationKind::Memtrace);
        }
      }
    }
  }

  return MyMap;
}

// Find functions that need to get picked up that aren't annotated
void checkRegexFuncMatches(llvm::Module &M, AnnotationMap &AM){
  const StringMap<AnnotationKind> AlwaysMatch = {
    {"omp", AnnotationKind::Memtrace}
  };

  for(auto const &F : M){
    auto const& Name = F.getName();
    for(auto const& Match : AlwaysMatch){
      if(Name.contains(Match.getKey())){
        AM.addFunctionAnnotation(&F,Match.second);
      }
    }
  }
}
