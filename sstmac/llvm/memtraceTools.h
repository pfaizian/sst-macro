#ifndef SSTMAC_LLVM_MEMTRACE_TOOLS_H_INCLUDED
#define SSTMAC_LLVM_MEMTRACE_TOOLS_H_INCLUDED

#include "llvm/ADT/APInt.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Casting.h"
#include <cstdint>

// List of annotation types that our module might ask for.  The purpose of this
// list is mostly to help decide which functions to declare in the module.
enum class AnnotationKind {
  Ignore = 1 << 0,
  Memtrace = 1 << 1,
  OpCount = 1 << 2,
  MemtraceOpCount = Memtrace | OpCount
};

/// Contains all the source info needed to generate annotations.
class AnnotationMap {
public:
  AnnotationMap() = default;
  AnnotationMap(AnnotationMap const &) = default;

  llvm::Optional<AnnotationKind> matchInst(llvm::Instruction const *I) const;
  llvm::Optional<AnnotationKind> matchFunc(llvm::Function const *F) const;

  void addMatchAllFunction(llvm::Function const *, AnnotationKind);
  void addFileAndLines(llvm::Function const *, llvm::SmallVector<int, 5> &&,
                       AnnotationKind);

private:
  llvm::Optional<AnnotationKind>
  matchFileAndLines(llvm::DILocation const *) const;

  llvm::DenseMap<llvm::Function const *, AnnotationKind> FunctionsToMatchAll;
  llvm::DenseMap<llvm::DIFile const *,
                 llvm::SmallVector<std::pair<AnnotationKind, int>, 5>>
      LinesToMatch;
};

// Find the annotated functions
AnnotationMap parseAnnotations(llvm::Module &M);

// Declare the functions that we need to call for the SST code and return
// a map to them so that we can use them in passes
llvm::StringMap<llvm::Function *> declareSSTFunctionsInModule(llvm::Module &,
                                                              AnnotationKind);

#endif // SSTMAC_LLVM_MEMTRACE_TOOLS_H_INCLUDED
