app_platform_conf = configuration_data()
# actually the address should be platform-app dependent
app_platform_conf.set('APP_RAM_ORIGIN', '0x80020000')
app_platform_conf.set('APP_RAM_LENGTH', '0x10000')
app_platform_conf.set('UART_BASE', app_uart_base)
app_platform_conf.set('APP_LAYOUT_PREFIX', 'APP2')
app_platform_conf.set('VIRTIO_BASE', app_virtio_base)



configure_file(
  input: app_platform_ld_in,
  output: 'app.ld',
  configuration: app_platform_conf,
)

configure_file(
  input: '../../_mesonlib/tutorial_platform/app_layout.h.in',
  output: 'app2_layout.h',
  configuration: app_platform_conf,
)

app_platform_ld = meson.current_build_dir() / 'app.ld'
app2_layout_inc = include_directories('.')

app2_elf = executable(
	'app2.elf',
	sources: files(
		'head.S',
		'main.c',
	) + app_platform_uart,
	c_args: tutorial_app_c_args,
	link_args: [
		'-nostartfiles',
		'-specs=picolibc.specs',
		'-T', app_platform_ld ,
	],
	dependencies: [
		libs3k_dep,
	],
)
