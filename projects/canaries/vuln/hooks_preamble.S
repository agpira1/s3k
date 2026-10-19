

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

.global __cyg_profile_func_exit            /* 1. Makes the function visible to the C linker */
.text                                       /* 2. Puts the code in the executable code segment */
.align 4                                    /* 3. Aligns the instructions for CPU efficiency */
.type __cyg_profile_func_exit, @function   /* 4. Tells the toolchain this label is a function */


__cyg_profile_func_exit:     // Write a capture of SP again which is frame_high to send to pop.

    mv a2, sp
    addi sp, sp, -16
    sd ra, 8(sp)
    call __cyg_profile_func_exit_c
    ld ra, 8(sp)
    addi sp, sp, 16
    ret
    
    .size __cyg_profile_func_exit, .-__cyg_profile_func_exit


    // An hour per person oral exam. Monday 26th. By 21. What are the limitations. We go through the code and he asks questions. Why did i do something. 
    // Then some questions about the papers. 