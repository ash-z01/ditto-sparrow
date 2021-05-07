#include <stdio.h>
#include <string.h>

#include "../lexer/lexer.h"
#include "../vm/vm.h"
#include "../vm/core.h"

static void runFile(const char* path) {
    const char * lastSlash = strrchr(path, '/');
    if (lastSlash != NULL) {
        char * root = (char *)malloc(lastSlash - path + 2);
        memcpy(root, path, lastSlash - path + 1);
        root[lastSlash - path + 1] = '\0';
        rootDir = root;
    }

    VM* vm = newVM();
    const char* sourceCode = readFile(path);

    struct lexer lexer;
    initLexer(vm, &lexer, path, sourceCode);

    #include "token.list"
    while (lexer.curToken.type != TOKEN_EOF) {
        getNextToken(&lexer);
        printf("%dL: %s [", lexer.curToken.lineNo, tokenArray[lexer.curToken.type]);
        uint32_t idx = 0;
        while (idx < lexer.curToken.length) {
            printf("%c", *(lexer.curToken.start+idx++));
        }
        printf("]\n");
    }
}

int main(int argc, const char* argv[]) {
    if (argc == 1) {
        ;
    } else {
        runFile(argv[1]);
    }
    return 0;
}