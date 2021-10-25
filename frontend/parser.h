#pragma once
#include <optional>
#include <stdexcept>
#include <vector>
#include "frontend/ast.h"
#include "frontend/token.h"

namespace cs160::frontend {

// This is meant as a general error
struct InvalidASTError : public std::runtime_error {
  InvalidASTError() : runtime_error("Invalid AST created") {}
  InvalidASTError(const std::string & message) : runtime_error("Invalid AST created: " + message) {}
};

// This is the parser class you need to implement. The function you need to
// implement is the parsify() method. You can define other class members such as
// fields or helper functions.
class Parser final {
 public:
  // The entry point of the parser you need to implement. It takes the output of
  // the lexer as the argument and produces an abstract syntax tree as the
  // result of parsing the tokens.
  Parser(const std::vector<Token> &lexer_tokens) : tokens(lexer_tokens) {}

  std::optional<Token> nextToken(int peek = 1);
  Token matchToken(const TokenType &);

  std::unique_ptr<const AccessPath> parseAccessPath();
  Variable parseVariable();
  IntegerExprP parseIntegerExpr();

  ArithmeticExprP parseArithmeticExpr();
  std::pair<ArithmeticExprP, std::optional<Token>> parseAExpPrime();
  ArithmeticExprP parseATerm();
  ArithmeticExprP parseATermPrime();
  ArithmeticExprP parseAFactor();

  RelationalExprP parseRexp();
  RelationalExprP parseRexpPrime1();
  std::pair<RelationalExprP, std::optional<Token>> parseRexpPrime2();

  Declaration parseDeclaration();
  Declaration::Block parseDecls();

  // TypeExprP parseTypeExpe();

  StatementP parseStatementP();
  LoopExprP parseLoopExprP();
  ConditionalExprP parseCondExprP();
  AssignmentExprP parseAssignmentExprP();
  Statement::Block parseStmts();
  BlockStmtP parseBlockStmt();

  std::vector<std::pair<std::unique_ptr<const TypeExpr>,
                        Variable>>
  parseParams();
  std::vector<std::pair<std::unique_ptr<const TypeExpr>,
                        Variable>>
  parseOptParams();
  // std::vector<DeclarationExprP> parseOptParams();

  std::vector<ArithmeticExprP> parseFunArgs();
  FunctionDefP parseFunDef();
  FunctionDef::Block parseFunDefs();

  TypeDef parseTypeDef();
  TypeDef::Block parseTypeDefs();

  FunctionCallP parseFunCall();

  ProgramExprP parse();

 private:
  const std::vector<Token> tokens;
  int head = -1;  // we start at one before the token list
};
};  // namespace cs160::frontend
