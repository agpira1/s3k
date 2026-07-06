app1_ram_origin = '0x80000000'
app1_ram_length = '64K'

if (get_option('platform') == 'qemu_virt') or (get_option('platform') == 'qemu_virt4')
  app1_platform_uart = files('../../_mesonlib/tutorial_platform/ns16550a.c')
  app1_platform_ld_in = '../../_mesonlib/tutorial_platform/qemu_virt.ld.in'
  app1_uart_base = '0x10000000'
elif (get_option('platform') == 'cheshire') or (get_option('platform') == 'cheshire2')
  app1_platform_uart = files('../../_mesonlib/tutorial_platform/ti16750.c')
  app1_platform_ld_in = '../../_mesonlib/tutorial_platform/cheshire.ld.in'
  app1_uart_base = '0x03002000'
else
  error('Unknown platform: ' + get_option('platform'))
endif

app1_platform_conf = configuration_data()
app1_platform_conf.set('APP_RAM_ORIGIN', app1_ram_origin)
app1_platform_conf.set('APP_RAM_LENGTH', app1_ram_length)
app1_platform_conf.set('UART_BASE', app1_uart_base)

configure_file(
  input: app1_platform_ld_in,
  output: 'app1.ld',
  configuration: app1_platform_conf,
)

app1_platform_ld = meson.current_build_dir() / 'app1.ld'

app1_elf = executable(
	'app1.elf',
	sources: files(
		'head.S',
		'main.c',
	) + app1_platform_uart,
	c_args: tutorial_app_c_args,
	link_args: [
		'-nostartfiles',
		'-specs=picolibc.specs',
		'-T', app1_platform_ld,
	],
	dependencies: [
		libs3k_dep,
	],
)
