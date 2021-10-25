
#include "frontend/parser.h"
#include <iostream>

namespace cs160::frontend {

Token Parser::matchToken(const TokenType& tok) {
  // we're duplicating some work here but I don't understand quite what
  if (auto token = nextToken(); token && token->type() == tok) {
    head++;
    return *token;
  } else {
    std::string message = "Expected a " + std::string(tokenTypeToString(tok));
    if (nextToken()) {
      message += " but found " + nextToken()->toString();
    } else {
      message += " but reached the end of program";
    }
    throw InvalidASTError{message};
  }
}

// peek one token ahead but don't advance; should move from head as int to head
// as iterator with std::next
std::optional<cs160::frontend::Token> Parser::nextToken(int peek) {
  if (head == static_cast<int>(tokens.size()) - 1) {
    return std::nullopt;
  } else {
    return tokens[head + peek];
  }
}

// arithmetic expressions

IntegerExprP Parser::parseIntegerExpr() {
  // std::cout << "Parser::parseIntegerExpr" << std::endl;
  matchToken(TokenType::Num);
  return std::make_unique<const IntegerExpr>(tokens[head].intValue());
}

Variable Parser::parseVariable() {
  return Variable(matchToken(TokenType::Id).stringValue());
}

std::unique_ptr<const AccessPath> Parser::parseAccessPath() {
  // std::cout << "Parser::parseVariable" << std::endl;
  auto root = parseVariable();
  std::vector<std::string> fields;
  // Convert recursive descent to a while loop
  while (nextToken() && nextToken()->type() == TokenType::Dot) {
    matchToken(TokenType::Dot);
    fields.push_back(matchToken(TokenType::Id).stringValue());
  }
  return std::make_unique<const AccessPath>(root, fields);
}

ArithmeticExprP Parser::parseAFactor() {
  // std::cout << "Parser::parseAFactor " << nextToken().value() << std::endl;
  if (nextToken() && nextToken().value().type() == TokenType::LParen) {
    matchToken(TokenType::LParen);
    auto ae = parseArithmeticExpr();
    matchToken(TokenType::RParen);
    return ae;
  } else if (nextToken() && nextToken().value().type() == TokenType::Num) {
    return parseIntegerExpr();
  } else if (nextToken() && nextToken().value().type() == TokenType::Id) {
    return parseAccessPath();
  } else if (nextToken() && nextToken().value().type() == TokenType::Nil) {
    matchToken(TokenType::Nil);
    return std::make_unique<NilExpr>();
  } else if (nextToken() && nextToken().value().type() == TokenType::New) {
    matchToken(TokenType::New);
    auto type = matchToken(TokenType::Type);
    if (type.stringValue() == "int") {
      throw InvalidASTError(); // cannot create ints with new
    }
    return std::make_unique<NewExpr>(type.stringValue());
  }
  throw InvalidASTError();  // if we get here it should be a parse error
}

ArithmeticExprP Parser::parseATermPrime() {
  // std::cout << "Parser::parseATermPrime" << std::endl;

  if (nextToken() &&
      nextToken().value() == Token::makeArithOp(ArithOp::Times)) {
    matchToken(Token::makeArithOp(ArithOp::Times).type());
    auto l = parseAFactor();
    auto p = parseATermPrime();
    if (p) {
      return std::make_unique<const MultiplyExpr>(std::move(l), std::move(p));
    } else {
      return l;
    }
  } else {
    // empty unique_ptr
    return {};
  }
}

ArithmeticExprP Parser::parseATerm() {
  // std::cout << "Parser::parseATerm" << std::endl;
  auto l = parseAFactor();
  auto p = parseATermPrime();
  if (p) {
    return std::make_unique<const MultiplyExpr>(std::move(l), std::move(p));
  } else {
    return l;
  }
}

std::pair<ArithmeticExprP, std::optional<Token>> Parser::parseAExpPrime() {
  // std::cout << "Parser::parseAExpPrime " << nextToken().value() << std::endl;
  if (nextToken() && nextToken().value() == Token::makeArithOp(ArithOp::Plus)) {
    matchToken(Token::makeArithOp(ArithOp::Plus).type());
    auto l = parseATerm();
    auto p = parseAExpPrime();
    if (p.first && p.second.value() == Token::makeArithOp(ArithOp::Plus)) {
      return std::make_pair(
          std::make_unique<const AddExpr>(std::move(l), std::move(p.first)),
          Token::makeArithOp(ArithOp::Plus));
    } else if (p.first &&
               p.second.value() == Token::makeArithOp(ArithOp::Minus)) {
      return std::make_pair(std::make_unique<const SubtractExpr>(
                                std::move(l), std::move(p.first)),
                            Token::makeArithOp(ArithOp::Plus));

    } else {
      return std::make_pair(std::move(l), Token::makeArithOp(ArithOp::Plus));
    }
  }

  else if (nextToken() &&
           nextToken().value() == Token::makeArithOp(ArithOp::Minus)) {
    matchToken(Token::makeArithOp(ArithOp::Minus).type());
    auto l = parseATerm();
    auto p = parseAExpPrime();
    if (p.first && p.second.value() == Token::makeArithOp(ArithOp::Plus)) {
      return std::make_pair(
          std::make_unique<const AddExpr>(std::move(l), std::move(p.first)),
          Token::makeArithOp(ArithOp::Minus));
    } else if (p.first &&
               p.second.value() == Token::makeArithOp(ArithOp::Minus)) {
      return std::make_pair(std::make_unique<const SubtractExpr>(
                                std::move(l), std::move(p.first)),
                            Token::makeArithOp(ArithOp::Minus));
    } else {
      return std::make_pair(std::move(l), Token::makeArithOp(ArithOp::Minus));
    }

  } else {
    return std::make_pair(nullptr, std::nullopt);  // epsilon case
  }
}

ArithmeticExprP Parser::parseArithmeticExpr() {
  // std::cout << "Parser::parseArithmeticExpr" << std::endl;
  auto at = parseATerm();
  auto p = parseAExpPrime();
  if (p.second && p.second.value() == Token::makeArithOp(ArithOp::Plus)) {
    return std::make_unique<const AddExpr>(std::move(at), std::move(p.first));
  }

  else if (p.second && p.second.value() == Token::makeArithOp(ArithOp::Minus)) {
    return std::make_unique<const SubtractExpr>(std::move(at),
                                                std::move(p.first));
  }

  else {
    return at;
  }
}

// relational expressions

RelationalExprP Parser::parseRexpPrime1() {
  // std::cout << "Parser::parseRexpPrime1" << std::endl;
  if (nextToken() && nextToken().value().type() == TokenType::LNeg) {
    matchToken(TokenType::LNeg);
    auto re = parseRexp();
    return std::make_unique<const LogicalNotExpr>(std::move(re));

  } else if (nextToken() && nextToken().value().type() == TokenType::LBracket) {
    matchToken(TokenType::LBracket);
    auto re = parseRexp();
    matchToken(TokenType::RBracket);
    return re;

  } else {
    auto ae1 = parseArithmeticExpr();
    if (nextToken() &&
        nextToken().value() == Token::makeRelOp(RelOp::LessThan)) {
      matchToken(Token::makeRelOp(RelOp::LessThan).type());
      auto ae2 = parseArithmeticExpr();
      return std::make_unique<const LessThanExpr>(std::move(ae1),
                                                  std::move(ae2));

    } else if (nextToken() &&
               nextToken().value() == Token::makeRelOp(RelOp::LessEq)) {
      matchToken(Token::makeRelOp(RelOp::LessEq).type());
      auto ae2 = parseArithmeticExpr();
      return std::make_unique<const LessThanEqualToExpr>(std::move(ae1),
                                                         std::move(ae2));

    } else if (nextToken() &&
               nextToken().value() == Token::makeRelOp(RelOp::Equal)) {
      matchToken(Token::makeRelOp(RelOp::Equal).type());
      auto ae2 = parseArithmeticExpr();
      return std::make_unique<const EqualToExpr>(std::move(ae1),
                                                 std::move(ae2));
    }
  }
  throw InvalidASTError();
}

std::pair<RelationalExprP, std::optional<Token>> Parser::parseRexpPrime2() {
  // std::cout << "Parser::parseRexpPrime2" << std::endl;
  if (nextToken() && nextToken().value() == Token::makeLBinOp(LBinOp::And)) {
    matchToken(Token::makeLBinOp(LBinOp::And).type());

    auto l = parseRexpPrime1();
    auto r = parseRexpPrime2();

    if (r.first) {
      return std::make_pair(std::make_unique<const LogicalAndExpr>(
                                std::move(l), std::move(r.first)),
                            Token::makeLBinOp(LBinOp::And));
    } else {
      return std::make_pair(std::move(l), Token::makeLBinOp(LBinOp::And));
    }
  }

  else if (nextToken() &&
           nextToken().value() == Token::makeLBinOp(LBinOp::Or)) {
    matchToken(Token::makeLBinOp(LBinOp::Or).type());

    auto l = parseRexpPrime1();
    auto r = parseRexpPrime2();

    if (r.first) {
      return std::make_pair(std::make_unique<const LogicalOrExpr>(
                                std::move(l), std::move(r.first)),
                            Token::makeLBinOp(LBinOp::Or));
    } else {
      return std::make_pair(std::move(l), Token::makeLBinOp(LBinOp::Or));
    }
  } else {
    return std::make_pair(nullptr, std::nullopt);
  }
}

RelationalExprP Parser::parseRexp() {
  // std::cout << "Parser::parseRexp" << std::endl;
  auto l = parseRexpPrime1();
  auto r = parseRexpPrime2();
  if (r.second && r.second.value() == Token::makeLBinOp(LBinOp::And)) {
    return std::make_unique<const LogicalAndExpr>(std::move(l),
                                                  std::move(r.first));
  } else if (r.second && r.second.value() == Token::makeLBinOp(LBinOp::Or)) {
    return std::make_unique<const LogicalOrExpr>(std::move(l),
                                                 std::move(r.first));
  } else {
    return l;
  }
}

// statements and declarations

LoopExprP Parser::parseLoopExprP() {
  // std::cout << "Parser::parseLoopExprP" << std::endl;
  matchToken(TokenType::While);
  matchToken(TokenType::LParen);
  auto re = parseRexp();
  matchToken(TokenType::RParen);
  matchToken(TokenType::LBrace);
  auto blk = parseBlockStmt();
  matchToken(TokenType::RBrace);
  return std::make_unique<const Loop>(std::move(re), std::move(blk));
}

ConditionalExprP Parser::parseCondExprP() {
  // std::cout << "Parser::parseCondExprP" << std::endl;
  matchToken(TokenType::If);
  matchToken(TokenType::LParen);

  auto re = parseRexp();

  matchToken(TokenType::RParen);
  matchToken(TokenType::LBrace);

  auto blk = parseBlockStmt();

  matchToken(TokenType::RBrace);

  if (nextToken() && nextToken().value().type() == TokenType::Else) {
    matchToken(TokenType::Else);
    matchToken(TokenType::LBrace);

    auto blk2 = parseBlockStmt();
    matchToken(TokenType::RBrace);

    return std::make_unique<const Conditional>(std::move(re), std::move(blk),
                                               std::move(blk2));

  } else {
    std::vector<DeclarationExprP> d;
    std::vector<StatementP> s;
    return std::make_unique<const Conditional>(
        std::move(re), std::move(blk),
        std::make_unique<const BlockStmt>(Declaration::Block{}, std::move(s)));
  }
}

// break up into second call, e.g. parseRHS?
AssignmentExprP Parser::parseAssignmentExprP() {
  // std::cout << "Parser::parseAssignmentExprP" << std::endl;
  auto lhs = parseAccessPath();
  matchToken(TokenType::Assign);

  // the one place we do LL(2)
  if (nextToken(1) && nextToken(2) &&
      nextToken(1).value().type() == TokenType::Id &&
      nextToken(2).value().type() == TokenType::LParen) {
    auto c = parseFunCall();
    matchToken(TokenType::Semicolon);
    return std::make_unique<const Assignment>(std::move(lhs), std::move(c));

  } else {
    auto ae = parseArithmeticExpr();
    matchToken(TokenType::Semicolon);
    return std::make_unique<const Assignment>(std::move(lhs), std::move(ae));
  }
}

Declaration Parser::parseDeclaration() {
  auto t = TypeExpr{matchToken(TokenType::Type).stringValue()};
  auto id = parseVariable();
  matchToken(TokenType::Semicolon);
  return Declaration(std::move(t), std::move(id));
}

Declaration::Block Parser::parseDecls() {
  Declaration::Block ret;
  while (nextToken() && (nextToken().value().type() == TokenType::Type)) {
    ret.push_back(parseDeclaration());
  }
  return ret;
}

StatementP Parser::parseStatementP() {
  // std::cout << "Parser::parseStatementP" << std::endl;
  if (nextToken() && nextToken().value().type() == TokenType::While) {
    return parseLoopExprP();
  } else if (nextToken() && nextToken().value().type() == TokenType::If) {
    return parseCondExprP();
    // playing a little fast/loose maybe todo
  } else if (nextToken() && nextToken().value().type() == TokenType::Id) {
    return parseAssignmentExprP();
  }
  throw InvalidASTError();
}

// not sure I"m doing the assign case correctly here
Statement::Block Parser::parseStmts() {
  // std::cout << "Parser::parseStmts" << std::endl;
  Statement::Block ret;
  while (nextToken() && (nextToken().value().type() == TokenType::Id ||
                         nextToken().value().type() == TokenType::While ||
                         nextToken().value().type() == TokenType::If)) {
    auto s = parseStatementP();
    ret.push_back(std::move(s));
  }
  return ret;
}

BlockStmtP Parser::parseBlockStmt() {
  // std::cout << "Parser::parseBlockStmt" << std::endl;
  Declaration::Block d = parseDecls();
  Statement::Block s = parseStmts();
  return std::make_unique<const BlockStmt>(std::move(d), std::move(s));
}

// function defs, calls, args, and params
std::vector<ArithmeticExprP> Parser::parseFunArgs() {
  // std::cout << "Parser::parseFunArgs" << std::endl;
  std::vector<ArithmeticExprP> ret;

  while (nextToken() && (nextToken().value().type() == TokenType::Num ||
                         nextToken().value().type() == TokenType::Id ||
                         nextToken().value().type() == TokenType::ArithOp)) {
    auto ae = parseArithmeticExpr();
    ret.push_back(std::move(ae));

    // todo, another fast/loose possible break of LL(1)
    if (nextToken() && nextToken().value().type() == TokenType::Comma) {
      matchToken(TokenType::Comma);
    }
  }
  return ret;
}

FunctionCallP Parser::parseFunCall() {
  // std::cout << "Parser::parseFunCall" << std::endl;
  auto id = matchToken(TokenType::Id).stringValue();
  matchToken(TokenType::LParen);
  auto args = parseFunArgs();
  matchToken(TokenType::RParen);
  return std::make_unique<const FunctionCall>(id, std::move(args));
}

std::vector<std::pair<std::unique_ptr<const TypeExpr>,
                      Variable>>
Parser::parseParams() {
  // std::cout << "Parser::parseParams" << std::endl;
  std::vector<std::pair<std::unique_ptr<const TypeExpr>,
                        Variable>>
      ret;
  while (nextToken() && nextToken().value().type() == TokenType::Type) {
    auto t = std::make_unique<TypeExpr>(matchToken(TokenType::Type).stringValue());
    auto id = parseVariable();
    auto pair =
        std::make_pair(std::move(t), std::move(id));
    ret.push_back(std::move(pair));

    // todo, another fast/loose possible break of LL(1)
    if (nextToken() && nextToken().value().type() == TokenType::Comma) {
      matchToken(TokenType::Comma);
    }
  }
  return ret;
}

std::vector<std::pair<std::unique_ptr<const TypeExpr>,
                      Variable>>
Parser::parseOptParams() {
  // std::cout << "Parser::parseOptParams" << std::endl;
  if (nextToken() && nextToken().value().type() == TokenType::Type) {
    return parseParams();
  } else {
    std::vector<std::pair<std::unique_ptr<const TypeExpr>,
                          std::unique_ptr<const Variable>>>
        ret;
    // return empty vector
    return {};
  }
}

FunctionDefP Parser::parseFunDef() {
  // std::cout << "Parser::parseFunDef" << std::endl;
  matchToken(TokenType::Def);
  auto id = matchToken(TokenType::Id).stringValue();

  matchToken(TokenType::LParen);
  auto optparams = parseOptParams();
  matchToken(TokenType::RParen);
  matchToken(TokenType::HasType);

  auto retType = std::make_unique<const TypeExpr>(matchToken(TokenType::Type).stringValue());
  matchToken(TokenType::LBrace);

  auto b = parseBlockStmt();
  matchToken(TokenType::Return);

  auto ae = parseArithmeticExpr();
  matchToken(TokenType::Semicolon);
  matchToken(TokenType::RBrace);
  return std::make_unique<const FunctionDef>(
      id, std::move(retType), std::move(optparams),
      std::move(b), std::move(ae));
}

FunctionDef::Block Parser::parseFunDefs() {
  // std::cout << "Parser::parseFunDefs" << std::endl;
  FunctionDef::Block ret;
  while (nextToken() && (nextToken().value().type() == TokenType::Def)) {
    auto f = parseFunDef();
    ret.push_back(std::move(f));
  }
  return ret;
}

TypeDef Parser::parseTypeDef() {
  matchToken(TokenType::Struct);
  auto type = matchToken(TokenType::Type);
  if (type.stringValue() == "int") {
    // can't define int as a struct
    throw InvalidASTError();
  }
  matchToken(TokenType::LBrace);
  auto decls = parseDecls();
  matchToken(TokenType::RBrace);
  matchToken(TokenType::Semicolon);

  return {type.stringValue(), std::move(decls)};
}

TypeDef::Block Parser::parseTypeDefs() {
  TypeDef::Block ret;
  while (nextToken() && (nextToken().value().type() == TokenType::Struct)) {
    ret.push_back(parseTypeDef());
  }
  return ret;
}

// toplevel
ProgramExprP Parser::parse() {
  // std::cout << "Parser::parseProgram" << std::endl;

  TypeDef::Block typeDefs = parseTypeDefs();
  FunctionDef::Block f = parseFunDefs();
  BlockStmtP s = parseBlockStmt();

  matchToken(TokenType::Output);
  auto ae = parseArithmeticExpr();
  matchToken(TokenType::Semicolon);

  return std::make_unique<const Program>(std::move(typeDefs),
                                         std::move(f),
                                         std::move(s),
                                         std::move(ae));
}
}  // namespace cs160::frontend
