ENTRY(app_entry)

MEMORY
{
  APP (rwx) : ORIGIN = 0x20020000, LENGTH = 64K
}

SECTIONS
{
  . = ORIGIN(APP);

  .text : {
    KEEP(*(.text.app_entry))   /* Put app_entry into its own section */
    *(.text*)
    *(.rodata*)
  } > APP

  .data : {
    *(.data*)
  } > APP

  .bss : {
    *(.bss*)
    *(COMMON)
  } > APP

  /* Entry symbol */
  PROVIDE(app_entry = app_entry);
}
