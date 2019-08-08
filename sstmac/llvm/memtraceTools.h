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
enum AnnotationKind {
  None = 1 << 0,
  Ignore = 1 << 1,
  Memtrace = 1 << 2,
  OpCount = 1 << 3
};

/// Contains all the source info needed to generate annotations.
class AnnotationMap {
public:
  AnnotationMap() = default;
  AnnotationMap(AnnotationMap const &) = default;

  int matchInst(llvm::Instruction const *I) const;
  AnnotationKind matchFunc(llvm::Function const *F) const;

  void addFunctionAnnotation(llvm::Function const *, AnnotationKind);
  void addSrcLinesAnnotation(llvm::Function const *, llvm::SmallVector<int, 5> &&,
                       AnnotationKind);

private:
  AnnotationKind
  matchLine(llvm::DILocation const *) const;

  llvm::DenseMap<llvm::Function const *, AnnotationKind> FunctionAnnotations;
  llvm::DenseMap<llvm::DIFile const *, llvm::DenseMap<int, AnnotationKind>>
      LineAnnotations;
};

// Find the annotated functions
AnnotationMap parseAnnotations(llvm::Module &M);

// Find functions that need to get picked up that aren't annotated
void checkRegexFuncMatches(llvm::Module &, AnnotationMap &);

// Declare the functions that we need to call for the SST code and return
// a map to them so that we can use them in passes
llvm::StringMap<llvm::Function *> declareSSTFunctionsInModule(llvm::Module &,
                                                              AnnotationKind);

#endif // SSTMAC_LLVM_MEMTRACE_TOOLS_H_INCLUDED
