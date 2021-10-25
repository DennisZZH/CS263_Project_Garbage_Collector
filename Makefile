CXX=g++-8

# Flags for compiler
CXXFLAGS=-std=c++17 -Wall -I. -fPIC -g -O3
LDFLAGS=

# Flags for runtime components
RT_CXX=$(CXX)
RT_CXXFLAGS=-m32 -std=c++17 -Wall -I -fPIC -g
RT_LDFLAGS=-m32

# All headers needed for AST usage
AST_HEADERS=frontend/ast.h frontend/token.h frontend/ast_visitor.h frontend/print_visitor.h

.PHONY: test clean all

all: build/c1 build/lexer_test build/token_test build/parser_test build/gc.o build/bootstrap.o

build/bootstrap.o: bootstrap.cpp gc.h
	$(RT_CXX) $(RT_CXXFLAGS) -c bootstrap.cpp -o $@

build/gc.o: gc.h gc.cpp
	$(RT_CXX) $(RT_CXXFLAGS) -c gc.cpp -o $@

build/token.o: frontend/token.cpp frontend/token.h
	mkdir -p build
	$(CXX) $(CXXFLAGS) -c frontend/token.cpp -o $@

build/lexer.o: frontend/token.h frontend/lexer.h frontend/lexer.cpp
	mkdir -p build
	$(CXX) $(CXXFLAGS) -c frontend/lexer.cpp -o $@

build/ast.o: $(AST_HEADERS) frontend/ast.cpp
	mkdir -p build
	$(CXX) $(CXXFLAGS) -c frontend/ast.cpp -o $@

build/parser.o: frontend/parser.cpp frontend/parser.h $(AST_HEADERS)
	mkdir -p build
	$(CXX) $(CXXFLAGS) -c frontend/parser.cpp -o $@

build/lexer_test.o: frontend/token.h frontend/lexer.h frontend/lexer_test.cpp
	mkdir -p build
	$(CXX) $(CXXFLAGS) -c frontend/lexer_test.cpp -o $@

build/token_test.o: frontend/token.h frontend/token_test.cpp
	mkdir -p build
	$(CXX) $(CXXFLAGS) -c frontend/token_test.cpp -o $@

build/parser_test.o: frontend/parser_test.cpp frontend/parser.h $(AST_HEADERS)
	mkdir -p build
	$(CXX) $(CXXFLAGS) -c frontend/parser_test.cpp -o $@

build/codegen_test.o: backend/codegen.h backend/codegen_test.cpp $(AST_HEADERS)
	mkdir -p build
	$(CXX) $(CXXFLAGS) -c backend/codegen_test.cpp -o $@

build/codegen.o: backend/codegen.h backend/codegen.cpp $(AST_HEADERS)
	mkdir -p build
	$(CXX) $(CXXFLAGS) -c backend/codegen.cpp -o $@

build/main.o: frontend/token.h frontend/lexer.h $(AST_HEADERS) frontend/parser.h backend/codegen.h main.cpp
	mkdir -p build
	$(CXX) $(CXXFLAGS) -c main.cpp -o $@

build/c1: build/main.o build/lexer.o build/token.o build/parser.o build/ast.o build/codegen.o
	$(CXX) $(LDFLAGS) $^ -o $@

build/lexer_test: build/lexer.o build/token.o build/lexer_test.o
	$(CXX) $(LDFLAGS) $^ -o $@

build/token_test: build/token.o build/token_test.o
	$(CXX) $(LDFLAGS) $^ -o $@

build/parser_test: build/parser.o build/token.o build/lexer.o build/parser_test.o build/ast.o
	$(CXX) $(LDFLAGS) $^ -o $@

build/codegen_test: build/parser.o build/token.o build/lexer.o build/codegen_test.o build/ast.o build/codegen.o
	$(CXX) $(LDFLAGS) $^ -o $@

test: build/token_test build/lexer_test build/parser_test build/codegen_test
	-./build/token_test
	-./build/lexer_test
	-./build/parser_test
	-./build/codegen_test

clean:
	rm -f build/*
