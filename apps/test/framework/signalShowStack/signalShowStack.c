/**
 * This module is for unit testing the handle of hardware signals in the legato
 * runtime library (liblegato.so).
 *
 * The following is a list of the test cases:
 *
 * - SEGV : Invalid address access
 * - ILL  : Illegal instruction execution
 * - FPE  : Floating Point Exception -- Not able to raise it
 * - ABRT : Abort due to service misuage or corruption, like a double free()
 * - BUS  : External bus access but also alignment -- Not able to raise it
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "fenv.h"

double NaN = NAN;
double z = 0.0;

/*
 * SEGV case: access to an invalid address
 */
void segv
(
    void
)
{
    int* p = (int*)0xDEADBEEF;
    *p = 1234567890;
}

void run_segv
(
    void
)
{
    fprintf(stderr, "DO SEGV\n");
    segv();
}

/*
 * SEGV case 2: crush the stack frame
 */
void crush
(
    void
)
{
    int p = 1;
    int *pp = &p;
    memset(pp-16, 0xDE, 128);
}

void run_crush
(
    void
)
{
    fprintf(stderr, "DO CRUSH\n");
    crush();
}

/*
 * ILL case: try to execute an illegal instruction
 */
int ill
(
    void
)
{
    // Values are for stack investigation
    int x = 2;
    int i[] = {0xFFFFFFFF, 0xFFFFFFFE, 0xFFFFFFFD};
    asm( ".word %a0" : : "X" (-1) );
    return i[x];
}

void run_ill
(
    void
)
{
    int (*q)(void) = (int(*)(void))(ill + 0);
    fprintf(stderr, "DO ILL %p q %p\n", ill, q);
    int j=q(); //NOSONAR
    j++; //NOSONAR
}

/*
 * FPE case: floating point exception. Do not know how to raise it
 */
void fpe
(
    void
)
{
    double f = 123.4567890;
    feenableexcept(FE_ALL_EXCEPT);
    f = f / z; //NOSONAR
    f = z / z; //NOSONAR
}

void run_fpe
(
    void
)
{
    fprintf(stderr, "DO FPE\n");
    fpe();
}

/*
 * BUS case: bus error or alignment exception
 */
void bus
(
    void
)
{
    double f = 123.4567890;
    char *p = ((char*)&f)+19;
    f = *(double*)p / z; //NOSONAR
}

void run_bus
(
    void
)
{
    fprintf(stderr, "DO BUS\n");
    fprintf(stderr, "First: echo 4 >/proc/cpu/alignment\n");
    system("echo 4 >/proc/cpu/alignment");
    bus();
}

/*
 * ABRT case: service misusage or corruption. Like a double free
 */
char *abrt
(
    void
)
{
    char *ptr = malloc(1); //NOSONAR

    fprintf(stderr, "ptr allocated at %p", ptr);
    free(ptr); //NOSONAR
    return ptr;
}

void run_abrt
(
    void
)
{
    char *abrtPtr;
    fprintf(stderr, "DO ABRT\n");
    abrtPtr = abrt();
    free(abrtPtr); //NOSONAR
}

COMPONENT_INIT
{
    const char *argPtr;

    if (1 == le_arg_NumArgs())
    {
        argPtr = le_arg_GetArg(0);
        if (NULL == argPtr)
        {
            LE_ERROR("argPtr is NULL");
            exit(EXIT_FAILURE);
        }
        if (0 == strcmp("SEGV", argPtr))
        {
            run_segv();
        }
        else if (0 == strcmp("CRUSH", argPtr))
        {
            run_crush();
        }
        else if (0 == strcmp("ILL", argPtr))
        {
            run_ill();
        }
        else if (0 == strcmp("BUS", argPtr))
        {
            run_bus();
        }
        else if (0 == strcmp("FPE", argPtr))
        {
            run_fpe();
        }
        else if (0 == strcmp("ABRT", argPtr))
        {
            run_abrt();
        }
        else
        {
            // all other cases, return an error
            fprintf(stderr, "Unknown argument %s", argPtr);
            exit(EXIT_FAILURE);
        }
        LE_ASSERT(0);
    }
    fprintf(stderr, "Need argument to raise signal: SEGV CRUSH ILL FPE BUS ABRT\n");
    exit(EXIT_FAILURE);
}
