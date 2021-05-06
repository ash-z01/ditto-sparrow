#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// #include "unicodeUtf8.h"
#include "lexer.h"
#include "../include/utils.h"

struct keywordToken {
    char* keyword;
    uint8_t length;
    TokenType token;
};  // 关键字结构体


struct keywordToken keywordTokens[] = {
    {"var", 3, TOKEN_VAR},
    {"fun", 3, TOKEN_FUN},
    {"if", 2, TOKEN_IF},
    {"else", 4, TOKEN_ELSE},
    {"true", 4, TOKEN_TURE},
    {"false", 5, TOKEN_FALSE},
    {"while", 5, TOKEN_FOR},
    {"for", 3, TOKEN_FOR},
    {"break", 5, TOKEN_BREAK},
    {"continue", 8, TOKEN_CONTINUE},
    {"return", 6, TOKEN_RETURN},
    {"null", 4, TOKEN_NULL},
    {"class", 5, TOEKN_CLASS},
    {"is", 2, TOKEN_IS},
    {"static", 6, TOKEN_STATIC},
    {"this", 4, TOKEN_THIS},
    {"super", 5, TOKEN_SUPER},
    {"import", 6, TOKEN_IMPORT},
    {NULL, 0, TOKEN_UNKNOWN}
};

// 判断start 是否为关键字，并返回token
static TokenType idOrKeyword(const char* start, uint32_t lenghth) {
    uint32_t idx = 0;
    while (keywordTokens[idx].keyword != NULL) {
        if (keywordTokens[idx].length == lenghth && \ 
            memcmp(keywordTokens[idx].keyword, start, lenghth) == 0) {
            return keywordTokens[idx].token;
        }
        idx++;
    }
    return TOKEN_ID;
}

// 向前查看一个字符
char lookAheadChar(Lexer* lexer) {
    return *lexer->nextCharPtr;
}


// 获取下一个字符
static void getNextChar(Lexer* lexer) {
    lexer->curChar = *lexer->nextCharPtr++;
}

// 查看下一个字符是否为预期字符，如果是则读取，并返回ture，否则返回false
static bool matchNextChar(Lexer* lexer, char expectedChar) {
    if (lookAheadChar(lexer) == expectedChar) {
        getNextChar(lexer);
        return true;
    }
    return false;
}

// 跳过连续的空白符
static void skipBlanks(Lexer* lexer) {
    while (isspace(lexer->curChar)) {
        if (lexer->curChar == '\n') {
            lexer->curToken.lineNo++;
        }
        getNextChar(lexer);
    }
}

// 解析标识符
static void lexID(Lexer* lexer, TokenType token_type) {
    while (isalnum(lexer->curChar) || lexer->curChar == '_') {
        getNextChar(lexer);
    }

    // nextChar 会指向第一个不合法字符的下一个字符，所以-1
    uint32_t length = (uint32_t)(lexer->nextCharPtr - lexer->curToken.start -1);
    if (token_type != TOKEN_UNKNOWN) {
        lexer->curToken.type = token_type;
    } else {
        lexer->curToken.type = idOrKeyword(lexer->curToken.start, length);
    }
    lexer->curToken.length = length;
}

// 解析unicode码点
static void lexUnicodeCodePoint(Lexer* lexer, ByteBuffer* buf) {
    uint32_t idx = 0;
    int value = 0;
    uint32_t digit = 0;

    // 获取数值 u后面跟着4位16进制数字
    while (idx++ < 4) {
        getNextChar(lexer);
        if (lexer->curChar == '\0') {
            LEX_ERROR(lexer, "unterminated unicode!");
        }
        if (lexer->curChar >= '0' && lexer->curChar <= '9') {
            digit = lexer->curChar - '0';
        } else if (lexer->curChar >= 'a' && lexer->curChar <= 'f') {
            digit = lexer->curChar - 'a' + 10;
        } else if (lexer->curChar >= 'A' && lexer->curChar <= 'F') {
            digit = lexer->curChar - 'A' + 10;
        } else {
            LEX_ERROR(lexer, "invalid unicode!");
        }
        value = value * 16 | digit;
    }

    uint32_t byteNum = getByteNumOfEncodeUtf8(value);
    ASSERT(byteNum != 0, "utf8 encode bytes should between 1 and 4!");

    // 为了代码通用 下面会直接写 buf->datas, 再次先写入 byteNum 个 0， 保证实现有空间
    ByteBufferFillWrite(lexer->vm, buf, 0, byteNum);

    // 把 value 编码为 uft8 后 写入缓冲区
    encodeUtf8(buf->datas + buf->count - byteNum, value);
}

