//--------------------------------------------------------------------------------------------------
/**
 * @file main.c
 *
 * File for shared library generation.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */
//--------------------------------------------------------------------------------------------------

#include <stdio.h>
#include "foo.h"

int main(void)
{
    puts("Testing shared library ...");
    foo();
    return 0;
}
