#include <fstream>
#include <iostream>
#include <sstream>
#include "frontend/lexer.h"
#include "frontend/parser.h"
#include "backend/codegen.h"

using namespace cs160::frontend;
using namespace cs160::backend;

void usage(char const* programName) {
  std::cerr << "Usage: " << programName << " program.l2 [--gen-asm-only] output-file\n\n"
            << "This program compiles given L2 program. If the `--gen-asm-only` option is given, it will only generate the assembly code, otherwise it will also link the assembly code with the bootstrap code and GC code to produce an executable.";
}

// Option for generating assembly only
const std::string OnlyGenAsm{"--gen-asm-only"};

// C++ compiler. We use it as linker. GCC's C++ compier is usually named `g++` on most systems.
const std::string CPPCompiler{"g++"};

// Name of the assembler program. GNU assembler is usually named `as` on most systems.
const std::string Assembler{"as"};

int main(int argc, char* argv[]) {
  std::string outputFileName;
  bool link = true;

  if (argc == 3) {
    outputFileName = argv[2];
  } else if (argc == 4 && OnlyGenAsm == argv[2]) {
    outputFileName = argv[3];
    link = false;
  } else{
    usage(argv[0]);
    return 1;
  }

  std::ifstream programFile{argv[1]};
  if (!programFile.is_open()) {
    std::cerr << "'" << argv[1]
              << "' does not exist or is not a regular file.\n\n";
    usage(argv[0]);

  }  // Read the file
  std::string programText{std::istreambuf_iterator<char>(programFile),
                          std::istreambuf_iterator<char>()};

  // Run the lexer
  std::cout << "Lexing the input program '" << argv[1] << "'" << std::endl;
  Lexer lexer;
  auto tokens = lexer.tokenize(programText);

  // Run the parser
  std::cout << "Parsing the token stream" << std::endl;
  Parser parser(tokens);
  auto ast = parser.parse();

  if (! ast) {
    std::cerr << "Parse error: the parser produced an empty unique_ptr" << std::endl;
    return 1;
  }

  // Run the code generator
  std::cout << "Generating code" << std::endl;
  CodeGen codeGen;
  auto insns = codeGen.generateCode(*ast);

  // Write out the assembly file
  std::string asmFileName = link ? (outputFileName + ".asm") : outputFileName;
  std::ofstream asmFile{asmFileName};
  
  if (!asmFile.is_open()) {
    std::cerr << "Failed to open the assembly file '" << asmFileName << "'" << std::endl;
    return 1;
  }

  for (auto & line : insns) {
    asmFile << line << "\n";
  } 

  asmFile.close();

  // Assemble and link to build an executable
  if (link) {
    // Assemble the object file
    std::cout << "Calling the assembler on the assembly code to build\n";
    std::ostringstream cmdLine;
    cmdLine << Assembler << " --32 -g " << asmFileName << " -o " << outputFileName << ".o";
    auto cmd = cmdLine.str();
    std::cout << "Running assembler command: " << cmd << std::endl;
    if (auto err = std::system(cmd.c_str())) {
      std::cerr << "Assembler command exited with error code " << err << std::endl;
      return 1;
    }

    // Link the object files
    std::cout << "Linking the bootstrap code with L2 program object code\n";
    // reset the command line
    cmdLine = std::ostringstream{};
    cmdLine << CPPCompiler << " -m32 build/bootstrap.o build/gc.o " << outputFileName << ".o -o " << outputFileName;
    cmd = cmdLine.str();
    std::cout << "Running linker command: " << cmd << std::endl;
    // Run the linker
    if (auto err = std::system(cmd.c_str())) {
      std::cerr << "Linker exited with error code " << err << std::endl;
      return 1;
    }
  }

  return 0;
}
