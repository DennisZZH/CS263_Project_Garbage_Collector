#include "frontend/token.h"
#include <sstream>

namespace cs160::frontend {

const char* tokenTypeToString(TokenType type_) {
  switch (type_) {
    case TokenType::Id:
      return "Id";
    case TokenType::Num:
      return "Num";
    case TokenType::Type:
      return "Type";
    case TokenType::If:
      return "If";
    case TokenType::Else:
      return "Else";
    case TokenType::While:
      return "While";
    case TokenType::Def:
      return "Def";
    case TokenType::Return:
      return "Return";
    case TokenType::Output:
      return "Output";
    case TokenType::ArithOp:
      return "ArithOp";
    case TokenType::RelOp:
      return "RelOp";
    case TokenType::LBinOp:
      return "LBinOp";
    case TokenType::LNeg:
      return "LNeg";
    case TokenType::LParen:
      return "LParen";
    case TokenType::RParen:
      return "RParen";
    case TokenType::LBrace:
      return "LBrace";
    case TokenType::RBrace:
      return "RBrace";
    case TokenType::LBracket:
      return "LBracket";
    case TokenType::RBracket:
      return "RBracket";
    case TokenType::Semicolon:
      return "Semicolon";
    case TokenType::Assign:
      return "Assign";
    case TokenType::HasType:
      return "HasType";
    case TokenType::Comma:
      return "Comma";
    case TokenType::Dot:
      return "Dot";
    case TokenType::Nil:
      return "Nil";
    case TokenType::Struct:
      return "Struct";
    case TokenType::New:
      return "New";
    default:
      return "unknown or uninitialized token type";
  }
}

void expectTokenType(TokenType expected, TokenType actual) {
  if (expected != actual) {
    std::ostringstream message{"expected "};
    message << "expected " << tokenTypeToString(expected) << ", found "
            << tokenTypeToString(actual);
    throw TokenMismatchError{message.str()};
  }
}

const std::string& Token::stringValue() const {
  if (type_ != TokenType::Id && type_ != TokenType::Type) {
    throw TokenMismatchError{
        std::string{"expected a string-holding token type, found "} +
        tokenTypeToString(type_)};
  }

  return std::get<std::string>(*value_);
}
int Token::intValue() const {
  expectTokenType(TokenType::Num, type_);
  return std::get<int>(*value_);
}
RelOp Token::relOpValue() const {
  expectTokenType(TokenType::RelOp, type_);
  return std::get<RelOp>(*value_);
}
ArithOp Token::arithOpValue() const {
  expectTokenType(TokenType::ArithOp, type_);
  return std::get<ArithOp>(*value_);
}
LBinOp Token::logicBinOpValue() const {
  expectTokenType(TokenType::LBinOp, type_);
  return std::get<LBinOp>(*value_);
}

// Helper functions for converting operators to string
const char* opToString(ArithOp op) {
  switch (op) {
    case ArithOp::Plus:
      return "+";
    case ArithOp::Minus:
      return "-";
    case ArithOp::Times:
      return "*";
    default:
      throw std::logic_error{
          "Exhausted all possible operators. This case should be unreachable."};
  }
}

const char* opToString(RelOp op) {
  switch (op) {
    case RelOp::LessThan:
      return "<";
    case RelOp::LessEq:
      return "<=";
    case RelOp::Equal:
      return "=";
    default:
      throw std::logic_error{
          "Exhausted all possible operators. This case should be unreachable."};
  }
}

const char* opToString(LBinOp op) {
  switch (op) {
    case LBinOp::And:
      return "&&";
    case LBinOp::Or:
      return "||";
    default:
      throw std::logic_error{
          "Exhausted all possible operators. This case should be unreachable."};
  }
}

std::string Token::toString() const {
  std::ostringstream s;
  s << '<' << tokenTypeToString(type_);
  if (value_) {
    s << ',';
    if (auto str = std::get_if<std::string>(&*value_)) {
      s << *str;
    } else if (auto n = std::get_if<int>(&*value_)) {
      s << *n;
    } else if (auto op = std::get_if<RelOp>(&*value_)) {
      s << opToString(*op);
    } else if (auto op = std::get_if<ArithOp>(&*value_)) {
      s << opToString(*op);
    } else if (auto op = std::get_if<LBinOp>(&*value_)) {
      s << opToString(*op);
    } else {
      throw std::logic_error{
          "Exhausted all values a token may hold. This case should be "
          "unreachable."};
    }
  }
  s << '>';

  return s.str();
}

Token Token::makeId(std::string name) { return Token(TokenType::Id, name); }
Token Token::makeNum(int value) { return Token(TokenType::Num, value); }
Token Token::makeType(std::string name) { return Token(TokenType::Type, name); }
Token Token::makeIf() { return Token(TokenType::If); }
Token Token::makeElse() { return Token(TokenType::Else); }
Token Token::makeWhile() { return Token(TokenType::While); }
Token Token::makeDef() { return Token(TokenType::Def); }
Token Token::makeReturn() { return Token(TokenType::Return); }
Token Token::makeOutput() { return Token(TokenType::Output); }
Token Token::makeArithOp(ArithOp op) { return Token(TokenType::ArithOp, op); }
Token Token::makeRelOp(RelOp op) { return Token(TokenType::RelOp, op); }
Token Token::makeLBinOp(LBinOp op) { return Token(TokenType::LBinOp, op); }
Token Token::makeLNeg() { return Token(TokenType::LNeg); }
Token Token::makeLParen() { return Token(TokenType::LParen); }
Token Token::makeRParen() { return Token(TokenType::RParen); }
Token Token::makeLBrace() { return Token(TokenType::LBrace); }
Token Token::makeRBrace() { return Token(TokenType::RBrace); }
Token Token::makeLBracket() { return Token(TokenType::LBracket); }
Token Token::makeRBracket() { return Token(TokenType::RBracket); }
Token Token::makeSemicolon() { return Token(TokenType::Semicolon); }
Token Token::makeAssign() { return Token(TokenType::Assign); }
Token Token::makeHasType() { return Token(TokenType::HasType); }
Token Token::makeComma() { return Token(TokenType::Comma); }
Token Token::makeDot() { return Token(TokenType::Dot); }
Token Token::makeNil() { return Token(TokenType::Nil); }
Token Token::makeStruct() { return Token(TokenType::Struct); }
Token Token::makeNew() { return Token(TokenType::New); }

Token::Token(TokenType type) : type_(type) {}

Token::Token(TokenType type, std::string value) : type_(type), value_(value) {}

Token::Token(TokenType type, int value) : type_(type), value_(value) {}

Token::Token(TokenType type, RelOp value) : type_(type), value_(value) {}

Token::Token(TokenType type, ArithOp value) : type_(type), value_(value) {}

Token::Token(TokenType type, LBinOp value) : type_(type), value_(value) {}

std::ostream& operator<<(std::ostream& out, const Token& tok) {
  return out << tok.toString();
}

}  // namespace cs160::frontend
