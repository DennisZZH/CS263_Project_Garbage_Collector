#include "frontend/lexer.h"
#include <algorithm>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <string_view>
#include <vector>

namespace {

using namespace cs160::frontend;

using State = int;

// Generate a fresh state
State genState() {
  static State nextState = 0;
  return nextState += 1;
}

// A set of ASCII chars with an internal bitvector representation. A bitvector
// of size 256 is only 32 bytes large and allows fast implementation of most set
// operations. We have some space overhead because we use a vector-based
// implementation.
struct CharSet {
 private:
  static const size_t BitvectorSize =
      size_t(std::numeric_limits<unsigned char>::max()) + 1;

 public:
  CharSet() : bits(BitvectorSize, false) {}

  explicit CharSet(char c) : bits(BitvectorSize, false) { add(c); }

  CharSet(char begin, char end) : bits(BitvectorSize, false) {
    addRange(begin, end);
  }

  bool empty() const {
    return std::find(bits.begin(), bits.end(), true) == bits.end();
  }

  bool operator[](char c) const { return bits[(unsigned char)c]; }

  void add(char c) { bits[(unsigned char)c] = true; }

  void remove(char c) { bits[(unsigned char)c] = true; }

  void addRange(char begin, char end) {
    for (auto i = (unsigned char)begin, end_ = (unsigned char)end; i <= end_;
         ++i) {
      bits[i] = true;
    }
  }

  void removeRange(char begin, char end) {
    for (auto i = (unsigned char)begin, end_ = (unsigned char)end; i <= end_;
         ++i) {
      bits[i] = false;
    }
  }

  void flip() {
    for (auto i = bits.begin(), end = bits.end(); i != end; ++i) {
      (*i).flip();
    }
  }

  void operator&=(CharSet const &that) {
    for (size_t i = 0; i < BitvectorSize; ++i) {
      bits[i] = bits[i] && that.bits[i];
    }
  }

  void operator|=(CharSet const &that) {
    for (size_t i = 0; i < BitvectorSize; ++i) {
      bits[i] = bits[i] || that.bits[i];
    }
  }

  void operator-=(CharSet const &that) {
    for (size_t i = 0; i < BitvectorSize; ++i) {
      bits[i] = bits[i] && !that.bits[i];
    }
  }

  CharSet operator~() const {
    CharSet result = *this;
    result.flip();
    return result;
  }

  CharSet operator&(CharSet const &that) const {
    CharSet result = *this;
    result &= that;
    return result;
  }

  CharSet operator|(CharSet const &that) const {
    CharSet result = *this;
    result |= that;
    return result;
  }

  CharSet operator-(CharSet const &that) const {
    CharSet result = *this;
    result -= that;
    return result;
  }

  static const CharSet alpha;
  static const CharSet digits;
  static const CharSet alnum;

 private:
  std::vector<bool> bits;
};

const CharSet CharSet::alpha = CharSet('a', 'z') | CharSet('A', 'Z');
const CharSet CharSet::digits = CharSet('0', '9');
const CharSet CharSet::alnum =
    CharSet('a', 'z') | CharSet('A', 'Z') | CharSet('0', '9');

// A struct that contains information on when an NFA reached an accept state
struct AcceptInfo {
  // The last index the NFA reached
  size_t index;
  // Accept states the NFA reached
  std::set<State> states;
};

struct NFA {
  // Some type aliases to reduce typing
  using Transition = std::pair<CharSet, State>;
  using Transitions = std::map<State, std::vector<Transition>>;

  // delete the empty constructor
  NFA() = delete;

  // NFA for empty language
  static NFA emptyLang() { return NFA{genState(), {}, {}, {}}; }

  // NFA for empty string
  static NFA epsilon() {
    auto q0 = genState();
    return NFA{q0, {q0}, {}, {}};
  }

  // NFA accepting only given string
  static NFA acceptOnly(std::string s) {
    auto q0 = genState();
    auto current = q0;
    Transitions transitions;

    // build a chain of states that let only s go through.
    for (char c : s) {
      auto next = genState();
      transitions.emplace(current, std::vector<Transition>{{CharSet{c}, next}});
      current = next;
    }

    return NFA{q0, {current}, transitions, {}};
  }

  static NFA acceptRange(const CharSet &c) {
    auto q0 = genState();
    auto q1 = genState();

    return NFA{
        q0, {q1}, Transitions{{q0, std::vector<Transition>{{c, q1}}}, {}}};
  }

