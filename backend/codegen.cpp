#include "backend/codegen.h"

#include <algorithm>
#include <sstream>
#include <iostream>
#include <iomanip>

namespace cs160::backend {

// Assembly instruction operands, short names
struct Operand {
 virtual const std::string toString() const = 0;
};

// register
struct R final : public Operand {
  std::string value;

  R(std::string v) : value(std::move(v)) {}

  const std::string toString() const override {
    return '\%' + value;
  }
};

// label
struct L final : public Operand {
  std::string value;

  L(std::string v) : value(std::move(v)) {}

  const std::string toString() const override {
    return value;
  }
};

// pre-defined registers
static const R EAX{"eax"};
static const R EDX{"edx"};
static const R ESP{"esp"};
static const R EBP{"ebp"};

// integer constant
struct C final : public Operand {
  int32_t value;

  C(int32_t v) : value(v) {}

  const std::string toString() const override {
    return '$' + std::to_string(value);
  }
};

// unsigned integer constant, printed in hexadecimal for easier reading
struct H final : public Operand {
  uint32_t value;

  H(uint32_t v) : value(v) {}

  const std::string toString() const override {
    std::ostringstream s;
    s << "$0x" << std::hex << std::setfill('0') << std::setw(8) << value;
    return s.str();
  }
};

// offset
struct O final : public Operand {
  int32_t offset;
  R reg;

  O(int32_t offset, R reg) : offset(offset), reg(reg) {}

