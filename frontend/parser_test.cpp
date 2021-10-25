#define CATCH_CONFIG_MAIN

#include "frontend/parser.h"
#include "catch2/catch.hpp"
#include "frontend/lexer.h"
#include "frontend/token.h"

using namespace cs160::frontend;
using Catch::Matchers::Equals;
using Catch::Matchers::Message;

TEST_CASE("Basic coverage tests", "[parser]") {
  // integer expression: 4
  auto parsed =
      Parser{std::vector<Token>{Token::makeOutput(), Token::makeNum(4),
                                Token::makeSemicolon()}}
  .parse();

  Declaration::Block di;
  std::vector<std::unique_ptr<const Statement>> si;
  auto expected =
      Program(TypeDef::Block(),
              FunctionDef::Block(),
              std::make_unique<const BlockStmt>(std::move(di), std::move(si)),
              std::make_unique<const IntegerExpr>(4));
  REQUIRE(parsed->toString() == expected.toString());

  //  addition expression: 1+2
  auto parsed_plus =
      Parser{std::vector<Token>{Token::makeOutput(), Token::makeNum(1),
                                Token::makeArithOp(ArithOp::Plus),
                                Token::makeNum(2), Token::makeSemicolon()}}
          .parse();

  std::vector<std::unique_ptr<const Statement>> sp;
  auto expected_plus = Program(
      TypeDef::Block(),
      FunctionDef::Block(),
      std::make_unique<const BlockStmt>(Declaration::Block{}, std::move(sp)),
      std::make_unique<const AddExpr>(std::make_unique<const IntegerExpr>(1),
                                      std::make_unique<const IntegerExpr>(2)));
  REQUIRE(parsed_plus->toString() == expected_plus.toString());

  //
  //  assignment: x:=4; output x;
  auto parsed_assign =
      Parser{std::vector<Token>{Token::makeId("x"), Token::makeAssign(),
                                Token::makeNum(4), Token::makeSemicolon(),
                                Token::makeOutput(), Token::makeId("x"),
                                Token::makeSemicolon()}}
          .parse();

  Declaration::Block da;
  std::vector<std::unique_ptr<const Statement>> sa;
  sa.push_back(std::make_unique<const Assignment>(
      std::make_unique<const AccessPath>(Variable{"x"}),
      std::make_unique<const IntegerExpr>(4)));
  auto expected_assign =
      Program(TypeDef::Block(), FunctionDef::Block(),
              std::make_unique<const BlockStmt>(Declaration::Block{}, std::move(sa)),
              std::make_unique<const AccessPath>(Variable{"x"}));
  REQUIRE(parsed_assign->toString() == expected_assign.toString());

  //
  //  conditional: if (1<2) {} output 4;
  auto parsed_cond =
      Parser{std::vector<Token>{
                 Token::makeIf(), Token::makeLParen(), Token::makeNum(1),
                 Token::makeRelOp(RelOp::LessThan), Token::makeNum(2),
                 Token::makeRParen(), Token::makeLBrace(),
                 Token::makeRBrace(),  // nothing doing
                 Token::makeOutput(), Token::makeNum(4),
                 Token::makeSemicolon()}}
          .parse();

  Declaration::Block dc;
  std::vector<std::unique_ptr<const Statement>> sc;
  Declaration::Block true_branch_decls;
  std::vector<std::unique_ptr<const Statement>> true_branch_stmts;
  Declaration::Block false_branch_decls;
  std::vector<std::unique_ptr<const Statement>> false_branch_stmts;
  sc.push_back(std::make_unique<const Conditional>(
      std::make_unique<const LessThanExpr>(
          std::make_unique<const IntegerExpr>(1),
          std::make_unique<const IntegerExpr>(2)),
      std::make_unique<const BlockStmt>(std::move(true_branch_decls),
                                        std::move(true_branch_stmts)),
      std::make_unique<const BlockStmt>(std::move(false_branch_decls),
                                        std::move(false_branch_stmts))));
  auto expected_cond =
      Program(TypeDef::Block(), FunctionDef::Block(),
              std::make_unique<const BlockStmt>(std::move(dc), std::move(sc)),
              std::make_unique<const IntegerExpr>(4));
  REQUIRE(parsed_cond->toString() == expected_cond.toString());

  //
  //  loop: while (1<2) {} output 4;
  auto parsed_loop =
      Parser{std::vector<Token>{
                 Token::makeWhile(), Token::makeLParen(), Token::makeNum(1),
                 Token::makeRelOp(RelOp::LessThan), Token::makeNum(2),
                 Token::makeRParen(), Token::makeLBrace(),
                 Token::makeRBrace(),  // no statements
                 Token::makeOutput(), Token::makeNum(4),
                 Token::makeSemicolon()}}
          .parse();

  Declaration::Block dl;
  std::vector<std::unique_ptr<const Statement>> sl;
  Declaration::Block loop_body_decls;
  std::vector<std::unique_ptr<const Statement>> loop_body_stmts;
  sl.push_back(std::make_unique<const Loop>(
      std::make_unique<const LessThanExpr>(
          std::make_unique<const IntegerExpr>(1),
          std::make_unique<const IntegerExpr>(2)),
      std::make_unique<const BlockStmt>(std::move(loop_body_decls),
                                        std::move(loop_body_stmts))));
  auto expected_loop =
      Program(TypeDef::Block(), FunctionDef::Block(),
              std::make_unique<const BlockStmt>(std::move(dl), std::move(sl)),
              std::make_unique<const IntegerExpr>(4));
  REQUIRE(parsed_loop->toString() == expected_loop.toString());

  //
  //  declaration: int x; output 4;
  auto parsed_declaration =
      Parser{std::vector<Token>{Token::makeType("int"), Token::makeId("x"),
                                Token::makeSemicolon(), Token::makeOutput(),
                                Token::makeNum(4), Token::makeSemicolon()}}
          .parse();

  Declaration::Block dd;
  std::vector<std::unique_ptr<const Statement>> sd;
  dd.push_back(Declaration{TypeExpr{"int"}, Variable{"x"}});
  auto expected_declaration =
      Program(TypeDef::Block(), FunctionDef::Block(),
              std::make_unique<const BlockStmt>(std::move(dd), std::move(sd)),
              std::make_unique<const IntegerExpr>(4));
  REQUIRE(parsed_declaration->toString() == expected_declaration.toString());

  //
  // function def: def f() :int { return 4; } output 4;
  auto parsed_fdef =
      Parser{std::vector<Token>{Token::makeDef(), Token::makeId("f"),
                                Token::makeLParen(), Token::makeRParen(),
                                Token::makeHasType(), Token::makeType("int"),
                                Token::makeLBrace(), Token::makeReturn(),
                                Token::makeNum(4), Token::makeSemicolon(),
                                Token::makeRBrace(), Token::makeOutput(),
                                Token::makeNum(4), Token::makeSemicolon()}}
          .parse();

  Declaration::Block empty_defs;
  std::vector<std::unique_ptr<const Statement>> empty_stmts;
  FunctionDef::Block fdef_body;
  std::vector<std::pair<std::unique_ptr<const TypeExpr>,
                        Variable>>
      parameters;
  fdef_body.push_back(std::make_unique<const FunctionDef>(
      "f",                                // function name
      std::make_unique<const TypeExpr>("int"),  // return type
      std::move(parameters),              // no parameters
      std::make_unique<const BlockStmt>(
          std::move(empty_defs), std::move(empty_stmts)),  // no function body
      std::make_unique<const IntegerExpr>(4)));            // return value

  auto expected_fdef =
      Program(TypeDef::Block(), std::move(fdef_body),
              std::make_unique<const BlockStmt>(std::move(empty_defs),
                                                std::move(empty_stmts)),
              std::make_unique<const IntegerExpr>(4));
  REQUIRE(parsed_fdef->toString() == expected_fdef.toString());
}