  // BEGIN NFA operations. All of these operations keep the state labels from
  // the original NFA.

  // union of two NFAs' languages
  NFA operator|(const NFA &that) const {
    NFA result = *this;
    result |= that;
    return result;
  }
  // concatenate two NFAs' languages
  NFA operator*(const NFA &that) const {
    NFA result = *this;
    result *= that;
    return result;
  }

  void operator|=(const NFA &that) {
    auto oldStart = start;
    start = genState();
    // merge transition relations
    transitions.insert(that.transitions.begin(), that.transitions.end());
    epsilonTransitions.insert(that.epsilonTransitions.begin(),
                              that.epsilonTransitions.end());
    // merge start states
    epsilonTransitions.emplace(start, std::set<State>{oldStart, that.start});
    // merge accept states
    accept.insert(that.accept.begin(), that.accept.end());
  }

  void operator*=(const NFA &that) {
    // merge transition relations
    transitions.insert(that.transitions.begin(), that.transitions.end());
    epsilonTransitions.insert(that.epsilonTransitions.begin(),
                              that.epsilonTransitions.end());
    // add epsilon edges from old accept states to that.start
    for (auto q : accept) {
      if (epsilonTransitions.count(q) != 0) {
        epsilonTransitions.emplace(q, std::set<State>{});
      }
      epsilonTransitions[q].insert(that.start);
    }
    // set the new accept states
    accept = that.accept;
  }

  // Kleene star
  NFA operator*() const {
    auto newStart = genState();
    auto newEpsilonTransitions = epsilonTransitions;
    auto newTransitions = transitions;

    // create epsilon-edges from old accept states to new start state
    for (auto q : accept) {
      if (newEpsilonTransitions.count(q) == 0) {
        newEpsilonTransitions.emplace(q, std::set<State>{});
      }

      newEpsilonTransitions[q].insert(newStart);
    }

    newEpsilonTransitions.emplace(newStart, std::set<State>{start});

    return NFA{newStart,
               {newStart},
               std::move(newTransitions),
               std::move(newEpsilonTransitions)};
  }

  // Repeat 1 or more times.
  NFA operator+() const { return (*this) * (**this); }

  // Repeat 0 or 1 times. Corresponds to ? operation in extended regular
  // expressions
  NFA optional() const {
    auto newStart = genState();
    auto newEpsilonTransitions = epsilonTransitions;
    auto newTransitions = transitions;
    auto newAccept = accept;
    newAccept.emplace(newStart);

    newEpsilonTransitions.emplace(newStart, std::set<State>{start});

    return NFA{newStart, newAccept, std::move(newTransitions),
               std::move(newEpsilonTransitions)};
  }

  // END NFA operations.

  State start;
  std::set<State> accept;
  // non-epsilon transitions
  Transitions transitions;
  // epsilon transitions
  std::map<State, std::set<State>> epsilonTransitions;

  // epsilon closure of a given set of states. all states reachable from qs with
  // only epsilon edges
  std::set<State> epsClosure(const std::set<State> &qs) const {
    std::set<State> result;
    std::set<State> worklist;
    for (auto q : qs) {
      worklist.insert(q);
    }

    while (!worklist.empty()) {
      auto q = *worklist.begin();
      worklist.erase(worklist.begin());
      if (result.count(q) != 0) {
        // this node is already processed
        continue;
      }
      result.insert(q);
      auto it = epsilonTransitions.find(q);
      if (it != epsilonTransitions.end()) {
        for (auto q_ : it->second) {
          worklist.insert(q_);
        }
      }
    }

    return result;
  }

  // transitions with epsilon closure afterward
  std::set<State> delta(const std::set<State> &qs, char c) const {
    std::set<State> result;

    for (auto q : qs) {
      auto it = transitions.find(q);
      if (it != transitions.end()) {
        for (auto &[cs, q_] : it->second) {
          if (cs[c]) {
            result.insert(q_);
          }
        }
      }
    }

    return epsClosure(result);
  }

