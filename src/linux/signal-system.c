/**\file
 *
 * \copyright
 * Copyright (c) 2008-2014, Kyuba Project Members
 * \copyright
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * \copyright
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * \copyright
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * \see Project Documentation: http://ef.gy/documentation/curie
 * \see Project Source Code: http://git.becquerel.org/kyuba/curie.git
*/

#include <syscall/syscall.h>
#include <curie/signal.h>
#include <curie/signal-system.h>
#include <curie/int.h>

#if !defined(__SIZE_TYPE__)
typedef unsigned long size_t;
#else
typedef __SIZE_TYPE__ size_t;
#endif

#define _LINUX_TYPES_H
#define _LINUX_TIME_H

#include <asm/signal.h>

typedef void (*signal_handler)(enum signal);
static signal_handler signal_handlers[(SIGNAL_MAX_NUM+1)];

static enum signal signum2signal (int signum) {
    switch (signum) {
        case SIGHUP:
            return sig_hup;
        case SIGINT:
            return sig_int;
        case SIGQUIT:
            return sig_quit;
        case SIGILL:
            return sig_ill;
        case SIGABRT:
            return sig_abrt;
        case SIGFPE:
            return sig_fpe;
        case SIGKILL:
            return sig_kill;
        case SIGSEGV:
            return sig_segv;
        case SIGPIPE:
            return sig_pipe;
        case SIGALRM:
            return sig_alrm;
        case SIGTERM:
            return sig_term;
        case SIGCHLD:
            return sig_chld;
        case SIGCONT:
            return sig_cont;
        case SIGSTOP:
            return sig_stop;
        case SIGTSTP:
            return sig_tstp;
        case SIGTTIN:
            return sig_ttin;
        case SIGTTOU:
            return sig_ttou;
        case SIGUSR1:
            return sig_usr1;
        case SIGUSR2:
            return sig_usr2;

        case SIGBUS:
            return sig_bus;
        case SIGPOLL:
            return sig_poll;
        case SIGPROF:
            return sig_prof;
        case SIGSYS:
            return sig_sys;
        case SIGTRAP:
            return sig_trap;
        case SIGURG:
            return sig_urg;
        case SIGVTALRM:
            return sig_vtalrm;
        case SIGXCPU:
            return sig_xcpu;
        case SIGXFSZ:
            return sig_xfsz;

#if SIGIOT != SIGABRT
        case SIGIOT:
            return sig_iot;
#endif
#if defined(SIGEMT)
        case SIGEMT:
            return sig_emt;
#endif
        case SIGSTKFLT:
            return sig_stkflt;
#if SIGIO != SIGPOLL
        case SIGIO:
            return sig_io;
#endif
        case SIGPWR:
            return sig_pwr;
#if defined(SIGINFO)
        case SIGINFO:
            return sig_info;
#endif
#if defined(SIGLOST)
        case SIGLOST:
            return sig_lost;
#endif
        case SIGWINCH:
            return sig_winch;

        default:
            return sig_unused;
    }
}

static int signal2signum (enum signal signal) {
    switch (signal) {
        case sig_hup:
            return SIGHUP;
        case sig_int:
            return SIGINT;
        case sig_quit:
            return SIGQUIT;
        case sig_ill:
            return SIGILL;
        case sig_abrt:
            return SIGABRT;
        case sig_fpe:
            return SIGFPE;
        case sig_kill:
            return SIGKILL;
        case sig_segv:
            return SIGSEGV;
        case sig_pipe:
            return SIGPIPE;
        case sig_alrm:
            return SIGALRM;
        case sig_term:
            return SIGTERM;
        case sig_chld:
            return SIGCHLD;
        case sig_cont:
            return SIGCONT;
        case sig_stop:
            return SIGSTOP;
        case sig_tstp:
            return SIGTSTP;
        case sig_ttin:
            return SIGTTIN;
        case sig_ttou:
            return SIGTTOU;
        case sig_usr1:
            return SIGUSR1;
        case sig_usr2:
            return SIGUSR2;

        case sig_bus:
            return SIGBUS;
        case sig_poll:
            return SIGPOLL;
        case sig_prof:
            return SIGPROF;
        case sig_sys:
            return SIGSYS;
        case sig_trap:
            return SIGTRAP;
        case sig_urg:
            return SIGURG;
        case sig_vtalrm:
            return SIGVTALRM;
        case sig_xcpu:
            return SIGXCPU;
        case sig_xfsz:
            return SIGXFSZ;

        case sig_iot:
            return SIGIOT;
#if defined(SIGEMT)
        case sig_emt:
            return SIGEMT;
#endif
        case sig_stkflt:
            return SIGSTKFLT;
        case sig_io:
            return SIGIO;
        case sig_pwr:
            return SIGPWR;
#if defined(SIGINFO)
        case sig_info:
            return SIGINFO;
#endif
#if defined(SIGLOST)
        case sig_lost:
            return SIGLOST;
#endif
        case sig_winch:
            return SIGWINCH;

        default:
            return sig_unused;
    }
}

static void sig_invoker (int signum) {
    enum signal signal = signum2signal(signum);

    if (signal != sig_unused) {
        signal_handler sighandler = signal_handlers[signal];

        sighandler(signal);
    }
}

#if !defined(sys_sigreturn_opcodes) && defined(linux_use_signal_restorer)
void a_sigreturn()
{
    sys_rt_sigreturn(0);
}
#endif

void a_set_signal_handler (enum signal signal, void (*handler)(enum signal signal)) {
    union {
        struct sigaction action;
        unsigned int actionui[sizeof (struct sigaction) / sizeof(unsigned int)];
    } x;
    unsigned long int i;
    int signum = signal2signum (signal);
#if defined(sys_sigreturn_opcodes) && defined(linux_use_signal_restorer)
    __attribute__((section(".text"))) static unsigned char sigret[] = sys_sigreturn_opcodes;
    union {
        void  *sigret_v;
        void (*sigret_f)(); 
    } y = { (void *)sigret };
#endif

    if (signum == sig_unused) return;

    for (i = 0; i < (sizeof (x.action) / sizeof(unsigned int)); i++) {
        x.actionui[i] = 0;
    }

    x.action.sa_handler = sig_invoker;
#if !defined(linux_use_signal_restorer)
    x.action.sa_flags = 0;
#else
    x.action.sa_flags = SA_RESTORER;

#if defined(sys_sigreturn_opcodes)
    x.action.sa_restorer = y.sigret_f;
#else
    x.action.sa_restorer = (void (*)())a_sigreturn;
#endif
#endif

    signal_handlers[signal] = handler;

#if defined(have_sys_sigaction)
    (void)sys_sigaction (signum, (void *)&(x.action), (void *)0);
#else
    (void)sys_rt_sigaction (signum, (void *)&(x.action), (void *)0, 8);
#endif
}
