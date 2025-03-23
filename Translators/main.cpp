#include "DoWhileLoopLexer.h"
#include "DoWhileLoopParser.h"
#include "antlr4-runtime.h"
#include "common.h"
#include <iostream>

std::map<std::string, int> variables;

int main(int argc, const char* argv[]) 
{
    if (argc < 2) 
    {
        std::cerr << "Usage: " << argv[0] << " <input file>" << std::endl;
        return 1;
    }

    std::ifstream stream;
    stream.open(argv[1]);

    antlr4::ANTLRInputStream input(stream);

    DoWhileLoopLexer lexer(&input);

    antlr4::CommonTokenStream tokens(&lexer);

    DoWhileLoopParser parser(&tokens);

    antlr4::tree::ParseTree* tree = parser.program();

    return 0;
}
