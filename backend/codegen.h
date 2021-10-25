#pragma once

#include "frontend/ast.h"
#include "frontend/ast_visitor.h"
#include <string>
#include <vector>
#include <stdexcept>
#include <memory>
#include <unordered_map>
#include <optional>
#include <cstdint>

using namespace cs160::frontend;

namespace cs160::backend {

struct CodeGenError : public std::runtime_error {
  explicit CodeGenError(const std::string & message) : runtime_error(message) {}
};

using VarInfo = std::pair<int32_t, const std::string &>;

// A nested context containing variable information with lexical scoping
struct Context {
  std::unordered_map<std::string, VarInfo> varInfo;
  std::unique_ptr<Context> parent;
  // Information about the stack space and the current local variable context
  uint32_t nextOffset = 12;

  std::optional<VarInfo> lookup(std::string const & x);
};

// Function information that keeps track of argument and return types
struct FnInfo {
  std::vector<std::string> argTypes;
  std::string retType;
};

// Type information that keeps track of offsets and types of each field
struct TypeInfo {
  // Type name information is for debugging purposes
  const std::string name;
  // Fields are represented as pairs of variables and types
  const std::vector<std::pair<std::string, std::string>> fields;

  int32_t offsetOf(std::string const & field) const {
    return varInfoOf(field).first;
  }

  const std::string & typeOf(std::string const & field) const {
    return varInfoOf(field).second;
  }

  VarInfo varInfoOf(std::string const & field) const {
    for (size_t i = 0; i < fields.size(); ++i) {
      if (fields[i].first == field) {
        return VarInfo{static_cast<int32_t>(i), fields[i].second};
      }
    }

    throw std::logic_error { std::string("Field ") + field + " is not found in struct " + name };
  }

  // Compute the tag needed by GC
  uint32_t tag() const {
    auto tag = (uint32_t)fields.size() << 24;
    for (size_t i = 0; i < fields.size(); ++i) {
      if (fields[i].second != "int") {
        // the field is a pointer set the relevant bit in tag
        tag |= 1 << (i + 1);
      }
    }

    // set the mark bit so the GC will know that this is not a follow ptr
    tag |= 1;

    return tag;
  }
};

// Symbol table, implementation detail
struct SymbolTable {
  std::unordered_map<std::string, TypeInfo> typeInfo;
  std::unordered_map<std::string, FnInfo> fnInfo;

  Context ctx;
  static const std::string tmpPrefix;
  uint32_t nextTmp = 0;

  // Check and add function definition
  void addFnDef(const FunctionDef & fnDef);

  uint32_t getArity(const std::string & f);

  // Reset local variable information, used when entering a new function definition
  void resetLocalsInfo();

  // Allocate stack space for a new local variable
  void allocateVar(std::string x, const std::string & type);

  // Create a new scope, used when entering a block
  void openScope();

  // Clear the current scope and go back to the parent
  void closeScope();
};

// The code generator is implemented as an AST visitor that will generate the relevant pieces of code as it traverses a node
class CodeGen final : public AstVisitor {
 public:
  // Entry point of the code generator. This function should visit given program and return generated code as a list of instructions and labels.
  std::vector<std::string> generateCode(const Program & program);

  // Visitor functions
  void VisitNil(const NilExpr& exp) override;
  void VisitNewExpr(const NewExpr& exp) override;
  void VisitIntegerExpr(const IntegerExpr& exp) override;
  void VisitVariable(const Variable& exp) override;
  void VisitAccessPath(const AccessPath& exp) override;
  void VisitAddExpr(const AddExpr& exp) override;
  void VisitSubtractExpr(const SubtractExpr& exp) override;
  void VisitMultiplyExpr(const MultiplyExpr& exp) override;
  void VisitLessThanExpr(const LessThanExpr& exp) override;
  void VisitLessThanEqualToExpr(const LessThanEqualToExpr& exp) override;
  void VisitEqualToExpr(const EqualToExpr& exp) override;
  void VisitLogicalAndExpr(const LogicalAndExpr& exp) override;
  void VisitLogicalOrExpr(const LogicalOrExpr& exp) override;
  void VisitLogicalNotExpr(const LogicalNotExpr& exp) override;
  void VisitTypeExpr(const TypeExpr& exp) override;
  void VisitBlockStmt(const BlockStmt& exp) override;
  void VisitDeclarationExpr(const Declaration& exp) override;
  void VisitAssignmentExpr(const Assignment& assignment) override;
  void VisitConditionalExpr(const Conditional& conditional) override;
  void VisitLoopExpr(const Loop& loop) override;
  void VisitFunctionCallExpr(const FunctionCall& call) override;
  void VisitFunctionDefExpr(const FunctionDef& def) override;
  void VisitTypeDef(const TypeDef& def) override;
  void VisitProgramExpr(const Program& program) override;

 private:
  // Implementation details, remove in student solution

  // Next index for label generation
  uint32_t nextIndex = 0;
  
  uint32_t freshIndex() {
    return nextIndex++;
  }

  // Whether we are currently generating left hand-side of an
  // assignment. This flag is used for keeping the address for an
  // access path.
  bool inLhsOfAssignment = false;

  // List of instructions generated so far
  std::vector<std::string> insns;
  // Symbol table
  SymbolTable symbolTable;
  // A flag to check if we are in the global scope or top level of a function body, this is needed for the top-level variables to be alive until the argument of `output`/`return`.
  bool inTopLevelScope;

  // allocated temporary variable, this class is used with RAII to generate code to remove the temporary when it is longer used
  class TmpVar {
    std::string name;
    CodeGen & codegen;
   public:
    explicit TmpVar(const std::string & name, CodeGen &codegen);
    TmpVar(const TmpVar&) = delete;
    ~TmpVar();
    int32_t operator * () const;

    // We use this type for temporaries
    static const std::string IntT;
  };

  // Create a fresh temporary variable that is managed via RAII
  TmpVar freshTmp();
};

}
