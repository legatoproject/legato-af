/*
 * A platform-independent linker script for supporting hibernation of memory pools
 *
 * Copyright (C) Sierra Wireless Inc.
 */

SECTIONS
{

  .bss._mem_Pools :
  {
    le_mem_StartPools = .;
    *(.bss._mem_*Data); /* Assume we're using -fdata-sections */
    le_mem_EndPools = .;
  }
}
