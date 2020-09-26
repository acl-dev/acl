// Stub file for universal binary

#if defined(__APPLE__)
    #include "jump_combined_sysv_macho_gas.S"
#elif defined(_WIN32) || defined(__MINGW32__) || defined(__MINGW64__) || defined(__CYGWIN__) || defined(__MSYS__)
    #if defined(__x86_64__)
        #include "jump_x86_64_ms_pe_gas.asm"
    #elif defined(__i386__)
        #include "jump_i386_ms_pe_gas.asm"
    #endif
#elif defined(__x86_64__)
    #include "jump_x86_64_sysv_elf_gas.S"
#elif defined(__i386__)
    #include "jump_i386_sysv_elf_gas.S"
#elif defined(__ppc64__)
    #include "jump_ppc64_sysv_elf_gas.S"
#elif defined(__ppc__)
    #include "jump_ppc32_sysv_elf_gas.S"
#elif defined(__aarch64__)
    #include "jump_arm64_aapcs_elf_gas.S"
#elif defined(__arm__)
    #include "jump_arm_aapcs_elf_gas.S"
#elif defined(__mips64)
    #include "jump_mips64_n64_elf_gas.S"
#elif defined(__mips__)
    #include "jump_mips32_o32_elf_gas.S"
#elif defined(__riscv)
    #include "jump_riscv64_sysv_elf_gas.S"
#elif defined(__s390x__)
    #include "jump_s390x_sysv_elf_gas.S"
#else
    #error "No arch's"
#endif
