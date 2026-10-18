SECTIONS {
  . = 0x7C00;
  .mbr16 : { *(.mbr16) }
  . = 0x8000;
  .entry16 : { *(.entry16) }
  .entry32 : { *(.entry32) }
  .text : { *(.text .text.*) }
  .rodata : { *(.rodata*) }

  .init_array :
  {
    _init_array_start = .;
    KEEP (*(SORT_BY_INIT_PRIORITY(.init_array.*)))
    KEEP (*(.init_array))
    _init_array_end = .;
  }

  /* Bare-metal GCC emits legacy constructor lists rather than init_array. */
  .ctors :
  {
    _ctors_start = .;
    KEEP (*(.ctors))
    _ctors_end = .;
  }

  .fini_array :
  {
    _fini_array_start = .;
    KEEP (*(SORT_BY_INIT_PRIORITY(.fini_array.*)))
    KEEP (*(.fini_array))
    _fini_array_end = .;
  }

  .data : { *(.data .data.*) }
  ASSERT(. < 0x80000, "Boot image exceeds the low-memory BIOS loading area")

  /* Static buffers/stacks must not overlap VGA, the EBDA, or BIOS ROM. */
  . = 0x100000;
  .bss (NOLOAD) :
  {
    _bss_start = .;
    *(.bss .bss.* COMMON)
    _bss_end = .;
  }
  . = ALIGN(4096);
  _kernel_end = .;
}
