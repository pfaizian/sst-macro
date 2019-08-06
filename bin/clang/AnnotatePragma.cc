/**
Copyright 2009-2019 National Technology and Engineering Solutions of Sandia,
LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S.  Government
retains certain rights in this software.

Sandia National Laboratories is a multimission laboratory managed and operated
by National Technology and Engineering Solutions of Sandia, LLC., a wholly
owned subsidiary of Honeywell International, Inc., for the U.S. Department of
Energy's National Nuclear Security Administration under contract DE-NA0003525.

Copyright (c) 2009-2019, NTESS

All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Questions? Contact sst-macro-help@sandia.gov
*/

#include "AnnotatePragma.h"
#include "astVisitor.h"
#include <clang/AST/Stmt.h>

namespace {
clang::FunctionDecl const *getParentFunctionDecl(clang::Stmt const *S,
                                               clang::ASTContext &Ctx) {
  for (auto const &P : Ctx.getParents(*S)) {
    if (auto const FD = P.get<clang::FunctionDecl>()){
      return FD;
    } else if (auto const ST = P.get<clang::Stmt>()) {
      return getParentFunctionDecl(ST, Ctx);
    }
  }

  return nullptr;
}

// If this is inefficient switch to sstream @JJ
std::string writeAnnotation(std::string const &Type, unsigned Start,
                            unsigned End) {
  std::string Annotation = "__attribute__((annotate(\"" + Type + ":";
  for (auto I = Start; I <= End; ++I) {
    if (I != Start) {
      Annotation += ",";
    }
    Annotation += std::to_string(I);
  }
  Annotation += "\"))) ";
  return Annotation;
}
} // namespace

SSTAnnotatePragma::SSTAnnotatePragma(
    std::string ToolStr, std::map<std::string, std::list<std::string>> Args)
    : SSTPragma(Annotate), Tool(std::move(ToolStr)), ToolArgs(std::move(Args)) {
}

void SSTAnnotatePragma::activate(clang::Stmt *S, clang::Rewriter &R,
                                 PragmaConfig &Cfg) {
  // Get the start and end source locations for this statement
  auto &Sm = Cfg.astVisitor->getCompilerInstance().getSourceManager();
  auto Begin = Sm.getPresumedLineNumber(S->getBeginLoc());
  auto End = Sm.getPresumedLineNumber(S->getEndLoc());

  auto &Ctx = Cfg.astVisitor->getCompilerInstance().getASTContext();
  if (auto ParentFunc = getParentFunctionDecl(S, Ctx)) {
    R.InsertTextBefore(ParentFunc->getBeginLoc(),
                       writeAnnotation(Tool, Begin, End));
  } else {
    errorAbort(S->getBeginLoc(), Cfg.astVisitor->getCompilerInstance(),
               "Couldn't find a parent function for the statement");
  }
}

void SSTAnnotatePragma::activate(clang::Decl *D, clang::Rewriter &R,
                                 PragmaConfig &Cfg) {
  auto LocalD = D;
  if (auto TD = llvm::dyn_cast<clang::FunctionTemplateDecl>(LocalD)) {
    LocalD = TD->getAsFunction();
  }

  auto &Sm = Cfg.astVisitor->getCompilerInstance().getSourceManager();
  auto Begin = Sm.getPresumedLineNumber(LocalD->getBeginLoc());
  auto End = Sm.getPresumedLineNumber(LocalD->getEndLoc());
  R.InsertTextBefore(LocalD->getBeginLoc(), writeAnnotation(Tool, Begin, End));
}

SSTAnnotatePragmaHandler::SSTAnnotatePragmaHandler(SSTPragmaList &Plist,
                                                   clang::CompilerInstance &Ci,
                                                   SkeletonASTVisitor &Visitor)
    : SSTStringMapPragmaHandler("placeholder", Plist, Ci, Visitor) {}

SSTPragma *SSTAnnotatePragmaHandler::allocatePragma(
    std::map<std::string, std::list<std::string>> const &Args) const {
  if (auto Tool = Args.find("tool") == Args.end()) {
    std::cerr << "AnnotatePragma must have a tool argument.\n";
    exit(EXIT_FAILURE);
  }

  std::map<std::string, std::list<std::string>> Copy = Args;
  auto Tool = Copy.find("tool")->second.front();
  Copy.erase(Copy.find("tool"));

  return new SSTAnnotatePragma(Tool, std::move(Copy));
}
