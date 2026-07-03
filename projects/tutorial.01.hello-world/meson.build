project('tutorial.01.hello-world', 'c',
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

subdir('app1')

tutorial_app_elves = [app1_elf]
tutorial_qemu_loader_cpus = [0]

subdir('subprojects/s3k/projects/_mesonlib/tutorial_targets')
