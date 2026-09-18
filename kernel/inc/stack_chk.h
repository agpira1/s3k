#pragma once

void stack_chk_guard_init(void);
void __stack_chk_fail(void) __attribute__((noreturn));
