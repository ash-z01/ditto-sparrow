#include <string.h>
#include <sys/stat.h>

#include "core.h"
#include "utils.h"
#include "vm.h"

char* rootDir = NULL;


// 读取源文件代码
char* readFile(const char* path) {
    FILE* file = foepn(path, "r"); 
    if (file == NULL) {
        IO_ERROR("open file [%s] failed\n", path);
    }
    struct stat fileStat;
    stat(path, &fileStat);
    size_t fileSize = fileStat.st_size;
    char* fileContent = (char*)malloc(fileSize+1);
    if (fileContent == NULL) {
        MEM_ERROR("allocate memory for reading file %s failed\n", path);
    }
    size_t numRead = fread(fileContent, sizeof(char), fileSize, file);
    if (numRead < fileSize) {
        IO_ERROR("read file %s err\n", path);
    }
    fileContent[fileSize] = '\0';
    fclose(file);
    return fileContent;
}