#define CATCH_CONFIG_MAIN

#include "catch2/catch.hpp"
#include "frontend/parser.h"

using namespace cs160::frontend;

TEST_CASE("Builder methods", "[ast]") {
  SECTION("makeVar") {
    auto tok = std::make_unique<const Variable>("x");
    REQUIRE(tok->name() == "x");
  }

  SECTION("makeInt") {
    auto tok = std::make_unique<const IntegerExpr>(4);
    REQUIRE(tok->value() == 4);
  }
}

TEST_CASE("toString", "[ast]") {
  Parser p({});
  auto ast = std::make_unique<const Program>(
      FunctionDef::Block(), Statement::Block(),
      std::move(std::make_unique<const Variable>("x")));
  CHECK(p.toString(std::move(ast)) == "x");
}
