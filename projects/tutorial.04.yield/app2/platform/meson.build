app2_ram_origin = '0x80020000'
app2_ram_size = '0x10000'
app2_ram_length = '64K'

if (get_option('platform') == 'qemu_virt')
  app2_platform_uart = files('ns16550a.c')
  app2_platform_ld_in = meson.current_source_dir() / 'qemu_virt.ld'
elif (get_option('platform') == 'cheshire') or (get_option('platform') == 'cheshire2')
  app2_platform_uart = files('ti16750.c')
  app2_platform_ld_in = meson.current_source_dir() / 'cheshire.ld'
else
  error('Unknown platform: ' + get_option('platform'))
endif

app2_platform_conf = configuration_data()
app2_platform_conf.set('APP2_RAM_ORIGIN', app2_ram_origin)
app2_platform_conf.set('APP2_RAM_SIZE', app2_ram_size)
app2_platform_conf.set('APP2_RAM_LENGTH', app2_ram_length)

app2_platform_ld = configure_file(
  input: app2_platform_ld_in,
  output: 'app2.ld',
  configuration: app2_platform_conf,
)

configure_file(
  input: 'app2_layout.h.in',
  output: 'app2_layout.h',
  configuration: app2_platform_conf,
)

app2_layout_inc = include_directories('.')
