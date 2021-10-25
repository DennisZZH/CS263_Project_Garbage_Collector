#define CATCH_CONFIG_MAIN

#include "frontend/lexer.h"
#include "catch2/catch.hpp"

using namespace cs160::frontend;
using Catch::Matchers::Equals;
using Catch::Matchers::Message;

TEST_CASE("Single-token tests", "[lexer]") {
  CHECK_THAT(Lexer{}.tokenize("x"), Equals(std::vector{Token::makeId("x")}));
  CHECK_THAT(Lexer{}.tokenize("y"), Equals(std::vector{Token::makeId("y")}));
  CHECK_THAT(Lexer{}.tokenize("3"), Equals(std::vector{Token::makeNum(3)}));
  CHECK_THAT(Lexer{}.tokenize("-42"), Equals(std::vector{Token::makeNum(-42)}));
  CHECK_THAT(Lexer{}.tokenize("int"),
             Equals(std::vector{Token::makeType("int")}));
  CHECK_THAT(Lexer{}.tokenize("uint"),
             Equals(std::vector{Token::makeId("uint")}));
  CHECK_THAT(Lexer{}.tokenize("if"), Equals(std::vector{Token::makeIf()}));
  CHECK_THAT(Lexer{}.tokenize("else"), Equals(std::vector{Token::makeElse()}));
  CHECK_THAT(Lexer{}.tokenize("while"),
             Equals(std::vector{Token::makeWhile()}));
  CHECK_THAT(Lexer{}.tokenize("def"), Equals(std::vector{Token::makeDef()}));
  CHECK_THAT(Lexer{}.tokenize("return"),
             Equals(std::vector{Token::makeReturn()}));
  CHECK_THAT(Lexer{}.tokenize("output"),
             Equals(std::vector{Token::makeOutput()}));
  CHECK_THAT(Lexer{}.tokenize("+"),
             Equals(std::vector{Token::makeArithOp(ArithOp::Plus)}));
  CHECK_THAT(Lexer{}.tokenize("-"),
             Equals(std::vector{Token::makeArithOp(ArithOp::Minus)}));
  CHECK_THAT(Lexer{}.tokenize("*"),
             Equals(std::vector{Token::makeArithOp(ArithOp::Times)}));
  CHECK_THAT(Lexer{}.tokenize("<"),
             Equals(std::vector{Token::makeRelOp(RelOp::LessThan)}));
  CHECK_THAT(Lexer{}.tokenize("<="),
             Equals(std::vector{Token::makeRelOp(RelOp::LessEq)}));
  CHECK_THAT(Lexer{}.tokenize("="),
             Equals(std::vector{Token::makeRelOp(RelOp::Equal)}));
  CHECK_THAT(Lexer{}.tokenize("&&"),
             Equals(std::vector{Token::makeLBinOp(LBinOp::And)}));
  CHECK_THAT(Lexer{}.tokenize("||"),
             Equals(std::vector{Token::makeLBinOp(LBinOp::Or)}));
  CHECK_THAT(Lexer{}.tokenize("!"), Equals(std::vector{Token::makeLNeg()}));
  CHECK_THAT(Lexer{}.tokenize("("), Equals(std::vector{Token::makeLParen()}));
  CHECK_THAT(Lexer{}.tokenize(")"), Equals(std::vector{Token::makeRParen()}));
  CHECK_THAT(Lexer{}.tokenize("{"), Equals(std::vector{Token::makeLBrace()}));
  CHECK_THAT(Lexer{}.tokenize("}"), Equals(std::vector{Token::makeRBrace()}));
  CHECK_THAT(Lexer{}.tokenize(";"),
             Equals(std::vector{Token::makeSemicolon()}));
  CHECK_THAT(Lexer{}.tokenize(":="), Equals(std::vector{Token::makeAssign()}));
  CHECK_THAT(Lexer{}.tokenize(":"), Equals(std::vector{Token::makeHasType()}));
  CHECK_THAT(Lexer{}.tokenize(","), Equals(std::vector{Token::makeComma()}));
  CHECK_THAT(Lexer{}.tokenize("."), Equals(std::vector{Token::makeDot()}));
  CHECK_THAT(Lexer{}.tokenize("new"), Equals(std::vector{Token::makeNew()}));
  CHECK_THAT(Lexer{}.tokenize("nil"), Equals(std::vector{Token::makeNil()}));
  CHECK_THAT(Lexer{}.tokenize("struct"), Equals(std::vector{Token::makeStruct()}));
  CHECK_THAT(Lexer{}.tokenize("%foo"), Equals(std::vector{Token::makeType("%foo")}));
  CHECK_THAT(Lexer{}.tokenize("%foo %bar42 int%Q49uux"), Equals(std::vector{
        Token::makeType("%foo"),
        Token::makeType("%bar42"),
        Token::makeType("int"),
        Token::makeType("%Q49uux"),
      }));
}

TEST_CASE("Simple invalid token tests", "[Lexer{}]") {
  REQUIRE_THROWS_MATCHES(Lexer{}.tokenize("&"), InvalidLexemeError,
                         Message("Invalid lexeme in input program: & at position 0"));
  REQUIRE_THROWS_MATCHES(Lexer{}.tokenize("|"), InvalidLexemeError,
                         Message("Invalid lexeme in input program: | at position 0"));
  REQUIRE_THROWS_MATCHES(Lexer{}.tokenize(">"), InvalidLexemeError,
                         Message("Invalid lexeme in input program: > at position 0"));
  REQUIRE_THROWS_MATCHES(Lexer{}.tokenize("%"), InvalidLexemeError,
                         Message("Invalid lexeme in input program: % at position 0"));
}

TEST_CASE("Simple whitespace and comment tests", "[Lexer{}]") {
  CHECK_THAT(Lexer{}.tokenize("x ("),
             Equals(std::vector{Token::makeId("x"), Token::makeLParen()}));
  CHECK_THAT(Lexer{}.tokenize("\nwhile//\n;"),
             Equals(std::vector{Token::makeWhile(), Token::makeSemicolon()}));
  CHECK_THAT(Lexer{}.tokenize("  def\n\nif"),
             Equals(std::vector{Token::makeDef(), Token::makeIf()}));
}
