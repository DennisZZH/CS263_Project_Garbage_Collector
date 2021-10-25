
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "ast_visitor.h"

namespace cs160::frontend {

class genTreeVisitor : public AstVisitor {
 public:
  genTreeVisitor() {}
  ~genTreeVisitor() {}

  const std::string GetOutput() const { return output_.str(); }

  void VisitIntegerExpr(const IntegerExpr& exp) override {
    output_ << exp.value();
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

  void VisitLessThanExpr(const LessThanExpr& exp) override {
    output_ << "(< ";
    exp.lhs().Visit(this);
    output_ << " ";
    exp.rhs().Visit(this);
    output_ << ")";
  }

  void VisitLessThanEqualToExpr(const LessThanEqualToExpr& exp) override {
    output_ << "(<= ";
    exp.lhs().Visit(this);
    output_ << " ";
    exp.rhs().Visit(this);
    output_ << ")";
  }

  void VisitEqualToExpr(const EqualToExpr& exp) override {
    output_ << "(= ";
    exp.lhs().Visit(this);
    output_ << " ";
    exp.rhs().Visit(this);
    output_ << ")";
  }

  void VisitLogicalAndExpr(const LogicalAndExpr& exp) override {
    output_ << "(&& ";
    exp.lhs().Visit(this);
    output_ << " ";
    exp.rhs().Visit(this);
    output_ << ")";
  }

  void VisitLogicalOrExpr(const LogicalOrExpr& exp) override {
    output_ << "(|| ";
    exp.lhs().Visit(this);
    output_ << " ";
    exp.rhs().Visit(this);
    output_ << ")";
  }

  void VisitLogicalNotExpr(const LogicalNotExpr& exp) override {
    output_ << "(!";
    exp.operand().Visit(this);
    output_ << ")";
  }

  void VisitIntTypeExpr(const IntType& exp) override { output_ << exp.value(); }

  void VisitBlockStmt(const BlockStmt& exp) override {
    for (auto it = exp.decls().begin(); it != exp.decls().end(); ++it) {
      (*it)->Visit(this);
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
    output_ << "while ";
    exp.guard().Visit(this);
    output_ << " {";
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
      (*it).second->Visit(this);
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

  void VisitProgramExpr(const Program& exp) override {
    output_ << "Program(";
    for (auto it = exp.function_defs().begin(); it != exp.function_defs().end();
         ++it) {
      (*it)->Visit(this);
    }
    exp.statements().Visit(this);
    output_ << "output ";
    exp.arithmetic_exp().Visit(this);
    output_ << ");";
  }

 private:
  std::stringstream output_;

  // map of productions
  std::unordered_map<AstNode, std::vector<AstNode>> productions(
      {{AExpPrime, Variable}});

};  // namespace cs160::frontend
}  // namespace cs160::frontend
