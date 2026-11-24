/* Arm semihosting for Cortex-M SultanC images run under QEMU
 * (`-semihosting-config enable=on,target=native`) or a debugger. Given to
 * the linker after the board script, it selects the semihosting forms of
 * the start-up the compiler emits into a program's object
 * (compiler/backend/provider/llvm/cortex_m_entry.sn): main's result
 * becomes the exit status (SYS_EXIT_EXTENDED), and a HardFault (a SultanC
 * trap) exits with status 132. Without it the image stops in a WFI loop
 * after main and spins in Default_Handler on a fault: on hardware with no
 * debugger attached BKPT would itself fault. */
__sultanc_exit = __sultanc_semihosting_exit;
HardFault_Handler = __sultanc_semihosting_fault;
