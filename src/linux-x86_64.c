/*
 * Copyright (c) 2013, Juniper Networks, Inc.
 * All rights reserved.
 *
 * You may distribute under the terms of :
 *
 * the BSD 2-Clause license
 *
 * Any patches released for this software are to be released under these
 * same license terms.
 *
 * BSD 2-Clause license:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "target_ptrace.h"
#include "global.h"
#include "os.h"

#define GDB_RAX       0
#define GDB_RBX       1
#define GDB_RCX       2
#define GDB_RDX       3
#define GDB_RSI       4
#define GDB_RDI       5
#define GDB_RBP       6
#define GDB_RSP       7
#define GDB_R8        8
#define GDB_R9        9
#define GDB_R10      10
#define GDB_R11      11
#define GDB_R12      12
#define GDB_R13      13
#define GDB_R14      14
#define GDB_R15      15
#define GDB_RIP      16
#define GDB_RFLAGS   17
#define GDB_CS       18
#define GDB_SS       19
#define GDB_DS       20
#define GDB_ES       21
#define GDB_FS       22
#define GDB_GS       23

#define GDB_ORIG_RAX 57

struct reg_location_list grll[] = {
	/* general */
	GRLL(r15,      regs.r15,      GDB_R15,      0, 0, 0),
	GRLL(r14,      regs.r14,      GDB_R14,      0, 0, 0),
	GRLL(r13,      regs.r13,      GDB_R13,      0, 0, 0),
	GRLL(r12,      regs.r12,      GDB_R12,      0, 0, 0),
	GRLL(r11,      regs.r11,      GDB_R11,      0, 0, 0),
	GRLL(r10,      regs.r10,      GDB_R10,      0, 0, 0),
	GRLL(r9,       regs.r9,       GDB_R9,       0, 0, 0),
	GRLL(r8,       regs.r8,       GDB_R8,       0, 0, 0),
	GRLL(rsi,      regs.rsi,      GDB_RSI,      0, 0, 0),
	GRLL(rdi,      regs.rdi,      GDB_RDI,      0, 0, 0),
	GRLL(orig_rax, regs.orig_rax, GDB_ORIG_RAX, 0, 0, 0),
	GRLL(rbp,      regs.rbp,      GDB_RBP,      0, 0, 0),
	GRLL(rbx,      regs.rbx,      GDB_RBX,      0, 0, 0),
	GRLL(rdx,      regs.rdx,      GDB_RDX,      0, 0, 0),
	GRLL(rcx,      regs.rcx,      GDB_RCX,      0, 0, 0),
	GRLL(rax,      regs.rax,      GDB_RAX,      0, 0, 0),
	GRLL(fs,       regs.fs,       GDB_FS,       0, 0, 4),
	GRLL(gs,       regs.gs,       GDB_GS,       0, 0, 4),
	GRLL(es,       regs.es,       GDB_ES,       0, 0, 4),
	GRLL(ds,       regs.ds,       GDB_DS,       0, 0, 4),
	GRLL(rip,      regs.rip,      GDB_RIP,      0, 0, 0),
	GRLL(cs,       regs.cs,       GDB_CS,       0, 2, 4),
	GRLL(rflags,   regs.eflags,   GDB_RFLAGS,   0, 0, 4),
	GRLL(rsp,      regs.rsp,      GDB_RSP,      0, 0, 4),
	GRLL(ss,       regs.ss,       GDB_SS,       0, 2, 4),
	{0},
};

#define GDB_FST0  24
#define GDB_FST1  25
#define GDB_FST2  26
#define GDB_FST3  27
#define GDB_FST4  28
#define GDB_FST5  29
#define GDB_FST6  30
#define GDB_FST7  31
#define GDB_FCTRL 32
#define GDB_FSTAT 33
#define GDB_FTAG  34
#define GDB_FISEG 35
#define GDB_FIOFF 36
#define GDB_FOSEG 37
#define GDB_FOOFF 38
#define GDB_FOP   39
#define GDB_XMM0  40
#define GDB_XMM1  41
#define GDB_XMM2  42
#define GDB_XMM3  43
#define GDB_XMM4  44
#define GDB_XMM5  45
#define GDB_XMM6  46
#define GDB_XMM7  47
#define GDB_XMM8  48
#define GDB_XMM9  49
#define GDB_XMM10 50
#define GDB_XMM11 51
#define GDB_XMM12 52
#define GDB_XMM13 53
#define GDB_XMM14 54
#define GDB_XMM15 55
#define GDB_MXCSR 56

#define FXRLL(N, E, GDB, O, S, GDB_S)					\
{								\
	.off = (O) + offsetof(struct user_fpregs_struct, E),	\
	.size = (0 != S) ? (S) : msizeof(struct user_fpregs_struct, E), \
	.gdb = (GDB),					\
	.name = #N,					\
	.gdb_size = (0 != GDB_S) ? (GDB_S) : msizeof(struct user_fpregs_struct, E), \
			}

