.POSIX:
.SECONDARY:

PROJECTS:=projects/hello \
	  projects/trapped \
	  projects/ping-pong

TEMPLATE:=projects/template
NAME    ?=

# Highest application index created by `make new`. 0 is app0 alone, 1 is app0
# and app1, and so on.
APPS    ?=0

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
		echo "usage: make new NAME=<project-name> [APPS=<n>]"; \
		echo "creates projects/<project-name> from ${TEMPLATE}"; \
		echo "APPS is the highest application index: 0 creates app0,"; \
		echo "1 creates app0 and app1, and so on"; \
		exit 1; \
	fi
	@TEMPLATE="${TEMPLATE}" ./scripts/new-project.sh "${NAME}" "${APPS}"

.PHONY: all clean format new compile-commands common ${PROJECTS}
