ENTRY(app_entry)

MEMORY
{
  APP (rwx) : ORIGIN = 0x20020000, LENGTH = 64K
}

SECTIONS
{
  . = ORIGIN(APP);

  .text : {
    KEEP(*(.text.app_entry))   /* app_entryを専用セクションに */
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

  /* エントリシンボル */
  PROVIDE(app_entry = app_entry);
}
