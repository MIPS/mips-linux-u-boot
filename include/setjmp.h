/*
 * Copyright (c) 2016 Imagination Technologies
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __SETJMP_H__
#define __SETJMP_H__

#ifdef CONFIG_HAVE_ARCH_SETJMP
#include <asm/setjmp.h>
#endif

/**
 * setjmp() - Saves CPU state to allow multiple returns
 * @env: buffer to save state to
 *
 * Call to save CPU state into @env allowing execution to return to this point
 * via a later call to longjmp(). That is, this function may return multiple
 * times & callers should be sure that they are prepared for this by not using
 * non-volatile local variables that may be modified between the calls to
 * setjmp() & longjmp(). The caller should not return before any calls to
 * longjmp(), since that may lead execution to return to a function whose stack
 * frame has been lost or reused.
 *
 * Return: 0 when directly invoked, or val provided to longjmp()
 */
extern int setjmp(jmp_buf env);

/**
 * longjmp() - Return execution to a point previously saved by setjmp()
 * @env: buffer containing state saved by setjmp()
 * @val: the value that setjmp() will return
 *
 * Return execution to whatever called setjmp() on @env, with that call
 * returning @val. See setjmp() for further details.
 */
extern void __noreturn longjmp(jmp_buf env, int val);

#endif /* __SETJMP_H__ */
