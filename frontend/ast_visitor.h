#pragma once
#include "frontend/ast.h"

namespace cs160::frontend {

// The visitor abstract base class for visiting abstract syntax trees.
class AstVisitor {
 public:
  virtual ~AstVisitor() {}

  // These should be able to change members of the visitor, thus not const
  virtual void VisitNil(const NilExpr& exp) = 0;
  virtual void VisitIntegerExpr(const IntegerExpr& exp) = 0;
  virtual void VisitNewExpr(const NewExpr& exp) = 0;
  virtual void VisitVariable(const Variable&) = 0;
  virtual void VisitAccessPath(const AccessPath&) = 0;
  virtual void VisitAddExpr(const AddExpr& exp) = 0;
  virtual void VisitSubtractExpr(const SubtractExpr& exp) = 0;
  virtual void VisitMultiplyExpr(const MultiplyExpr& exp) = 0;
  virtual void VisitLessThanExpr(const LessThanExpr& exp) = 0;
  virtual void VisitLessThanEqualToExpr(const LessThanEqualToExpr& exp) = 0;
  virtual void VisitEqualToExpr(const EqualToExpr& exp) = 0;
  virtual void VisitLogicalAndExpr(const LogicalAndExpr& exp) = 0;
  virtual void VisitLogicalOrExpr(const LogicalOrExpr& exp) = 0;
  virtual void VisitLogicalNotExpr(const LogicalNotExpr& exp) = 0;
  virtual void VisitTypeExpr(const TypeExpr& exp) = 0;
  virtual void VisitBlockStmt(const BlockStmt& exp) = 0;
  virtual void VisitDeclarationExpr(const Declaration& exp) = 0;
  virtual void VisitAssignmentExpr(const Assignment& assignment) = 0;
  virtual void VisitConditionalExpr(const Conditional& conditional) = 0;
  virtual void VisitLoopExpr(const Loop& loop) = 0;
  virtual void VisitFunctionCallExpr(const FunctionCall& call) = 0;
  virtual void VisitFunctionDefExpr(const FunctionDef& def) = 0;
  virtual void VisitTypeDef(const TypeDef& def) = 0;
  virtual void VisitProgramExpr(const Program& program) = 0;
};

}  // namespace cs160::frontend