TEST_CASE("Type definition test", "[parser]") {
  //
  // type and function def: struct %list { int value; %list next; } def f() :int { return 4; } output 4;
  auto parsed_fdef =
      Parser{std::vector<Token>{
      Token::makeStruct(),
      Token::makeType("%list"),
      Token::makeLBrace(),
      Token::makeType("int"), Token::makeId("value"), Token::makeSemicolon(),
      Token::makeType("%list"), Token::makeId("next"), Token::makeSemicolon(),
      Token::makeRBrace(), Token::makeSemicolon(),
      Token::makeDef(), Token::makeId("f"),
      Token::makeLParen(), Token::makeRParen(),
      Token::makeHasType(), Token::makeType("int"),
      Token::makeLBrace(), Token::makeReturn(),
      Token::makeNum(4), Token::makeSemicolon(),
      Token::makeRBrace(), Token::makeOutput(),
      Token::makeNum(4), Token::makeSemicolon()}}
  .parse();

  Declaration::Block empty_defs;
  std::vector<std::unique_ptr<const Statement>> empty_stmts;
  FunctionDef::Block fdef_body;
  std::vector<std::pair<std::unique_ptr<const TypeExpr>,
                        Variable>>
      parameters;
  fdef_body.push_back(std::make_unique<const FunctionDef>(
      "f",                                // function name
      std::make_unique<const TypeExpr>("int"),  // return type
      std::move(parameters),              // no parameters
      std::make_unique<const BlockStmt>(
          std::move(empty_defs), std::move(empty_stmts)),  // no function body
      std::make_unique<const IntegerExpr>(4)));            // return value

  auto typeDef = TypeDef{"%list", {
      Declaration{TypeExpr{"int"}, Variable{"value"}},
      Declaration{TypeExpr{"%list"}, Variable{"next"}},
    }};

  auto expected_fdef =
      Program(TypeDef::Block{std::move(typeDef)}, std::move(fdef_body),
              std::make_unique<const BlockStmt>(std::move(empty_defs),
                                                std::move(empty_stmts)),
              std::make_unique<const IntegerExpr>(4));
  REQUIRE(parsed_fdef->toString() == expected_fdef.toString());
}

TEST_CASE("Simple invalid parser tests", "[Parser{}]") {
  auto tok = std::vector<Token>{
      Token::makeId("x"), Token::makeArithOp(ArithOp::Plus),
      Token::makeArithOp(ArithOp::Times), Token::makeId("y")};
  REQUIRE_THROWS_AS(Parser{tok}.parse(), InvalidASTError);
}
