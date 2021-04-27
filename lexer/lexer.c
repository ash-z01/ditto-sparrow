#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
// #include <stdbool.h>

struct sample
{
   int length;
   char * str;
};

void test1(){
    struct sample * s = malloc(sizeof(struct sample));
    s->length = 10;
    s->str = malloc(s->length+1);
    strcpy(s->str, "abcdefg");
    s->str[s->length] = '\0';
    printf("xxx[%s]xxx\n",s->str);
}

struct sample_vla
{
    int length;
    char str[];
};


void test2(){
    // bool a = false;
    char *string = "abcdefg";
    int len = strlen(string);
    struct sample_vla * s = malloc(sizeof(struct sample_vla) + len + 1);
    s->length = len + 1;
    strcpy(s->str, string);
    s->str[s->length] = '\0';
    printf("xxx[%s]xxx\n",s->str);
}

int main(void) {
    test2();
    return 0;
}