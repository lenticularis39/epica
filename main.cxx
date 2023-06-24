#include <cstdlib>
#include <llvm/Support/raw_ostream.h>
#include "main.h"
#include "semantic_analyser.h"
#include "codegen_llvm.h"

Driver::Driver() : trace_parsing(false), trace_scanning(false) { }

int Driver::parse(const std::string &f) {
    file = f;
    location.initialize(&file);
    scan_begin();
    yy::parser parse(*this);
    parse.set_debug_level(std::getenv("EPICA_DEBUG") ? std::stoi(std::getenv("EPICA_DEBUG")) : 0);
    int result = parse();
    scan_end();
    return result;
}

int main(int argc, char **argv) {
    Driver driver;

    if (argc != 2) {
        std::cerr << "Usage: epica <source-file>" << std::endl;
        return 1;
    }

    if (driver.parse(argv[1]))
        return 1;

    SemanticAnalyser semantic_analyser(static_cast<Program *>(driver.root));
    if (!semantic_analyser.analyse())
        return 1;

    CodegenLLVM codegen(static_cast<Program *>(driver.root));
    llvm::Module *mod = codegen.compile();
    llvm::outs() << *mod << "\n";

    delete driver.root;
}
