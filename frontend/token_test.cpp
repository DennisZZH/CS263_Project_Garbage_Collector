#define CATCH_CONFIG_MAIN

#include "frontend/token.h"
#include "catch2/catch.hpp"

using namespace cs160::frontend;

TEST_CASE("Builder methods", "[token]") {
  SECTION("makeId") {
    auto tok = Token::makeId("x");
    REQUIRE(tok.type() == TokenType::Id);
    REQUIRE(tok.stringValue() == "x");
  }

  SECTION("makeNum(int value)") {
    auto tok = Token::makeNum(3);
    REQUIRE(tok.type() == TokenType::Num);
    REQUIRE(tok.intValue() == 3);
  }

  SECTION("makeType(std::string name)") {
    auto tok = Token::makeType("int");
    REQUIRE(tok.type() == TokenType::Type);
    REQUIRE(tok.stringValue() == "int");
  }

  SECTION("makeIf()") {
    auto tok = Token::makeIf();
    REQUIRE(tok.type() == TokenType::If);
  }

  SECTION("makeElse()") {
    auto tok = Token::makeElse();
    REQUIRE(tok.type() == TokenType::Else);
  }

  SECTION("makeWhile()") {
    auto tok = Token::makeWhile();
    REQUIRE(tok.type() == TokenType::While);
  }

  SECTION("makeDef()") {
    auto tok = Token::makeDef();
    REQUIRE(tok.type() == TokenType::Def);
  }

  SECTION("makeReturn()") {
    auto tok = Token::makeReturn();
    REQUIRE(tok.type() == TokenType::Return);
  }

  SECTION("makeOutput()") {
    auto tok = Token::makeOutput();
    REQUIRE(tok.type() == TokenType::Output);
  }

  SECTION("makeArithOp(ArithOp op)") {
    auto tok = Token::makeArithOp(ArithOp::Minus);
    REQUIRE(tok.type() == TokenType::ArithOp);
    REQUIRE(tok.arithOpValue() == ArithOp::Minus);
  }

  SECTION("makeRelOp(RelOp op)") {
    auto tok = Token::makeRelOp(RelOp::Equal);
    REQUIRE(tok.type() == TokenType::RelOp);
    REQUIRE(tok.relOpValue() == RelOp::Equal);
  }

  SECTION("makeLBinOp(LBinOp op)") {
    auto tok = Token::makeLBinOp(LBinOp::And);
    REQUIRE(tok.type() == TokenType::LBinOp);
    REQUIRE(tok.logicBinOpValue() == LBinOp::And);
  }

  SECTION("makeLNeg()") {
    auto tok = Token::makeLNeg();
    REQUIRE(tok.type() == TokenType::LNeg);
  }

  SECTION("makeLParen()") {
    auto tok = Token::makeLParen();
    REQUIRE(tok.type() == TokenType::LParen);
  }

  SECTION("makeRParen()") {
    auto tok = Token::makeRParen();
    REQUIRE(tok.type() == TokenType::RParen);
  }

  SECTION("makeLBrace()") {
    auto tok = Token::makeLBrace();
    REQUIRE(tok.type() == TokenType::LBrace);
  }

  SECTION("makeRBrace()") {
    auto tok = Token::makeRBrace();
    REQUIRE(tok.type() == TokenType::RBrace);
  }

  SECTION("makeSemicolon()") {
    auto tok = Token::makeSemicolon();
    REQUIRE(tok.type() == TokenType::Semicolon);
  }

  SECTION("makeAssign()") {
    auto tok = Token::makeAssign();
    REQUIRE(tok.type() == TokenType::Assign);
  }

  SECTION("makeHasType()") {
    auto tok = Token::makeHasType();
    REQUIRE(tok.type() == TokenType::HasType);
  }

  SECTION("makeComma()") {
    auto tok = Token::makeComma();
    REQUIRE(tok.type() == TokenType::Comma);
  }

  SECTION("makeDot()") {
    auto tok = Token::makeDot();
    REQUIRE(tok.type() == TokenType::Dot);
  }

  SECTION("makeNil()") {
    auto tok = Token::makeNil();
    REQUIRE(tok.type() == TokenType::Nil);
  }

  SECTION("makeStruct()") {
    auto tok = Token::makeStruct();
    REQUIRE(tok.type() == TokenType::Struct);
  }

  SECTION("makeNew()") {
    auto tok = Token::makeNew();
    REQUIRE(tok.type() == TokenType::New);
  }
}

