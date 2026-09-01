app_platform_conf = configuration_data()
app_platform_conf.set('APP_RAM_ORIGIN', '0x80000000')
app_platform_conf.set('APP_RAM_LENGTH', '0x10000')
app_platform_conf.set('UART_BASE', app_uart_base)
app_platform_conf.set('VIRTIO_BASE', app_virtio_base)

configure_file(
  input: app_platform_ld_in,
  output: 'app.ld',
  configuration: app_platform_conf,
)

app_platform_ld = meson.current_build_dir() / 'app.ld'

app1_elf = executable(
	'app1.elf',
	sources: files(
		'head.S',
        'diskio.c',
        'ff.c',
        'ffsystem.c',
        'virtio_disk.c',
		'main.c',
	) + app_platform_uart,
	c_args: tutorial_app_c_args,
	link_args: [
		'-nostartfiles',
		'-specs=picolibc.specs',
		'-T', app_platform_ld,
	],
	dependencies: [
		libs3k_dep,
	],
)
