

.global __cyg_profile_func_enter            /* 1. Makes the function visible to the C linker */
.text                                       /* 2. Puts the code in the executable code segment */
.align 4                                    /* 3. Aligns the instructions for CPU efficiency */
.type __cyg_profile_func_enter, @function   /* 4. Tells the toolchain this label is a function */

__cyg_profile_func_enter:                   /* 5. The function entry point (label) */
    # Save the stack pointer as argument 3 to not overwrite it. 
    mv a2, sp

    # Create the finstruct's own stack frame, changing the stack pointer.
    # (We do not need all -16 bytes on the stack. Only 8 are utilized but we need multiples of 16 as stack frame size in risc-v.)
    addi sp, sp, -16

    # Save the caller return address.
    sd ra, 8(sp)

    # Call normal C code.
    call __cyg_profile_func_enter_c

    # Restore caller return address.
    ld ra, 8(sp)

    # Remove hook's frame.
    addi sp, sp, 16

    # Return to the instrumented function.
    ret

    .size __cyg_profile_func_enter, .-__cyg_profile_func_enter