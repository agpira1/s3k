app2_ram_origin = '0x80020000'
app2_ram_size = '0x10000'
app2_ram_length = '64K'

if (get_option('platform') == 'qemu_virt') or (get_option('platform') == 'qemu_virt4')
  app2_platform_uart = files('../../../_mesonlib/tutorial_platform/ns16550a.c')
  app2_platform_ld_in = '../../../_mesonlib/tutorial_platform/qemu_virt.ld.in'
  app2_uart_base = '0x10000000'
elif (get_option('platform') == 'cheshire') or (get_option('platform') == 'cheshire2')
  app2_platform_uart = files('../../../_mesonlib/tutorial_platform/ti16750.c')
  app2_platform_ld_in = '../../../_mesonlib/tutorial_platform/cheshire.ld.in'
  app2_uart_base = '0x03002000'
else
  error('Unknown platform: ' + get_option('platform'))
endif

app2_platform_conf = configuration_data()
app2_platform_conf.set('APP2_RAM_ORIGIN', app2_ram_origin)
app2_platform_conf.set('APP2_RAM_SIZE', app2_ram_size)
app2_platform_conf.set('APP2_RAM_LENGTH', app2_ram_length)
app2_platform_conf.set('APP_RAM_ORIGIN', app2_ram_origin)
app2_platform_conf.set('APP_RAM_SIZE', app2_ram_size)
app2_platform_conf.set('APP_RAM_LENGTH', app2_ram_length)
app2_platform_conf.set('APP_LAYOUT_PREFIX', 'APP2')
app2_platform_conf.set('UART_BASE', app2_uart_base)

configure_file(
  input: app2_platform_ld_in,
  output: 'app2.ld',
  configuration: app2_platform_conf,
)

configure_file(
  input: '../../../_mesonlib/tutorial_platform/app_layout.h.in',
  output: 'app2_layout.h',
  configuration: app2_platform_conf,
)

app2_platform_ld = meson.current_build_dir() / 'app2.ld'
app2_layout_inc = include_directories('.')
