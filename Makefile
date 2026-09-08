.POSIX:
.SECONDARY:

PROJECTS:=projects/hello \
	  projects/trapped \
	  projects/ping-pong

TEMPLATE:=projects/template
NAME    ?=

# Project described by compile_commands.json. Only one at a time: each
# project force-includes its own s3k_conf.h and rebuilds the kernel with it.
PROJECT ?= hello

all: ${PROJECTS}

${PROJECTS}: common

common ${PROJECTS}:
	@${MAKE} -C $@ all

clean:
	@for i in common ${PROJECTS}; do \
		${MAKE} -C $$i clean; \
	done

format:
	clang-format -i $$(find * -type f -name '*.[hc]')

# Regenerate compile_commands.json so clangd and the VS Code C/C++ extension
# see the same flags the build uses. Needs no compiler: it reads `make -n`.
compile-commands:
	@./scripts/gen-compile-commands.sh ${PROJECT} ${PLATFORM}

new:
	@if [ -z "${NAME}" ]; then \
		echo "usage: make new NAME=<project-name>"; \
		echo "creates projects/<project-name> from ${TEMPLATE}"; \
		exit 1; \
	fi
	@case "${NAME}" in \
	*/*|.|..) \
		echo "make new: '${NAME}' is not a valid project name"; \
		exit 1 ;; \
	esac
	@if [ -e projects/${NAME} ]; then \
		echo "make new: projects/${NAME} already exists"; \
		exit 1; \
	fi
	@if [ ! -d ${TEMPLATE} ]; then \
		echo "make new: missing ${TEMPLATE}"; \
		exit 1; \
	fi
	@cp -R ${TEMPLATE} projects/${NAME}
	@rm -rf projects/${NAME}/build
	@echo "created projects/${NAME}"
	@echo
	@echo "  build   make -C projects/${NAME}"
	@echo "  run     make -C projects/${NAME} qemu        (exit: C-a then x)"
	@echo "  debug   make -C projects/${NAME} qemu-gdb    (then: gdb)"
	@echo
	@echo "see projects/${NAME}/README.md"

.PHONY: all clean format new compile-commands common ${PROJECTS}
