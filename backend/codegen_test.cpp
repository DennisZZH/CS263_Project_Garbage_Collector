#define CATCH_CONFIG_MAIN

#include "frontend/parser.h"
#include "catch2/catch.hpp"
#include "frontend/lexer.h"
#include "backend/codegen.h"

using namespace cs160::frontend;
using Catch::Matchers::Equals;
using Catch::Matchers::Message;