  const std::string toString() const override {
    std::ostringstream s;
    s << offset << "(" << reg.toString() << ")";
    return s.str();
  }
};

// Helper functions for producing assembly code

// overload for nullary case
std::string Insn(const std::string & opcode) {
  return "  " + opcode;
}

// overload for non-nullary case
template<typename OpT1, typename ... OpT>
std::string Insn(std::string opcode, OpT1 first, OpT... rest) {
  std::ostringstream insn;
  insn << "  " << opcode << " " << first.toString();

  // use fold expression to print the rest
  ((insn << ", " << rest.toString()), ...);  

  return insn.str();
}

std::optional<VarInfo> Context::lookup(std::string const & x) {
  auto offset = varInfo.find(x);

  if (offset != varInfo.end()) {
    return offset->second;
  }

  // x is not defined in current scope, check parents
  if (parent) {
    return parent->lookup(x);
  } else {
    return std::nullopt;
  }
}

// Symbol table methods

const std::string SymbolTable::tmpPrefix = "tmp_";

// Check and add function definition
void SymbolTable::addFnDef(const FunctionDef & fnDef) {
  if (fnInfo.count(fnDef.function_name()) != 0) {
    throw CodeGenError {"Function " + fnDef.function_name() + " is defined more than once"};
  }

  std::vector<std::string> paramTypes;

  for (auto & [type, _] : fnDef.parameters()) {
    paramTypes.push_back(type->name());
  }

  fnInfo.emplace(std::string(fnDef.function_name()), FnInfo{std::move(paramTypes), std::string(fnDef.type().name())});
}

uint32_t SymbolTable::getArity(const std::string & f) {
  auto a = fnInfo.find(f);
  if (a == fnInfo.end()) {
    throw CodeGenError {"Trying to use undefined function " + f};
  }

  return a->second.argTypes.size();
}

// Reset local variable information, used when entering a new function definition
void SymbolTable::resetLocalsInfo() {
  ctx = {};
}

// Allocate stack space for a new local variable
void SymbolTable::allocateVar(std::string x, const std::string & type) {
  if (ctx.varInfo.count(x)) {
    throw CodeGenError { x + " is already defined in the same scope" };
  }
  ctx.varInfo.emplace(std::move(x), VarInfo{ctx.nextOffset, type});
  // move the offset counter by 4
  ctx.nextOffset += 4;
}

// Create a new scope, used when entering a block
void SymbolTable::openScope() {
  auto oldOffset = ctx.nextOffset;
  ctx = Context{{}, std::make_unique<Context>(std::move(ctx)), oldOffset};
}

// Clear the current scope and go back to the parent
void SymbolTable::closeScope() {
  std::unique_ptr<Context> parent;
  ctx.parent.swap(parent);
  ctx = std::move(*parent);
}

// Temporary variable implementation

const std::string CodeGen::TmpVar::IntT = "int";

CodeGen::TmpVar::TmpVar(const std::string & name, CodeGen &codegen) : name(name), codegen(codegen) {
  codegen.symbolTable.openScope();
  codegen.symbolTable.allocateVar(name, IntT);
  codegen.insns.push_back(Insn("sub", C{4}, ESP));
}

CodeGen::TmpVar::~TmpVar() {
  codegen.symbolTable.closeScope();
  codegen.insns.push_back(Insn("add", C{4}, ESP));
}

int32_t CodeGen::TmpVar::operator * () const {
  return codegen.symbolTable.ctx.lookup(name)->first;
}

// Code generator methods

CodeGen::TmpVar CodeGen::freshTmp() {
  auto name = SymbolTable::tmpPrefix + std::to_string(symbolTable.nextTmp++);
  return TmpVar(name, *this);
}

std::vector<std::string> CodeGen::generateCode(const Program & program) {
  // reset instructions, label counter, symbol table, etc.
  insns = {"  .extern allocate"};
  nextIndex = 0;
  symbolTable = {};
  inTopLevelScope = true;
  // actual code gen
  VisitProgramExpr(program);
  return insns;
}

void CodeGen::VisitNil(const NilExpr& exp) {
  // We represent `nil` as constant 0
  insns.push_back(Insn("movl", C{0}, EAX));
}

void CodeGen::VisitNewExpr(const NewExpr& exp) {
  auto typeInfo = symbolTable.typeInfo.find(exp.type());
  if (typeInfo == symbolTable.typeInfo.end()) {
    throw CodeGenError { "Type " + exp.type() + " is not defined" };
  }

  auto size = static_cast<int32_t>(typeInfo->second.fields.size());
  // call allocate(int32_t size)
  insns.push_back("  // ALLOCATE FOR NEW " + exp.type());
  insns.push_back(Insn("pushl", C{size}));
  insns.push_back(Insn("call", L{"allocate"}));
  insns.push_back(Insn("sub", C{4}, ESP));
  insns.push_back("  // SET TAG");
  // set up the tag
  insns.push_back(Insn("movl", H{typeInfo->second.tag()}, O{-4, EAX}));
  insns.push_back("  // INITIALIZE FIELDS");
  // initialize fields to 0
  for (int32_t i = 0; i < size; ++i) {
    insns.push_back(Insn("movl", C{0}, O{i * 4, EAX}));
  }
  insns.push_back("  // END NEW " + exp.type());
}

void CodeGen::VisitIntegerExpr(const IntegerExpr& exp) {
  // Store the constant in the result register
  insns.push_back(Insn("movl", C{exp.value()}, EAX));
}

void CodeGen::VisitVariable(const Variable& exp) {
  // Load the address of the variable to result register. More efficient code can
  // be generated using LEA instruction.
  auto varOffset = symbolTable.ctx.lookup(exp.name());

  if (!varOffset) {
    throw CodeGenError {"reference to undefined variable " + exp.name()};
  }

  insns.push_back(Insn("movl", EBP, EAX));
  insns.push_back(Insn("sub", C{varOffset->first}, EAX) + "  /* load address of " + exp.name() + " */");
}

void CodeGen::VisitAccessPath(const AccessPath& exp) {
  // Note: the algorithm here computes the address to the access path
  // iteratively so it does dereferences and offset calculation in
  // separate instructions. A more efficient algorithm could put all
  // offset calculations to movl instructions, except for the last
  // `add` instruction.
  
  exp.root().Visit(this);
  // Dereference field addresses
  auto type = symbolTable.ctx.lookup(exp.root().name())->second;
  for (auto field : exp.fieldAccesses()) {
    auto [offset, fieldType] = symbolTable.typeInfo[type].varInfoOf(field);
    // dereference field address
    insns.push_back(Insn("movl", O{0, EAX}, EAX) + " /* dereference the address at EAX */");
    insns.push_back(Insn("add", C{offset * 4}, EAX) + " /* load address of field ." + field + " */");
    // update the type
    type = fieldType;
  }
  // Dereference the last part if we are not in lhs
  if (! inLhsOfAssignment) {
    insns.push_back("  // dereference the address because we are on rhs of an assignment");
    insns.push_back(Insn("movl", O{0, EAX}, EAX));
  }
}

void CodeGen::VisitAddExpr(const AddExpr& exp) {
  auto tmpVar = freshTmp();
  exp.lhs().Visit(this);
  insns.push_back(Insn("movl", EAX, O{-(*tmpVar), EBP}));
  exp.rhs().Visit(this);
  insns.push_back(Insn("movl", O{-(*tmpVar), EBP}, EDX));

  // LHS is at EDX, RHS is at EAX
  insns.push_back(Insn("add", EDX, EAX));
}
void CodeGen::VisitSubtractExpr(const SubtractExpr& exp) {
  auto tmpVar = freshTmp();
  exp.lhs().Visit(this);
  insns.push_back(Insn("movl", EAX, O{-(*tmpVar), EBP}));
  exp.rhs().Visit(this);
  insns.push_back(Insn("movl", O{-(*tmpVar), EBP}, EDX));

  // LHS is at EDX, RHS is at EAX
  insns.push_back(Insn("sub", EAX, EDX));
  insns.push_back(Insn("movl", EDX, EAX));
 }
void CodeGen::VisitMultiplyExpr(const MultiplyExpr& exp) {
  auto tmpVar = freshTmp();
  exp.lhs().Visit(this);
  insns.push_back(Insn("movl", EAX, O{-(*tmpVar), EBP}));
  exp.rhs().Visit(this);
  insns.push_back(Insn("movl", O{-(*tmpVar), EBP}, EDX));

  // LHS is at EDX, RHS is at EAX
  insns.push_back(Insn("imul", EDX, EAX));
}
void CodeGen::VisitLessThanExpr(const LessThanExpr& exp) {
  auto tmpVar = freshTmp();
  exp.lhs().Visit(this);
  insns.push_back(Insn("movl", EAX, O{-(*tmpVar), EBP}));
  exp.rhs().Visit(this);
  insns.push_back(Insn("movl", O{-(*tmpVar), EBP}, EDX));

  // LHS is at EDX, RHS is at EAX
  insns.push_back("  cmp %eax, %edx");
  insns.push_back("  setl %al");
  insns.push_back("  movzbl %al, %eax");
}
void CodeGen::VisitLessThanEqualToExpr(const LessThanEqualToExpr& exp) {
  auto tmpVar = freshTmp();
  exp.lhs().Visit(this);
  insns.push_back(Insn("movl", EAX, O{-(*tmpVar), EBP}));
  exp.rhs().Visit(this);
  insns.push_back(Insn("movl", O{-(*tmpVar), EBP}, EDX));

  // LHS is at EDX, RHS is at EAX
  insns.push_back("  cmp %eax, %edx");
  insns.push_back("  setle %al");
  insns.push_back("  movzbl %al, %eax");
}
void CodeGen::VisitEqualToExpr(const EqualToExpr& exp) {
  auto tmpVar = freshTmp();
  exp.lhs().Visit(this);
  insns.push_back(Insn("movl", EAX, O{-(*tmpVar), EBP}));
  exp.rhs().Visit(this);
  insns.push_back(Insn("movl", O{-(*tmpVar), EBP}, EDX));

  // LHS is at EDX, RHS is at EAX
  insns.push_back("  cmp %eax, %edx");
  insns.push_back("  sete %al");
  insns.push_back("  movzbl %al, %eax");
}
void CodeGen::VisitLogicalAndExpr(const LogicalAndExpr& exp) {
  auto tmpVar = freshTmp();
  exp.lhs().Visit(this);
  insns.push_back(Insn("movl", EAX, O{-(*tmpVar), EBP}));
  exp.rhs().Visit(this);
  insns.push_back(Insn("movl", O{-(*tmpVar), EBP}, EDX));

  // LHS is at EDX, RHS is at EAX
  insns.push_back(Insn("andl", EDX, EAX));
}
void CodeGen::VisitLogicalOrExpr(const LogicalOrExpr& exp) {
  auto tmpVar = freshTmp();
  exp.lhs().Visit(this);
  insns.push_back(Insn("movl", EAX, O{-(*tmpVar), EBP}));
  exp.rhs().Visit(this);
  insns.push_back(Insn("movl", O{-(*tmpVar), EBP}, EDX));

  // LHS is at EDX, RHS is at EAX
  insns.push_back(Insn("orl", EDX, EAX));
}
void CodeGen::VisitLogicalNotExpr(const LogicalNotExpr& exp) {
  exp.operand().Visit(this);
  // Result is in eax, use branch-free not implementation
  insns.push_back("  cmp $0, %eax");
  insns.push_back("  sete %al");
  insns.push_back("  movzbl %al, %eax");
}
void CodeGen::VisitTypeExpr(const TypeExpr& exp) {
  // No code is generated for types
}
void CodeGen::VisitBlockStmt(const BlockStmt& exp) {
  // Save whether we were in global scope
  bool wasInTopLevelScope = inTopLevelScope;
  inTopLevelScope = false;

  // Reserve stack space
  auto stackSize = C{static_cast<int32_t>(exp.decls().size()) * 4};
  insns.push_back(Insn("sub", stackSize, ESP));

  // To simplify garbage collection, we do not allow variables
  // declared in inner scopes in L2
  if (!wasInTopLevelScope && stackSize.value > 0) {
    throw CodeGenError {"Local variables in inner scopes are not allowed in L2"};
  }

  // Create new scope in symbol table
  symbolTable.openScope();

  // Insert declared variables to symbol table and initialize them to 0
  for (auto & d : exp.decls()) {
    d.Visit(this);
    insns.push_back(Insn("movl", C{0}, O{-(symbolTable.ctx.lookup(d.id().name())->first), EBP}));
  }
  // Generate code for the statements, note that this may create additional temporaries
  for (auto & s : exp.stmts()) {
    s->Visit(this);
  }

  // Adjust the stack size back if we are not in global scope
  if (!wasInTopLevelScope) {
    insns.push_back(Insn("add", stackSize, ESP));
    symbolTable.closeScope();
  }
  // Restore global status
  inTopLevelScope = wasInTopLevelScope;
}

void CodeGen::VisitDeclarationExpr(const Declaration& exp) {
  symbolTable.allocateVar(exp.id().name(), exp.type().name());
}

void CodeGen::VisitAssignmentExpr(const Assignment& assignment) {
  // We are generating code for the right-hand sides first so there
  // won't be any dangling references in case GC kicks in when
  // evaluating RHS (this is the case for allocation & function calls
  // but we are doing the same for assignments as well to keep the
  // code gen uniform).
  
  // generate code for rhs, this will put the result in EAX
  assignment.rhs().Visit(this);
  // create a temporary and save the result
  auto tmpVar = freshTmp();
  insns.push_back(Insn("movl", EAX, O{-(*tmpVar), EBP}));
  // resolve address of LHS
  inLhsOfAssignment = true;
  assignment.lhs().Visit(this);
  inLhsOfAssignment = false;
  // Address of LHS should be in EAX
  //
  // move the result from the temporary to the lhs
  insns.push_back(Insn("movl", O{-(*tmpVar), EBP}, EDX));
  insns.push_back(Insn("movl", EDX, O{0, EAX}));
}

void CodeGen::VisitConditionalExpr(const Conditional& conditional) {
  auto n = std::to_string(freshIndex());
  auto falseLabel = L{"IF_FALSE_" + n};
  auto endLabel = L{"IF_END_" + n};
  conditional.guard().Visit(this);
  insns.push_back(Insn("cmp", C{0}, EAX));
  insns.push_back(Insn("je", falseLabel));
  conditional.true_branch().Visit(this);
  insns.push_back(Insn("jmp", endLabel));
  insns.push_back(falseLabel.value + ":");
  conditional.false_branch().Visit(this);
  insns.push_back(endLabel.value + ":");
}
void CodeGen::VisitLoopExpr(const Loop& loop) {
  auto n = std::to_string(freshIndex());
  auto startLabel = L{"WHILE_START_" + n};
  auto endLabel = L{"WHILE_END_" + n};
  insns.push_back(startLabel.value + ":");
  loop.guard().Visit(this);
  insns.push_back(Insn("cmp", C{0}, EAX));
  insns.push_back(Insn("je", endLabel));
  loop.body().Visit(this);
  insns.push_back(Insn("jmp", startLabel));
  insns.push_back(endLabel.value + ":");
}

void CodeGen::VisitFunctionCallExpr(const FunctionCall& call) {
  // check if the arities match
  auto arity = symbolTable.getArity(call.callee_name());
  if (arity != call.arguments().size()) {
    throw CodeGenError { std::string("The function ") + call.callee_name() + " expects " + std::to_string(arity) + " arguments but " + std::to_string(call.arguments().size()) + " arguments are given" };
  }

  insns.push_back("  // CALL " + call.callee_name());
  
  // create temporaries for each argument, the code to deallocate them will be created when we are done.
  auto stackSpace = static_cast<int32_t>(call.arguments().size() * 4);

  // compute and push the arguments in reverse order
  for (auto arg = call.arguments().rbegin(), end = call.arguments().rend(); arg != end; ++arg) {
    // code to compute the argument
    (*arg)->Visit(this);
    // push the argument    
    insns.push_back(Insn("push", EAX));
    // increse the used stack space
    symbolTable.ctx.nextOffset += 4;
  }

  // call the function
  insns.push_back(Insn("call", L{call.callee_name()}));
  // free the stack space
  insns.push_back("  // POST-RETURN");
  insns.push_back(Insn("add", C{stackSpace}, ESP));
  symbolTable.ctx.nextOffset -= stackSpace;
}

void CodeGen::VisitFunctionDefExpr(const FunctionDef& def) {
  uint32_t argInfo = 0;
  size_t i = 0;
  for (auto & [type, _] : def.parameters()) {
    if (! type->isIntType()) {
      argInfo |= 1 << i;
    }
    ++i;
  }
  
  uint32_t localsInfo = 0;
  i = 0;
  for (auto & decl : def.function_body().decls()) {
    if (! decl.type().isIntType()) {
      localsInfo |= 1 << i;
    }
    ++i;
  }

  symbolTable.resetLocalsInfo();
  insns.push_back(def.function_name() + ":");
  // prologue
  insns.push_back("  // FUNCTION PROLOGUE");
  // save the stack frame
  insns.push_back(Insn("push", EBP));
  insns.push_back(Insn("movl", ESP, EBP));
  // push argument and local info
  insns.push_back("  // ARGUMENT INFO");
  insns.push_back(Insn("pushl", H{argInfo}));
  insns.push_back("  // LOCAL INFO");
  insns.push_back(Insn("pushl", H{localsInfo}));
  // end prologue
  insns.push_back("  // BODY");

  // add parameters to current context
  symbolTable.openScope();

  int32_t currentParamOffset = -8;
  for (auto & [type, param] : def.parameters()) {
    // we use offsets in the other direction (as if the stack is
    // growing up), we negate it when constructing offset arguments
    // (objects of `O` class)
    symbolTable.ctx.varInfo.insert({param.name(), VarInfo{currentParamOffset, type->name()}});
    currentParamOffset -= 4;
  }

  // generate code for the body
  def.function_body().Visit(this);
  // generate code for the return expression, the result will be in EAX conveniently
  def.retval().Visit(this);

  // free stack space
  auto stackSize = static_cast<int32_t>(def.function_body().decls().size()) * 4;
  insns.push_back(Insn("add", C{stackSize}, ESP));

  // epilogue
  insns.push_back("  // EPILOGUE");
  // restore the stack frame
  insns.push_back(Insn("movl", EBP, ESP));
  insns.push_back(Insn("pop", EBP));
  // return
  insns.push_back(Insn("ret"));
  // end epilogue
  insns.push_back("  // END OF " + def.function_name());
  insns.push_back("");

  // remove parameters from current context
  symbolTable.closeScope();
}

void CodeGen::VisitTypeDef(const TypeDef& def) {
  // Add this type definition to symbol table
  if (symbolTable.typeInfo.count(def.type_name()) != 0) {
    throw CodeGenError { "Type " + def.type_name() + " is already defined" };
  }
  
  std::vector<std::pair<std::string, std::string>> fields;

  for (auto & decl : def.fields()) {
    fields.push_back({decl.id().name(), decl.type().name()});
  }
  
  symbolTable.typeInfo.emplace(std::string(def.type_name()), TypeInfo{std::string(def.type_name()), std::move(fields)});
}

void CodeGen::VisitProgramExpr(const Program& program) {
  // fill the type definitions
  for (const auto & typeDef : program.type_defs()) {
    typeDef.Visit(this);
  }

  // fill the function symbol table
  for (const auto & fnDef : program.function_defs()) {
    symbolTable.addFnDef(*fnDef);
  }

  // generate definitions
  for (const auto & fnDef : program.function_defs()) {
    fnDef->Visit(this);
  }

  uint32_t localsInfo = 0;
  size_t i = 0;
  for (auto & decl : program.statements().decls()) {
    if (! decl.type().isIntType()) {
      localsInfo |= 1 << i;
    }
    ++i;
  }

  insns.push_back("  .globl Entry");
  insns.push_back("  .type Entry, @function");
  insns.push_back("Entry:");
  //program entry prologue
  insns.push_back("  // BOOTSTRAP ENTRY");
  insns.push_back(Insn("push", EBP));
  insns.push_back(Insn("movl", ESP, EBP));
  insns.push_back("  // ARGUMENT INFO");
  insns.push_back(Insn("pushl", H{0}));
  insns.push_back("  // LOCAL INFO");
  insns.push_back(Insn("pushl", H{localsInfo}));
  //end prologue
  insns.push_back("");
  insns.push_back("  // MAIN PROGRAM STATEMENTS");
  program.statements().Visit(this);
  insns.push_back("");
  insns.push_back("  // OUTPUT EXPRESSION");
  program.arithmetic_exp().Visit(this);
  // free stack space
  auto stackSize = static_cast<int32_t>(program.statements().decls().size()) * 4;
  insns.push_back(Insn("add", C{stackSize}, ESP));
  // program exit epilogue
  insns.push_back(Insn("movl", EBP, ESP));
  insns.push_back(Insn("pop", EBP));
  insns.push_back(Insn("ret"));
}

}  // namespace cs160::backend
