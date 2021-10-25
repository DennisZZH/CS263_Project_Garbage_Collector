#pragma once

#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>

namespace cs160::frontend {

// Token types in our language L1.
//
// The "enum class" makes sure that the C++ compiler does not silently convert
// our enumerations to/from ints so we can catch bugs.
//
// It also encapsulates enumeration names so we use `TokenType::Id` not just
// `Id` for example.
enum class TokenType {
  Id = 1,
  Num,
  Type,
  If,
  Else,
  While,
  Def,
  Return,
  Output,
  ArithOp,
  RelOp,
  LBinOp,
  LNeg,
  LParen,
  RParen,
  LBrace,
  RBrace,
  LBracket,
  RBracket,
  Semicolon,
  Assign,
  HasType,
  Comma,
  Dot,
  Nil,
  Struct,
  New
};

// Give a string representation of given token type. Used for debugging
const char* tokenTypeToString(TokenType type_);

// Arithmetic operations we allow, used for keeping the exact operator inside
// ArithOp tokens.
enum class ArithOp { Plus, Minus, Times };

// Relational operations we allow, used for keeping the exact operator inside
// RelOp tokens.
enum class RelOp { LessThan, LessEq, Equal };

// Logical binary operations we allow, used for keeping the exact operator
// inside LBinOp tokens.
enum class LBinOp { And, Or };

// Error that is thrown when an operation needs a different token type than the
// one it expects to be given.
struct TokenMismatchError : public std::logic_error {
  explicit TokenMismatchError(const std::string& what_arg)
      : logic_error("Token mismatch error: " + what_arg) {}
};

// Tokens in our language L1. The lexer will output a sequence of Token objects.
class Token final {
 public:
  // BEGIN getters

  // Get type of this token
  TokenType type() const { return type_; }

  // Get string value inside the token. Throws TokenMismatchError if token type
  // is not an Id or Type
  const std::string& stringValue() const;
  // Get integer value inside the token. Throws TokenMismatchError if token type
  // is not Num.
  int intValue() const;
  // Get the relational operator inside the token. Throws TokenMismatchError if
  // token type is not RelOp.
  RelOp relOpValue() const;
  // Get the arithmetic operator inside the token. Throws TokenMismatchError if
  // token type is not ArithOp.
  ArithOp arithOpValue() const;
  // Get the logical operator inside the token. Throws TokenMismatchError if
  // token type is not LBinOp.
  LBinOp logicBinOpValue() const;
  // END getters

  // Convert this token to a printable string for debugging
  std::string toString() const;

  // BEGIN static methods to build tokens in a type-safe manner
  static Token makeId(std::string name);
  static Token makeNum(int value);
  static Token makeType(std::string name);
  static Token makeIf();
  static Token makeElse();
  static Token makeWhile();
  static Token makeDef();
  static Token makeReturn();
  static Token makeOutput();
  static Token makeArithOp(ArithOp op);
  static Token makeRelOp(RelOp op);
  static Token makeLBinOp(LBinOp op);
  static Token makeLNeg();
  static Token makeLParen();
  static Token makeRParen();
  static Token makeLBrace();
  static Token makeRBrace();
  static Token makeLBracket();
  static Token makeRBracket();
  static Token makeSemicolon();
  static Token makeAssign();
  static Token makeHasType();
  static Token makeComma();
  static Token makeDot();
  static Token makeNew();
  static Token makeNil();
  static Token makeStruct();
  // END static methods to build tokens in a type-safe manner

  // Equality operators
  bool operator==(const Token& that) const {
    // std::cout << tokenTypeToString(this->type_) << ' ' <<
    // tokenTypeToString(that.type_) << ' ' << this->value_ << ' ' << that.value_
    // << std::endl;
    return this->type_ == that.type_ && this->value_ == that.value_;
  }

  bool operator!=(const Token& that) const { return !(*this == that); }

 private:
  // BEGIN private constructors for different kinds of values a token may carry
  explicit Token(TokenType type);
  Token(TokenType type, std::string value);
  Token(TokenType type, int value);
  Token(TokenType type, RelOp value);
  Token(TokenType type, ArithOp value);
  Token(TokenType type, LBinOp value);
  // END private constructors

  // This is the type of the token.
  TokenType type_;

  // This is the value held inside the token, it depends on the token type.
  std::optional<std::variant<std::string, int, RelOp, ArithOp, LBinOp>> value_;

  friend class std::hash<Token>;
};

// Stream operator for printing
std::ostream& operator<<(std::ostream& out, const Token& tok);

}  // namespace cs160::frontend
