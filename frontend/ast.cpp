
#include "frontend/ast.h"
#include "frontend/ast_visitor.h"
#include "frontend/print_visitor.h"

namespace cs160::frontend {

std::string AstNode::toString() const {
  PrintVisitor pv;
  this->Visit(&pv);
  return pv.GetOutput();
}

void AddExpr::Visit(AstVisitor* visitor) const { visitor->VisitAddExpr(*this); }
void NewExpr::Visit(AstVisitor* visitor) const {
  visitor->VisitNewExpr(*this);
}
void NilExpr::Visit(AstVisitor* visitor) const {
  visitor->VisitNil(*this);
}
void IntegerExpr::Visit(AstVisitor* visitor) const {
  visitor->VisitIntegerExpr(*this);
}
void Variable::Visit(AstVisitor* visitor) const {
  visitor->VisitVariable(*this);
}
void AccessPath::Visit(AstVisitor* visitor) const {
  visitor->VisitAccessPath(*this);
}
void SubtractExpr::Visit(AstVisitor* visitor) const {
  visitor->VisitSubtractExpr(*this);
}
void MultiplyExpr::Visit(AstVisitor* visitor) const {
  visitor->VisitMultiplyExpr(*this);
}
void LessThanExpr::Visit(AstVisitor* visitor) const {
  visitor->VisitLessThanExpr(*this);
}
void LessThanEqualToExpr::Visit(AstVisitor* visitor) const {
  visitor->VisitLessThanEqualToExpr(*this);
}
void EqualToExpr::Visit(AstVisitor* visitor) const {
  visitor->VisitEqualToExpr(*this);
}
void LogicalAndExpr::Visit(AstVisitor* visitor) const {
  visitor->VisitLogicalAndExpr(*this);
}
void LogicalOrExpr::Visit(AstVisitor* visitor) const {
  visitor->VisitLogicalOrExpr(*this);
}
void LogicalNotExpr::Visit(AstVisitor* visitor) const {
  visitor->VisitLogicalNotExpr(*this);
}
void TypeExpr::Visit(AstVisitor* visitor) const {
  visitor->VisitTypeExpr(*this);
}
void BlockStmt::Visit(AstVisitor* visitor) const {
  visitor->VisitBlockStmt(*this);
}
void Declaration::Visit(AstVisitor* visitor) const {
  visitor->VisitDeclarationExpr(*this);
}
void Assignment::Visit(AstVisitor* visitor) const {
  visitor->VisitAssignmentExpr(*this);
}
void Conditional::Visit(AstVisitor* visitor) const {
  visitor->VisitConditionalExpr(*this);
}
void Loop::Visit(AstVisitor* visitor) const { visitor->VisitLoopExpr(*this); }
void FunctionDef::Visit(AstVisitor* visitor) const {
  visitor->VisitFunctionDefExpr(*this);
}
void TypeDef::Visit(AstVisitor* visitor) const {
  visitor->VisitTypeDef(*this);
}
void FunctionCall::Visit(AstVisitor* visitor) const {
  visitor->VisitFunctionCallExpr(*this);
}
void Program::Visit(AstVisitor* visitor) const {
  visitor->VisitProgramExpr(*this);
}

}  // namespace cs160::frontend
