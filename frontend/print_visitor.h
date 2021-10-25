
#include <iostream>
#include <sstream>
#include <string>

#include "frontend/ast_visitor.h"

namespace cs160::frontend {

class PrintVisitor : public AstVisitor {
 public:
  PrintVisitor() {}
  ~PrintVisitor() {}

  const std::string GetOutput() const { return output_.str(); }

  void VisitIntegerExpr(const IntegerExpr& exp) override {
    output_ << exp.value();
  }

  void VisitNewExpr(const NewExpr& exp) override {
    output_ << "new " << exp.type();
  }

  void VisitNil(const NilExpr& exp) override {
    output_ << "nil";
  }

  void VisitAddExpr(const AddExpr& exp) override {
    output_ << "(+ ";
    exp.lhs().Visit(this);
    output_ << " ";
    exp.rhs().Visit(this);
    output_ << ")";
  }

  void VisitSubtractExpr(const SubtractExpr& exp) override {
    output_ << "(- ";
    exp.lhs().Visit(this);
    output_ << " ";
    exp.rhs().Visit(this);
    output_ << ")";
  }

  void VisitMultiplyExpr(const MultiplyExpr& exp) override {
    output_ << "(* ";
    exp.lhs().Visit(this);
    output_ << " ";
    exp.rhs().Visit(this);
    output_ << ")";
  }

  void VisitVariable(const Variable& exp) override {
    output_ << exp.name();
  }

  void VisitAccessPath(const AccessPath& path) override {
    path.root().Visit(this);
    for (auto const & f : path.fieldAccesses()) {
      output_ << "." << f;
    }
  }

  void VisitLessThanExpr(const LessThanExpr& exp) override {
    output_ << "[< ";
    exp.lhs().Visit(this);
    output_ << " ";
    exp.rhs().Visit(this);
    output_ << "]";
  }

  void VisitLessThanEqualToExpr(const LessThanEqualToExpr& exp) override {
    output_ << "[<= ";
    exp.lhs().Visit(this);
    output_ << " ";
    exp.rhs().Visit(this);
    output_ << "]";
  }

  void VisitEqualToExpr(const EqualToExpr& exp) override {
    output_ << "[= ";
    exp.lhs().Visit(this);
    output_ << " ";
    exp.rhs().Visit(this);
    output_ << "]";
  }

  void VisitLogicalAndExpr(const LogicalAndExpr& exp) override {
    output_ << "[&& ";
    exp.lhs().Visit(this);
    output_ << " ";
    exp.rhs().Visit(this);
    output_ << "]";
  }

  void VisitLogicalOrExpr(const LogicalOrExpr& exp) override {
    output_ << "[|| ";
    exp.lhs().Visit(this);
    output_ << " ";
    exp.rhs().Visit(this);
    output_ << "]";
  }

  void VisitLogicalNotExpr(const LogicalNotExpr& exp) override {
    output_ << "[!";
    exp.operand().Visit(this);
    output_ << "]";
  }

  void VisitTypeExpr(const TypeExpr& exp) override { output_ << exp.name(); }

  void VisitBlockStmt(const BlockStmt& exp) override {
    for (auto it = exp.decls().begin(); it != exp.decls().end(); ++it) {
      it->Visit(this);
    }
    output_ << " ";
    for (auto it = exp.stmts().begin(); it != exp.stmts().end(); ++it) {
      (*it)->Visit(this);
    }
  }

  void VisitDeclarationExpr(const Declaration& exp) override {
    output_ << "";
    exp.type().Visit(this);
    output_ << " ";
    exp.id().Visit(this);
    output_ << "; ";
  }

  void VisitAssignmentExpr(const Assignment& exp) override {
    output_ << "";
    exp.lhs().Visit(this);
    output_ << " := ";
    exp.rhs().Visit(this);
    output_ << "; ";
  }

  void VisitConditionalExpr(const Conditional& exp) override {
    output_ << "if ";
    exp.guard().Visit(this);
    output_ << " {";
    exp.true_branch().Visit(this);
    output_ << "} else {";
    exp.false_branch().Visit(this);
    output_ << "}";
  }

  void VisitLoopExpr(const Loop& exp) override {
    output_ << "while (";
    exp.guard().Visit(this);
    output_ << ") {";
    exp.body().Visit(this);
    output_ << "}";
  }

  void VisitFunctionCallExpr(const FunctionCall& exp) override {
    output_ << exp.callee_name() << "(";
    for (auto it = exp.arguments().begin(); it != exp.arguments().end(); ++it) {
      (*it)->Visit(this);
    }
    output_ << ")";
  }

  void VisitFunctionDefExpr(const FunctionDef& exp) override {
    output_ << "def " << exp.function_name();
    output_ << "(";
    for (auto it = exp.parameters().begin(); it != exp.parameters().end();
         ++it) {
      (*it).first->Visit(this);
      output_ << " ";
      (*it).second.Visit(this);
      if (std::next(it) != exp.parameters().end()) {
        output_ << ", ";
      }
    }
    output_ << ") : ";
    exp.type().Visit(this);
    output_ << " {";
    exp.function_body().Visit(this);
    output_ << "return ";
    exp.retval().Visit(this);
    output_ << "; }";
  }

  void VisitTypeDef(const TypeDef & typeDef) override {
    output_ << "struct " << typeDef.type_name() << " {\n";
    for (auto & decl : typeDef.fields()) {
      decl.Visit(this);
    }
    output_ << "\n};";
  }

  void VisitProgramExpr(const Program& exp) override {
    // output_ << "Program(";
    for (auto it = exp.function_defs().begin(); it != exp.function_defs().end();
         ++it) {
      (*it)->Visit(this);
    }
    exp.statements().Visit(this);
    output_ << " output ";
    exp.arithmetic_exp().Visit(this);
    output_ << ";";
  }

 private:
  std::stringstream output_;
};  // namespace cs160::frontend
}  // namespace cs160::frontend
