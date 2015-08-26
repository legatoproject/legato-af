#include <stdio.h>

__attribute__((constructor)) void Init(void)
{
    printf("External library is initialized.\n");
}

void DoSomething(void)
{
    printf("External library is doing something.\n");
}
