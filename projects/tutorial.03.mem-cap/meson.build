project('tutorial.03.mem-cap', 'c',
	version: '0.1',
	meson_version: '>=1.1.0',
	default_options: [
		'buildtype=debugoptimized',
		'c_std=gnu11',
	]
)

s3k = subproject('s3k')
libs3k_dep = s3k.get_variable('lib_dep')
s3k_elf = s3k.get_variable('elf')

subdir('subprojects/s3k/projects/_mesonlib/tutorial_options')
subdir('app1')

tutorial_app_elves = [app1_elf]
tutorial_gdb_commands = [
	'-ex', 'set confirm off',
	'-ex', 'set pagination off',
	'-ex', 'target extended-remote 10.100.0.1:3333',
	'-ex', 'set *(int*)0x3001000 = 0x0f',
	'-ex', 'set *(int*)0x3001010 = 0x1',
	'-ex', 'load ' + app1_elf.full_path(),
	'-ex', 'load ' + s3k_elf.full_path(),
	'-ex', 'continue',
]
tutorial_qemu_loader_cpus = [0]

subdir('subprojects/s3k/projects/_mesonlib/tutorial_targets')