TEST_CASE("equality", "[token]") {
  // Test equality over tokens
  auto makeTokenFixture = []() -> std::vector<Token> {
    return std::vector{Token::makeId("x"),
                       Token::makeId("y"),
                       Token::makeNum(3),
                       Token::makeNum(-42),
                       Token::makeType("int"),
                       Token::makeType("uint"),
                       Token::makeIf(),
                       Token::makeElse(),
                       Token::makeWhile(),
                       Token::makeDef(),
                       Token::makeReturn(),
                       Token::makeOutput(),
                       Token::makeArithOp(ArithOp::Plus),
                       Token::makeArithOp(ArithOp::Minus),
                       Token::makeRelOp(RelOp::LessThan),
                       Token::makeRelOp(RelOp::Equal),
                       Token::makeLBinOp(LBinOp::And),
                       Token::makeLBinOp(LBinOp::Or),
                       Token::makeLNeg(),
                       Token::makeLParen(),
                       Token::makeRParen(),
                       Token::makeLBrace(),
                       Token::makeRBrace(),
                       Token::makeSemicolon(),
                       Token::makeAssign(),
                       Token::makeHasType(),
                       Token::makeDot(),
                       Token::makeNil(),
                       Token::makeStruct(),
                       Token::makeNew(),
                       Token::makeComma()};
  };

  auto tokens1 = makeTokenFixture();
  auto tokens2 = makeTokenFixture();

  for (size_t i = 0; i < tokens1.size(); ++i) {
    for (size_t j = 0; j < tokens2.size(); ++j) {
      if (i == j) {
        REQUIRE(tokens1[i] == tokens2[j]);
        REQUIRE(!(tokens1[i] != tokens2[j]));
      } else {
        REQUIRE(tokens1[i] != tokens2[j]);
        REQUIRE(!(tokens1[i] == tokens2[j]));
      }
    }
  }
}

TEST_CASE("toString", "[token]") {
  CHECK(Token::makeId("x").toString() == "<Id,x>");
  CHECK(Token::makeId("y").toString() == "<Id,y>");
  CHECK(Token::makeNum(3).toString() == "<Num,3>");
  CHECK(Token::makeNum(-42).toString() == "<Num,-42>");
  CHECK(Token::makeType("int").toString() == "<Type,int>");
  CHECK(Token::makeType("uint").toString() == "<Type,uint>");
  CHECK(Token::makeIf().toString() == "<If>");
  CHECK(Token::makeElse().toString() == "<Else>");
  CHECK(Token::makeWhile().toString() == "<While>");
  CHECK(Token::makeDef().toString() == "<Def>");
  CHECK(Token::makeReturn().toString() == "<Return>");
  CHECK(Token::makeOutput().toString() == "<Output>");
  CHECK(Token::makeArithOp(ArithOp::Plus).toString() == "<ArithOp,+>");
  CHECK(Token::makeArithOp(ArithOp::Minus).toString() == "<ArithOp,->");
  CHECK(Token::makeRelOp(RelOp::LessThan).toString() == "<RelOp,<>");
  CHECK(Token::makeRelOp(RelOp::Equal).toString() == "<RelOp,=>");
  CHECK(Token::makeLBinOp(LBinOp::And).toString() == "<LBinOp,&&>");
  CHECK(Token::makeLBinOp(LBinOp::Or).toString() == "<LBinOp,||>");
  CHECK(Token::makeLNeg().toString() == "<LNeg>");
  CHECK(Token::makeLParen().toString() == "<LParen>");
  CHECK(Token::makeRParen().toString() == "<RParen>");
  CHECK(Token::makeLBrace().toString() == "<LBrace>");
  CHECK(Token::makeRBrace().toString() == "<RBrace>");
  CHECK(Token::makeSemicolon().toString() == "<Semicolon>");
  CHECK(Token::makeAssign().toString() == "<Assign>");
  CHECK(Token::makeHasType().toString() == "<HasType>");
  CHECK(Token::makeComma().toString() == "<Comma>");
  CHECK(Token::makeDot().toString() == "<Dot>");
  CHECK(Token::makeNew().toString() == "<New>");
  CHECK(Token::makeNil().toString() == "<Nil>");
  CHECK(Token::makeStruct().toString() == "<Struct>");
}
