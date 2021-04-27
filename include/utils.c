#include "utils.h"
#include "vm.h"
#include "lexer.h"

#include <stdlib.h>
#include <stdarg.h>

// 内存管理
// * 1 申请内存
// * 2 修改内存大小
// * 3 释放内存
void* memManager(VM * vm, void* ptr, uint32_t oldSise, uint32_t newSize) {
    // 系统累计分配的总内存
    vm->allocatedBytes += newSize - oldSise;

    // 避免 realloc(NULL, 0)定义的新地址， 此地址无法释放
    if (newSize == 0) {
        free(ptr);
        return NULL;
    }
    return realloc(ptr, newSize);
}


// 找出 $ 2^x >= k $ 的 最小 2次幂数
//  1U << (log(k-1) + 1)
uint32_t ceilToPowerOf2(uint32_t k) {
    // k += (k == 0);
    // 修复 k 为 0 时的边界情况
    if (k == 0) {
        k++;
    }

    k--;
    k |= k >> 1;
    k |= k >> 2;
    k |= k >> 4;
    k |= k >> 8;
    k |= k >> 16;
    k++;
    return k;
}


DEFINE_BUFFER_METHOD(String)
DEFINE_BUFFER_METHOD(Int)
DEFINE_BUFFER_METHOD(Char)
DEFINE_BUFFER_METHOD(Byte)


// 清理符号表
void symbolTableClear(VM* vm, SymbolTable *buf) {
    uint32_t idx = 0;
    while (idx < buf->count) {
        memManager(vm, buf->datas[idx++].str, 0, 0);
    }
    StringBufferClear(vm, buf);
}


// 通用报错函数
void errorReport(void *lexer, ErrorType errorType, const char *fmt, ...) {
    char buffer[DEFAULT_BUFFER_SIZE] = {'\0'};
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buffer, DEFAULT_BUFFER_SIZE, fmt, ap);
    va_end(ap);

    switch (errorType) {
        case ERROR_IO:
        case ERROR_MEM:
            fprintf(stderr, "%s:%d In function %s():%s\n",
                    __FILE__, __LINE__, __func__, buffer);
            break;
        case ERROR_LEX:
        case ERROR_COMPILE:
            ASSERT(lexer != NULL, "lexer is null!");
            fprintf(stderr, "%s:%d \"%s\"\n", ((Lexer *)lexer)->file,
                    ((Lexer *)lexer)->preToken.lineNo, buffer);
            break;
        case ERROR_RUNTIME:
            fprintf(stderr, "%s\n", buffer);
            break;
        default:
            NOT_REACHED()
    }
    exit(1);
}