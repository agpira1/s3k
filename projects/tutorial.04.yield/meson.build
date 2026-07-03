project('tutorial.04.yield', 'c',
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
subdir('app2')

tutorial_app_elves = [app1_elf, app2_elf]
tutorial_qemu_loader_cpus = [0]

subdir('subprojects/s3k/projects/_mesonlib/tutorial_targets')
