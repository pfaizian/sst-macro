#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Analysis/Analyses/ExprMutationAnalyzer.h>

#include <ostream>

#include "liftedContext.h"

using namespace clang::ast_matchers;

namespace {
using MetaDecl =
    std::pair<clang::ValueDecl const*, LiftingContext::ValueMetaData>;
clang::QualType get_lifted_type(MetaDecl const& pair, clang::ASTContext& ctx);
}  // namespace

LiftingContext::LiftingContext(clang::Stmt const* s, clang::ASTContext& ctx)
    : stmt_(s) {
  std::string id = "my_match";

  clang::ExprMutationAnalyzer mut(*stmt_,
                                  ctx);  // Used to see if a var is mutated

  // First get all of the declRefExprs in our statement
  auto declref_expresions = match(findAll(declRefExpr().bind(id)), *stmt_, ctx);
  for (auto& node : declref_expresions) {
    auto decl = node.getNodeAs<clang::DeclRefExpr>(id)->getDecl();

    // Construct the inital metadata
    ValueMetaData md;
    // TODO Check that isPointeeMutated does what I think it does
    md.mutated = (mut.isMutated(decl) || mut.isPointeeMutated(decl));
    md.input_type = decl->getType();

    decls_.insert({decl, std::move(md)});
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

  // Finally determine the input prameters of the lifting function
  for (auto& pair : decls_) {
    pair.second.lifted_type = get_lifted_type(pair, ctx);
  }
}

std::ostream& LiftingContext::write_lifted_function(std::ostream& os) const {
  for (auto const& decl : decls_) {
    auto vd = decl.first;
    auto& meta = decl.second;

    std::cout << "Decl " << vd->getNameAsString()
              << " has type: " << meta.input_type.getAsString()
              << " and is being passed as: " << meta.lifted_type.getAsString()
              << "\n";
  }
  return os;
}

namespace {
// Add a switch on currently accepted types and throw on rest
bool is_allowed_type(clang::Type const& t) {
  switch (t.getTypeClass()) {
    case clang::Type::TypeClass::LValueReference:
    case clang::Type::TypeClass::Pointer:
    case clang::Type::TypeClass::Builtin:
    case clang::Type::TypeClass::Complex:
      return true;
    default:
      return false;
  }
}

clang::QualType get_lifted_type(MetaDecl const& pair, clang::ASTContext& ctx) {
  // Capture our stuff
  auto& meta = pair.second;
  auto type = meta.input_type.getTypePtr();

  if(!is_allowed_type(*type)){ // Check if we can handle this
    std::abort();
  }

  clang::QualType output_type;

  // Do as little work as possible fowarding the type
  if (meta.mutated == false) {
    if (type->isPointerType() || type->isBuiltinType()) {
      output_type = ctx.getConstType(meta.input_type);
    } else {
      if (type->isLValueReferenceType()) {  // Yay c++
        output_type = ctx.getConstType(meta.input_type);
      } else {  // Not a pointer and not a lvalue ref just pass as pointer to
                // const
        output_type = ctx.getPointerType(ctx.getConstType(meta.input_type));
      }
    }
  } else {  // type gets mutated
    // Just pass as is since it's gonna get modified anyways
    if (type->isLValueReferenceType() || type->isPointerType()) {
      output_type = meta.input_type;  // Dunno what else to do
    } else {                          // value type for now ;)
      output_type = ctx.getPointerType(meta.input_type);
    }
  }

  return output_type;
}
}  // namespace