// 解析字符串
static void lexString(Lexer* lexer) {
    ByteBuffer str;
    ByteBufferInit(&str);
    while (true) {
        getNextChar(lexer);
        if (lexer->curChar == '\0') {
            LEX_ERROR(lexer, "unterminated string!");
        }
        if (lexer->curChar == '"') {
            lexer->curToken.type = TOKEN_STRING;
            break;
        }
        if (lexer->curChar == '%') {
            if (!matchNextChar(lexer, '(')) {
                LEX_ERROR(lexer, "'%' should follow by '('!");
            }
            if (lexer->interpolationExceptRightParenNum > 0) {
                COMPILE_ERROR(lexer, "Do not supoort nest interpolation expression!");
            }
            lexer->interpolationExceptRightParenNum = 1;
            lexer->curToken.type = TOKEN_INTERPOLATION;
            break;
        }
        // 处理转义字符
        if (lexer->curChar == '\\') {
            getNextChar(lexer);
            switch (lexer->curChar)
            {
            case '0':
                ByteBufferAdd(lexer->vm, &str, '\0');
                break;
            case 'a':
                ByteBufferAdd(lexer->vm, &str, '\a');
                break;
            case 'b':
                ByteBufferAdd(lexer->vm, &str, '\b');
                break;
            case 'f':
                ByteBufferAdd(lexer->vm, &str, '\f');
                break;
            case 'n':
                ByteBufferAdd(lexer->vm, &str, '\n');
                break;
            case 'r':
                ByteBufferAdd(lexer->vm, &str, '\r');
                break;
            case 't':
                ByteBufferAdd(lexer->vm, &str, '\t');
                break;
            case 'u':
                // ByteBufferAdd(lexer->vm, &str, '\u');
                lexUnicodeCodePoint(lexer, &str);
                break;
            case '"':
                ByteBufferAdd(lexer->vm, &str, '\"');
                break;
            case '\\':
                ByteBufferAdd(lexer->vm, &str, '\\');
                break;
            default:
                LEX_ERROR(lexer, "unsupport escape \\%c", lexer->curChar);
                break;
            }
        } else {
            // 普通字符
            ByteBufferAdd(lexer, &str, lexer->curChar);
        }
    }
    ByteBufferClear(lexer->vm, &str);
}


// 跳过一行
static void skipALine(Lexer* lexer) {
    getNextChar(lexer);
    while (lexer->curChar != '\0') {
        if (lexer->curChar == '\n') {
            lexer->curToken.lineNo++;
            getNextChar(lexer);
            break;
        }
        getNextChar(lexer);
    }
}

// 跳过注释
static void skipComment(Lexer* lexer) {
    char nextChar = lookAheadChar(lexer);

    // 行注释
    if (lexer->curChar == '/') {
        skipALine(lexer);
    } else {
        // 区域注释
        // TODO
    }
    // 注释之后可能会有空白
    skipBlanks(lexer);
}

