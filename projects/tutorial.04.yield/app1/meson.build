subdir('platform')

app1_elf = executable(
	'app1.elf',
	sources: files(
		'head.S',
		'main.c',
	) + app1_platform_uart,
	include_directories: [
		app2_layout_inc,
	],
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
