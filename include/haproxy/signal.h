/*
 * include/haproxy/signal.h
 * Asynchronous signal delivery functions.
 *
 * Copyright 2000-2010 Willy Tarreau <w@1wt.eu>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 */

#ifndef _HAPROXY_SIGNAL_H
#define _HAPROXY_SIGNAL_H

#include <signal.h>

#include <haproxy/api.h>
#include <haproxy/signal-t.h>
#include <haproxy/task-t.h>
#include <haproxy/thread.h>

extern int signal_queue_len;
extern struct signal_descriptor signal_state[];

__decl_thread(extern HA_SPINLOCK_T signals_lock);

void signal_handler(int sig);
void __signal_process_queue();
void deinit_signals();
struct sig_handler *signal_register_fct(int sig, void (*fct)(struct sig_handler *), int arg);
struct sig_handler *signal_register_task(int sig, struct task *task, int reason);
void signal_unregister_handler(struct sig_handler *handler);
void signal_unregister_target(int sig, void *target);
void signal_unregister(int sig);
void haproxy_unblock_signals();

static inline void signal_process_queue()
{
	if (unlikely(signal_queue_len > 0))
		__signal_process_queue();
}

#endif /* _HAPROXY_SIGNAL_H */

/*
 * Local variables:
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 */
