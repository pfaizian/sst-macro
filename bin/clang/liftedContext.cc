#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Analysis/Analyses/ExprMutationAnalyzer.h>

#include <ostream>
#include <sstream>

#include "clang/util.h"
#include "liftedContext.h"

using namespace clang::ast_matchers;

namespace {
std::string my_match = "match";

using MetaDecl =
    std::pair<clang::ValueDecl const* const, LiftingContext::ValueMetaData>;
void add_lifted_type(MetaDecl& pair, clang::ASTContext& ctx);

using NodeVector = llvm::SmallVector<class clang::ast_matchers::BoundNodes, 1>;
using DeclMap =
    std::unordered_map<clang::ValueDecl const*, LiftingContext::ValueMetaData>;
void remove_local_decls(NodeVector const& decl_stmts, DeclMap& current_stmts);

DeclMap determine_mutation_status(NodeVector const& expressions,
                                  clang::ASTContext& ctx,
                                  clang::Stmt const& stmt);
}  // namespace

LiftingContext::LiftingContext(clang::Stmt const* s, clang::ASTContext& ctx)
    : stmt_(s) {
      stmt_->dumpColor();
  // First get all of the declRefExprs in our statement
  auto declref_expresions =
      match(findAll(declRefExpr().bind(my_match)), *stmt_, ctx);
  decls_ = determine_mutation_status(declref_expresions, ctx, *stmt_);

  // Next remove all of the DeclStmts in the Stmt so we can avoid outlining
  // locals
  auto decl_stmts = match(findAll(declStmt().bind(my_match)), *stmt_, ctx);
  remove_local_decls(decl_stmts, decls_);

  // Finally write the lifted type to the meta data for the decl
  for (auto& pair : decls_) {
    add_lifted_type(pair, ctx);
  }
}

std::ostream& LiftingContext::write_lifted_function(std::ostream& os) const {
  // Debugging
  for (auto const& decl : decls_) {
    auto vd = decl.first;
    auto& meta = decl.second;

    std::cout << "Decl " << vd->getNameAsString()
              << " has type: " << meta.input_type.getAsString()
              << " and is being passed as: " << meta.lifted_type.getAsString()
              << "\n";
  }

  // Write function signiture
  std::stringstream ss;
  ss << "void myfunc123(";
  bool first = true;
  for (auto const& decl : decls_) {
    auto vd = decl.first;
    auto& meta = decl.second;

    const auto add_ptr = (meta.lifted_type != meta.input_type) ? "ptr" : "";
    const auto add_comma = (first) ? "" : " ,";
    first = false;
    ss << add_comma << meta.lifted_type.getAsString() << " "
       << vd->getNameAsString() + add_ptr;
  }
  ss << "){\n";

  // Unpack variables
  for (auto const& decl : decls_) {
    auto vd = decl.first;
    auto& meta = decl.second;

    if (meta.lifted_type != meta.input_type) {
      ss << meta.input_type.getNonReferenceType().getAsString() << " " << vd->getNameAsString()
         << " = *" << vd->getNameAsString() + "ptr;\n";
    }
  }

  PrettyPrinter pp;
  pp.print(stmt_);
  pp.dump(ss);

  // repack variables
  for (auto const& decl : decls_) {
    auto vd = decl.first;
    auto& meta = decl.second;

    if (meta.lifted_type != meta.input_type) {
      ss << "*" << vd->getNameAsString() + "ptr"
         << " = " << vd->getNameAsString() << ";\n";
    }
  }
  ss << "}";

  std::cout << ss.str() << std::endl;
  return os;
}

namespace {

void add_lifted_type(MetaDecl& pair, clang::ASTContext& ctx) {
  auto& md = pair.second;
  const auto tc = md.input_type->getTypeClass();
  switch (tc) {
    case (clang::Type::TypeClass::Pointer):
      md.lifted_type = md.input_type; // Pass ptrs by value
      break;
    case (clang::Type::TypeClass::LValueReference):
    case (clang::Type::TypeClass::RValueReference):
      md.lifted_type = ctx.getPointerType(md.input_type.getNonReferenceType());
      break;
    default:
      pair.second.lifted_type = ctx.getPointerType(md.input_type);
      break;
  }
}

void remove_local_decls(NodeVector const& decl_stmts, DeclMap& current_stmts) {
  for (auto const& node : decl_stmts) {
    for (auto const& var_decl :
         node.getNodeAs<clang::DeclStmt>(my_match)->decls()) {
      auto val_decl = static_cast<clang::ValueDecl const*>(var_decl);
      current_stmts.erase(current_stmts.find(val_decl));
    }
  }
}

DeclMap determine_mutation_status(NodeVector const& expressions,
                                  clang::ASTContext& ctx,
                                  clang::Stmt const& stmt) {
  DeclMap output;
  clang::ExprMutationAnalyzer mut(stmt, ctx);
  for (auto& node : expressions) {
    auto decl = node.getNodeAs<clang::DeclRefExpr>(my_match)->getDecl();
    decl->dumpColor();
    

    // Construct the inital metadata
    LiftingContext::ValueMetaData md;

    // TODO Check that isPointeeMutated does what I think it does
    md.mutated = (mut.isMutated(decl) || mut.isPointeeMutated(decl));
    md.input_type = decl->getType();

    output.insert({decl, std::move(md)});
  }
  return output;
}

}  // namespace