  // Longest munch to consume as many characters as possible
  std::optional<AcceptInfo> munch(size_t i,
                                  const std::string &programText) const {
    if (i > programText.size()) {
      return std::nullopt;
    }

    auto anyAccepting = [&](const std::set<State> &qs) -> bool {
      return std::any_of(qs.begin(), qs.end(), [&](const State &q) -> bool {
        return this->accept.count(q) != 0;
      });
    };

    auto states = epsClosure(std::set<State>{start});
    std::optional<AcceptInfo> lastAccept;

    if (anyAccepting(states)) {
      lastAccept = AcceptInfo{i, states};
    }

    for (; !states.empty() && i < programText.size(); ++i) {
      // Advance the NFA
      states = delta(states, programText[i]);
      // Update last accepted states
      if (anyAccepting(states)) {
        lastAccept = AcceptInfo{i + 1, states};
      }
    }

    // prune LastAccept to contain only accepting states
    if (lastAccept) {
      for (auto i = lastAccept->states.begin(), end = lastAccept->states.end();
           i != end;) {
        if (!accept.count(*i)) {
          i = lastAccept->states.erase(i);
        } else {
          ++i;
        }
      }
    }

    return lastAccept;
  }
};

// An additional struct to contain the NFA for recognizing lexemes and token
// types
struct NFAInfo {
  std::map<State, TokenType> q2tokType;
  std::optional<NFA> lexemeNFA;
  std::optional<NFA> whitespaceNFA;

  NFAInfo() {
    // create the NFA for identifiers
    NFA id =
        NFA::acceptRange(CharSet::alpha) * (*NFA::acceptRange(CharSet::alnum));

    // start by adding ID NFA to our lexeme NFA
    lexemeNFA.emplace(id);

    for (auto q : id.accept) {
      q2tokType.emplace(q, TokenType::Id);
    }

    // create the NFA for numbers
    NFA num =
        NFA::acceptOnly("-").optional() * (+NFA::acceptRange(CharSet::digits));

    *lexemeNFA |= num;

    for (auto q : num.accept) {
      q2tokType.emplace(q, TokenType::Num);
    }

    // A mapping from keywords, type int, and punctuation to their respective
    // token types
    std::vector<std::pair<std::string, TokenType>> keywordAndPunct{
        {"int", TokenType::Type},      {"while", TokenType::While},
        {"if", TokenType::If},         {"else", TokenType::Else},
        {"def", TokenType::Def},       {"return", TokenType::Return},
        {"output", TokenType::Output}, {"+", TokenType::ArithOp},
        {"-", TokenType::ArithOp},     {"*", TokenType::ArithOp},
        {"&&", TokenType::LBinOp},     {"||", TokenType::LBinOp},
        {"!", TokenType::LNeg},        {"<=", TokenType::RelOp},
        {"<", TokenType::RelOp},       {"=", TokenType::RelOp},
        {"(", TokenType::LParen},      {")", TokenType::RParen},
        {"{", TokenType::LBrace},      {"}", TokenType::RBrace},
        {"[", TokenType::LBracket},    {"]", TokenType::RBracket},
        {";", TokenType::Semicolon},   {":=", TokenType::Assign},
        {".", TokenType::Dot},         {"new", TokenType::New},
        {"nil", TokenType::Nil},       {"struct", TokenType::Struct},
        {":", TokenType::HasType},     {",", TokenType::Comma}};

    for (auto &[s, tokType] : keywordAndPunct) {
      // create the NFA
      auto nfa = NFA::acceptOnly(s);
      // add the NFA to our lexeme NFA
      *lexemeNFA |= nfa;
      // map the accepting state to corresponding token type
      q2tokType.emplace(*nfa.accept.begin(), tokType);
    }

    // Add the NFA for non-built-in types
    auto typeNFA = NFA::acceptOnly("%") * (+ NFA::acceptRange(CharSet::alnum));
    *lexemeNFA |= typeNFA;

    // Mark the accept states coming from typeNFA as Token::Type
    for (auto qAccept : typeNFA.accept) {
      q2tokType.emplace(qAccept, TokenType::Type);
    }

    // Build the whitespace NFA
    CharSet whitespace(' ');
    whitespace.add('\t');
    whitespace.add('\n');
    whitespace.add('\r');
    whitespaceNFA = NFA::acceptRange(whitespace);

    // a set containing all characters but null or newline
    CharSet nonLine{' ', std::numeric_limits<char>::max()};
    whitespace.add('\t');

    // make whitespace NFA skip comments as well
    *whitespaceNFA |= NFA::acceptOnly("//") * (*NFA::acceptRange(nonLine)) *
                      NFA::acceptRange(CharSet{'\n'});
    *whitespaceNFA = **whitespaceNFA;
  }

