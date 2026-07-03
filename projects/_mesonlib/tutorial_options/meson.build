# Shared app compiler flags derived from tutorial Meson options.
# Add new option-to-macro mappings here so app builds pick them up automatically.
tutorial_option_macros = {
  'nproc': ['NPROC'],
  'ntimeslot': ['MAX_TIME_SLOT'],
  'nmemoryfuel': ['MAX_MEMORY_FUEL'],
  'ntimefuel': ['MAX_TIME_FUEL'],
  'nmonitorfuel': ['MAX_MONITOR_FUEL'],
  'nipcfuel': ['MAX_IPC_FUEL'],
  'platform': ['PLATFORM'],
  'cspad': ['CSPAD'],
  'uart_stride': ['UART_REG_STRIDE'],
}

tutorial_app_c_args = ['-specs=picolibc.specs']
foreach option_name, macro_names : tutorial_option_macros
  option_value = '@0@'.format(get_option(option_name))
  foreach macro_name : macro_names
    tutorial_app_c_args += ['-D' + macro_name + '=' + option_value]
  endforeach
endforeach
