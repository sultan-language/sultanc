/* Section layout of a Cortex-M SultanC image. A board script defines the
 * FLASH and RAM memory regions and then includes this file, for example
 *
 *   MEMORY {
 *     FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 256K
 *     RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 64K
 *   }
 *   INCLUDE cortex_m.ld
 *
 * The compiler emits the vector table and the reset handler into the object
 * of every Cortex-M program (compiler/backend/provider/llvm/cortex_m_entry.sn).
 * The vector table is the first word of FLASH; the stack starts at the end
 * of RAM (8-byte aligned, as AAPCS requires at public interfaces). .data is
 * loaded in FLASH and copied to RAM by the reset handler, which also zeroes
 * .bss and runs .init_array. */

ENTRY(Reset_Handler)

SECTIONS
{
  .vector_table ORIGIN(FLASH) :
  {
    KEEP(*(.vector_table))
  } > FLASH

  .text :
  {
    *(.text .text.*)
  } > FLASH

  .rodata :
  {
    *(.rodata .rodata.*)
    . = ALIGN(4);
  } > FLASH

  .init_array :
  {
    __init_array_start = .;
    KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*)))
    KEEP(*(.init_array))
    __init_array_end = .;
  } > FLASH

  .ARM.extab :
  {
    *(.ARM.extab* .gnu.linkonce.armextab.*)
  } > FLASH

  .ARM.exidx :
  {
    __exidx_start = .;
    *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    __exidx_end = .;
  } > FLASH

  .data : ALIGN(4)
  {
    __data_start = .;
    *(.data .data.*)
    . = ALIGN(4);
    __data_end = .;
  } > RAM AT > FLASH
  __data_load = LOADADDR(.data);

  .bss (NOLOAD) : ALIGN(4)
  {
    __bss_start = .;
    *(.bss .bss.* COMMON)
    . = ALIGN(4);
    __bss_end = .;
  } > RAM

  __stack_top = (ORIGIN(RAM) + LENGTH(RAM)) & ~7;
}