  // Get the most prioritized token accepted by any of the given states. The
  // behavior is undefined if the token types have no comparable priority.
  TokenType getMostPrioritizedTokenType(const std::set<State> &states) const {
    auto tokType = q2tokType.at(*states.begin());

    for (auto q : states) {
      // everything else is prioritized over identifiers
      if (tokType == TokenType::Id) {
        tokType = q2tokType.at(q);
      }
    }

    return tokType;
  }

  Token getToken(const AcceptInfo &acceptInfo, const std::string &programText,
                 size_t currentIndex) const {
    static auto getArithOp = [](std::string_view s) -> ArithOp {
      if (s == "+") {
        return ArithOp::Plus;
      }
      if (s == "-") {
        return ArithOp::Minus;
      }
      if (s == "*") {
        return ArithOp::Times;
      }
      throw std::logic_error{"Unexpected arith op"};
    };

    static auto getRelOp = [](std::string_view s) -> RelOp {
      if (s == "<") {
        return RelOp::LessThan;
      }
      if (s == "<=") {
        return RelOp::LessEq;
      }
      if (s == "=") {
        return RelOp::Equal;
      }
      throw std::logic_error{"Unexpected rel op"};
    };

    static auto getLBinOp = [](std::string_view s) -> LBinOp {
      if (s == "&&") {
        return LBinOp::And;
      }
      if (s == "||") {
        return LBinOp::Or;
      }
      throw std::logic_error{"Unexpected logical bin op"};
    };

    auto accepted = std::string_view(programText)
                        .substr(currentIndex, acceptInfo.index - currentIndex);
    switch (getMostPrioritizedTokenType(acceptInfo.states)) {
      case TokenType::Id:
        return Token::makeId(std::string{accepted.begin(), accepted.end()});
      case TokenType::Num:
        return Token::makeNum(
            std::stoi(std::string{accepted.begin(), accepted.end()}));
      case TokenType::Type:
        return Token::makeType(std::string{accepted.begin(), accepted.end()});
      case TokenType::If:
        return Token::makeIf();
      case TokenType::Else:
        return Token::makeElse();
      case TokenType::While:
        return Token::makeWhile();
      case TokenType::Def:
        return Token::makeDef();
      case TokenType::Return:
        return Token::makeReturn();
      case TokenType::Output:
        return Token::makeOutput();
      case TokenType::ArithOp:
        return Token::makeArithOp(getArithOp(accepted));
      case TokenType::RelOp:
        return Token::makeRelOp(getRelOp(accepted));
      case TokenType::LBinOp:
        return Token::makeLBinOp(getLBinOp(accepted));
      case TokenType::LNeg:
        return Token::makeLNeg();
      case TokenType::LParen:
        return Token::makeLParen();
      case TokenType::RParen:
        return Token::makeRParen();
      case TokenType::LBrace:
        return Token::makeLBrace();
      case TokenType::RBrace:
        return Token::makeRBrace();
      case TokenType::LBracket:
        return Token::makeLBracket();
      case TokenType::RBracket:
        return Token::makeRBracket();
      case TokenType::Semicolon:
        return Token::makeSemicolon();
      case TokenType::Assign:
        return Token::makeAssign();
      case TokenType::HasType:
        return Token::makeHasType();
      case TokenType::Comma:
        return Token::makeComma();
      case TokenType::Dot:
        return Token::makeDot();
      case TokenType::New:
        return Token::makeNew();
      case TokenType::Nil:
        return Token::makeNil();
      case TokenType::Struct:
        return Token::makeStruct();
      default:
        throw std::logic_error{
            "Unexpected token type. This should be unreachable."};
    }
  }
};

}  // anonymous namespace

namespace cs160::frontend {

std::vector<Token> Lexer::tokenize(const std::string &programText) {
  static NFAInfo nfaInfo;

  std::vector<Token> tokens;
  size_t currentIndex = 0;

  auto skipWhitespace = [&]() {
    if (auto lastAcceptInfo =
            nfaInfo.whitespaceNFA->munch(currentIndex, programText)) {
      currentIndex = lastAcceptInfo->index;
    }
  };

  skipWhitespace();

  while (currentIndex != programText.size()) {
    if (auto lastAcceptInfo =
            nfaInfo.lexemeNFA->munch(currentIndex, programText)) {
      tokens.push_back(
          nfaInfo.getToken(*lastAcceptInfo, programText, currentIndex));
      currentIndex = lastAcceptInfo->index;
    } else {
      // Unexpected character in program text
      throw InvalidLexemeError{programText[currentIndex], currentIndex};
    }
    skipWhitespace();
  }

  return tokens;
}

}  // namespace cs160::frontend
