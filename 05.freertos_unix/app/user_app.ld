/* user_app.ld */
ENTRY(main)

MEMORY
{
  APP_RAM(rwx) : ORIGIN = 0x20020000, LENGTH = 56K
}

SECTIONS
{
  .text : {
    KEEP(*(.text.main))   /* Entry point */
    *(.text*)
    *(.rodata*)
  } > APP_RAM

  .data : {
    *(.data*)
  } > APP_RAM

  .bss : {
    *(.bss*)
    *(COMMON)
  } > APP_RAM

  /* Place the user app stack at the end (if needed) */
  __app_stack_top__ = ORIGIN(APP_RAM) + LENGTH(APP_RAM);
}
