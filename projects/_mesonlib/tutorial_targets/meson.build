tutorial_target_deps = [s3k_elf]
foreach elf : tutorial_app_elves
  tutorial_target_deps += elf
endforeach

gdb_riscv64 = find_program('riscv64-unknown-elf-gdb', required: false)
run_target(
  'gdb-run',
  command: [gdb_riscv64] + tutorial_gdb_commands,
  depends: tutorial_target_deps,
)

qemu_system_riscv64 = find_program('qemu-system-riscv64', required: false)
tutorial_qemu_run_command = [
  qemu_system_riscv64,
  '-machine', 'virt',
  '-bios', 'none',
  '-kernel', s3k_elf.full_path(),
  '-nographic',
  '-m', '1G',
  '-icount', '1',
]
foreach elf : tutorial_app_elves
  tutorial_qemu_run_command += ['-device', 'loader,file=' + elf.full_path()]
endforeach
foreach cpu_num : tutorial_qemu_loader_cpus
  tutorial_qemu_run_command += ['-device', 'loader,addr=0x90000000,cpu-num=' + cpu_num.to_string()]
endforeach

run_target(
  'qemu-run',
  command: tutorial_qemu_run_command,
  depends: tutorial_target_deps,
)

run_target(
  'qemu-debug',
  command: tutorial_qemu_run_command + ['-gdb', 'tcp::3333', '-S'],
  depends: tutorial_target_deps,
)
