#include "main.h"

Driver::Driver() : trace_parsing(false), trace_scanning(false) { }

int Driver::parse(const std::string &f) {
    file = f;
    location.initialize(&file);
    scan_begin();
    yy::parser parse(*this);
    parse.set_debug_level(1);
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

    driver.parse(argv[1]);
    std::cout << "Number of functions: " << driver.root->children.size() << std::endl;
    for (Node *child : driver.root->children) {
        auto fun = static_cast<Function *>(child);
        std::cout << "Function: " << fun->name << std::endl;
    }
    delete driver.root;
}