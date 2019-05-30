#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Analysis/Analyses/ExprMutationAnalyzer.h>

#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/Stmt.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/util.h"
#include "liftedContext.h"

#include <algorithm>
#include <cstdlib>
#include <ostream>

using namespace clang::ast_matchers;

LiftingContext::LiftingContext(clang::Stmt const* s, clang::ASTContext& ctx) : stmt_(s) {
  std::string id = "my_match";

  // First get all of the declRefExprs in our statement
  auto declref_expresions = match(findAll(declRefExpr().bind(id)), *stmt_, ctx);
  for (auto& node : declref_expresions) {
    auto expr = node.getNodeAs<clang::DeclRefExpr>(id);
    decls_.insert({expr->getDecl(), ValueMetaData()});
  }

  // Next get all of the DeclStmts in the Stmt so we can avoid lifting locals
  auto decl_stmts = match(findAll(declStmt().bind(id)), *stmt_, ctx);
  for (auto const& node : decl_stmts) {
    for (auto const& var_decl : node.getNodeAs<clang::DeclStmt>(id)->decls()) {
      // Erase VarDelcs that were declared in the current scope since those
      // will be copied in the the src2src part
      auto val_decl = static_cast<clang::ValueDecl const*>(var_decl);
      decls_.erase(decls_.find(val_decl));
    }
  }

  // Determine which of the filtered variables are not mutated
  clang::ExprMutationAnalyzer mut(*stmt_, ctx);
  for (auto& vd : decls_) {
    vd.second.mutated = mut.isMutated(vd.first);
  }

}

namespace {
  clang::Type get_variable_input_type(std::pair<clang::ValueDecl const*, LiftingContext::ValueMetaData> const& pair){
  }
}

// TODO figure out why this fails with a use after free error
std::ostream& LiftingContext::write_lifted_function(std::ostream &os) const {
  stmt_->dumpColor();
  return os;
}