struct reg_location_list frll[] = {
	FRLL(st0,  st_space, GDB_FST0,   0, 10, 10),
	FRLL(st1,  st_space, GDB_FST1,  10, 10, 10),
	FRLL(st2,  st_space, GDB_FST2,  20, 10, 10),
	FRLL(st3,  st_space, GDB_FST3,  30, 10, 10),
	FRLL(st4,  st_space, GDB_FST4,  40, 10, 10),
	FRLL(st5,  st_space, GDB_FST5,  50, 10, 10),
	FRLL(st6,  st_space, GDB_FST6,  60, 10, 10),
	FRLL(st7,  st_space, GDB_FST7,  70, 10, 10),
	FRLL(cwd,       cwd, GDB_FCTRL,  0,  2,  4),
	FRLL(swd,       swd, GDB_FSTAT,  0,  2,  4),
	FRLL(ftw,       ftw, GDB_FTAG,   0,  2,  4),
	FRLL(fop,       fop, GDB_FOP,    0,  2,  4),
	FRLL(mm0,   xmm_space, GDB_XMM0,  0x00, 0x10, 0x10),
	FRLL(mm1,   xmm_space, GDB_XMM1,  0x10, 0x10, 0x10),
	FRLL(mm2,   xmm_space, GDB_XMM2,  0x20, 0x10, 0x10),
	FRLL(mm3,   xmm_space, GDB_XMM3,  0x30, 0x10, 0x10),
	FRLL(mm4,   xmm_space, GDB_XMM4,  0x40, 0x10, 0x10),
	FRLL(mm5,   xmm_space, GDB_XMM5,  0x50, 0x10, 0x10),
	FRLL(mm6,   xmm_space, GDB_XMM6,  0x60, 0x10, 0x10),
	FRLL(mm7,   xmm_space, GDB_XMM7,  0x70, 0x10, 0x10),
	FRLL(mm8,   xmm_space, GDB_XMM8,  0x80, 0x10, 0x10),
	FRLL(mm9,   xmm_space, GDB_XMM9,  0x90, 0x10, 0x10),
	FRLL(mm10,  xmm_space, GDB_XMM10, 0xa0, 0x10, 0x10),
	FRLL(mm11,  xmm_space, GDB_XMM11, 0xb0, 0x10, 0x10),
	FRLL(mm12,  xmm_space, GDB_XMM12, 0xc0, 0x10, 0x10),
	FRLL(mm13,  xmm_space, GDB_XMM13, 0xd0, 0x10, 0x10),
	FRLL(mm14,  xmm_space, GDB_XMM14, 0xe0, 0x10, 0x10),
	FRLL(mm15,  xmm_space, GDB_XMM15, 0xf0, 0x10, 0x10),
	FRLL(mxcsr, mxcsr,     GDB_MXCSR,    0,    0,    0),
	{0},
};

struct reg_location_list fxrll[] = {
	{0},
};

void ptrace_arch_read_fxreg()
{
	/* part of fp regs */
}

void ptrace_arch_write_fxreg()
{
	/* part of fp regs */
}

#ifdef PT_GETXSTATE
#define GDB_GREG_MAX 49
#else
#define GDB_GREG_MAX 40
#endif

int ptrace_arch_gdb_greg_max()
{
	return 24;
}

void ptrace_arch_get_pc(unsigned long *pc)
{
	_read_greg();
	memcpy(pc, _target.reg + offsetof(struct user, regs.rip),
	       sizeof(unsigned long));
}

void ptrace_arch_set_pc(unsigned long pc)
{
	_read_greg();
	memcpy(_target.reg + offsetof(struct user, regs.rip), &pc,
	       sizeof(unsigned long));
	_write_greg();
}

void ptrace_arch_option_set_syscall(pid_t pid)
{
	ptrace_os_option_set_syscall(pid);
}

bool ptrace_arch_check_syscall(pid_t pid, int *in_out_sig)
{
	return false;
}

void ptrace_arch_get_syscall(void *id, void *arg1, void *arg2,
			     void *arg3, void *arg4, void *ret)
{
	_read_greg();
}

bool ptrace_arch_check_unrecognized_register(int reg, size_t *pad_size)
{
	bool ret = false;
	if (GDB_FISEG == reg) {
		*pad_size = 4;
		ret = true;
	} else if (GDB_FIOFF == reg) {
		*pad_size = 4;
		ret = true;
	} else if (GDB_FOSEG == reg) {
		*pad_size = 4;
		ret = true;
	} else if (GDB_FOOFF == reg) {
		*pad_size = 4;
		ret = true;
	}
	return ret;
}