// 获取 Token
void getNextToken(Lexer* lexer) {
    lexer->preToken = lexer->curToken;
    skipBlanks(lexer); // 跳过待识别单词之前的空格
    lexer->curToken.type = TOKEN_EOF;
    lexer->curToken.length = 0;
    lexer->curToken.start = lexer->nextCharPtr - 1;

    while (lexer->curChar != '\0') {
        switch (lexer->curChar)
        {
        case ',':
            lexer->curToken.type = TOKEN_COMMA;
            break;
        case ':':
            lexer->curToken.type = TOKEN_COLON;
            break;
        case '(':
            if (lexer->interpolationExceptRightParenNum > 0) {
                lexer->interpolationExceptRightParenNum++;
            }
            lexer->curToken.type = TOKEN_LEFT_PAREN;
            break;
        case ')':
            if (lexer->interpolationExceptRightParenNum > 0) {
                lexer->interpolationExceptRightParenNum--;
                if (lexer->interpolationExceptRightParenNum == 0) {
                    lexString(lexer);
                    break;
                }
            }
            lexer->curToken.type = TOKEN_RIGHT_PAREN;
            break;
        case '[':
            lexer->curToken.type = TOKEN_LEFT_BRACKET;
            break;
        case ']':
            lexer->curToken.type = TOKEN_RIGHT_BRACKET;
            break;
        case '{':
            lexer->curToken.type = TOKEN_LEFT_BRACE;
            break;
        case '}':
            lexer->curToken.type = TOKEN_RIGHT_BRACE;
            break;
        case '.':
            if (matchNextChar(lexer, '.')) {
                lexer->curToken.type = TOKEN_DOT_DOT;
            } else {
                lexer->curToken.type = TOKEN_DOT;
            }
            break;
        case '=':
            if (matchNextChar(lexer, '=')) {
                lexer->curToken.type = TOKEN_EQUAL;
            } else {
                lexer->curToken.type = TOKEN_ASSIGN;
            }
            break;
        case '+':
            lexer->curToken.type = TOKEN_ADD;
            break;
        case '-':
            lexer->curToken.type = TOKEN_SUB;
            break;
        case '*':
            lexer->curToken.type = TOKEN_MUL;
            break;
        case '/':
            // 跳过 注释 //  和  /* 
            if (matchNextChar(lexer, '/') || matchNextChar(lexer, '*')) {
                skipComment(lexer);
                // 重置 下一个 Token的起始地址
                lexer->curToken.start = lexer->nextCharPtr - 1;
                continue;
            } else {
                lexer->curToken.type = TOKEN_DIV;
            }
            break;
        case '%':
            lexer->curToken.type = TOKEN_MOD;
            break;
        case '&':
            if (matchNextChar(lexer, '&')) {
                lexer->curToken.type = TOKEN_LOGIC_AND;
            } else {
                lexer->curToken.type = TOKEN_BIT_AND;
            }
            break;
        case '|':
            if (matchNextChar(lexer, '|')) {
                lexer->curToken.type = TOKEN_LOGIC_OR;
            } else {
                lexer->curToken.type = TOKEN_BIT_OR;
            }
            break;
        case '~':
            lexer->curToken.type = TOKEN_BIT_NOT;
            break;
        case '?':
            lexer->curToken.type = TOKEN_QUESTION;
            break;
        case '>':
            if (matchNextChar(lexer, '=')) {
                lexer->curToken.type = TOKEN_GREATER_EQUAL;
            } else if (matchNextChar(lexer, '>')) {
                lexer->curToken.type = TOKEN_BIT_SHIFT_RIGHT;
            } else {
                lexer->curToken.type = TOKEN_GREATER;
            }
            break;
        case '<':
            if (matchNextChar(lexer, '=')) {
                lexer->curToken.type = TOKEN_LESS_EQUAL;
            } else if (matchNextChar(lexer, '<')) {
                lexer->curToken.type = TOKEN_BIT_SHIFT_LEFT;
            } else {
                lexer->curToken.type = TOKEN_LESS;
            }
            break;
        case '!':
            if (matchNextChar(lexer, '=')) {
                lexer->curToken.type = TOKEN_NOT_EQUAL;
            } else {
                lexer->curToken.type = TOKEN_LOGIC_NOT;
            }
            break;
        case '"':
            lexString(lexer);
            break;
        default:
            // 处理变量名和数字
            // 进入default分支的字符必定是数字或者变量名的首字母
            // 后面会调用响应的函数，将剩余字符一并解析
            // TODO 识别数字

            // 若是 首字符是 字母 或者 _ ，则是变量名
            if (isalpha(lexer->curChar) || lexer->curChar == '_') {
                lexID(lexer, TOKEN_UNKNOWN); // 解析变量名的剩余部分
            } else {
                if (lexer->curChar == '#' && matchNextChar(lexer, '!')) {
                    skipALine(lexer);
                    // 重置下一个 Token的起始地址
                    lexer->curToken.start = lexer->nextCharPtr - 1;
                    continue;
                }
                LEX_ERROR(lexer, "unsupport char: \'%c\', quit.", lexer->curChar);
            }
            return;
        }

        lexer->curToken.length = (uint32_t)(lexer->nextCharPtr - lexer->curToken.start);
        getNextChar(lexer);
        return;
    }
}
