#ifndef _VM_VM_H
#define _VM_VM_H


#include "../include/common.h"


// 虚拟机结构
struct vm
{
    // 累计分配的内存大小
    uint32_t allocatedBytes;
    // 当前的词法分析器
    Lexer* curLexer;
};


// 初始化虚拟机
void initVM(VM* vm);

// 新建虚拟机
VM* newVM(void);


#endif