/*
 * include/haproxy/server.h
 * This file defines everything related to servers.
 *
 * Copyright (C) 2000-2009 Willy Tarreau - w@1wt.eu
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, version 2.1
 * exclusively.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _HAPROXY_SERVER_H
#define _HAPROXY_SERVER_H

#include <unistd.h>

#include <haproxy/api.h>
#include <haproxy/applet-t.h>
#include <haproxy/freq_ctr.h>
#include <haproxy/proxy-t.h>
#include <haproxy/resolvers-t.h>
#include <haproxy/server-t.h>
#include <haproxy/task.h>
#include <haproxy/thread-t.h>
#include <haproxy/time.h>


__decl_thread(extern HA_SPINLOCK_T idle_conn_srv_lock);
extern struct idle_conns idle_conns[MAX_THREADS];
extern struct task *idle_conn_task;
extern struct list servers_list;
extern struct dict server_key_dict;

int srv_downtime(const struct server *s);
int srv_lastsession(const struct server *s);
int srv_getinter(const struct check *check);
void srv_settings_cpy(struct server *srv, const struct server *src, int srv_tmpl);
int parse_server(const char *file, int linenum, char **args, struct proxy *curproxy, const struct proxy *defproxy, int parse_flags);
int srv_update_addr(struct server *s, void *ip, int ip_sin_family, const char *updater);
int server_parse_sni_expr(struct server *newsrv, struct proxy *px, char **err);
const char *srv_update_addr_port(struct server *s, const char *addr, const char *port, char *updater);
const char *srv_update_check_addr_port(struct server *s, const char *addr, const char *port);
const char *srv_update_agent_addr_port(struct server *s, const char *addr, const char *port);
struct server *server_find_by_id(struct proxy *bk, int id);
struct server *server_find_by_name(struct proxy *bk, const char *name);
struct server *server_find_best_match(struct proxy *bk, char *name, int id, int *diff);
void apply_server_state(void);
void srv_compute_all_admin_states(struct proxy *px);
int srv_set_addr_via_libc(struct server *srv, int *err_code);
int srv_init_addr(void);
struct server *cli_find_server(struct appctx *appctx, char *arg);
struct server *new_server(struct proxy *proxy);
void srv_take(struct server *srv);
struct server *srv_drop(struct server *srv);
int srv_init_per_thr(struct server *srv);
void srv_set_ssl(struct server *s, int use_ssl);

/* functions related to server name resolution */
int srv_prepare_for_resolution(struct server *srv, const char *hostname);
int srvrq_update_srv_status(struct server *s, int has_no_ip);
int snr_update_srv_status(struct server *s, int has_no_ip);
int srv_set_fqdn(struct server *srv, const char *fqdn, int resolv_locked);
const char *srv_update_fqdn(struct server *server, const char *fqdn, const char *updater, int dns_locked);
int snr_resolution_cb(struct resolv_requester *requester, struct dns_counters *counters);
int srvrq_resolution_error_cb(struct resolv_requester *requester, int error_code);
int snr_resolution_error_cb(struct resolv_requester *requester, int error_code);
struct server *snr_check_ip_callback(struct server *srv, void *ip, unsigned char *ip_family);
struct task *srv_cleanup_idle_conns(struct task *task, void *ctx, unsigned int state);
void srv_release_conn(struct server *srv, struct connection *conn);
struct connection *srv_lookup_conn(struct eb_root *tree, uint64_t hash);
struct connection *srv_lookup_conn_next(struct connection *conn);

int srv_add_to_idle_list(struct server *srv, struct connection *conn, int is_safe);
struct task *srv_cleanup_toremove_conns(struct task *task, void *context, unsigned int state);

int srv_apply_track(struct server *srv, struct proxy *curproxy);

/*
 * Registers the server keyword list <kwl> as a list of valid keywords for next
 * parsing sessions.
 */
void srv_register_keywords(struct srv_kw_list *kwl);

/* Return a pointer to the server keyword <kw>, or NULL if not found. */
struct srv_kw *srv_find_kw(const char *kw);

/* Dumps all registered "server" keywords to the <out> string pointer. */
void srv_dump_kws(char **out);

/* Recomputes the server's eweight based on its state, uweight, the current time,
 * and the proxy's algorithm. To be used after updating sv->uweight. The warmup
 * state is automatically disabled if the time is elapsed.
 */
void server_recalc_eweight(struct server *sv, int must_update);

/*
 * Parses weight_str and configures sv accordingly.
 * Returns NULL on success, error message string otherwise.
 */
const char *server_parse_weight_change_request(struct server *sv,
					       const char *weight_str);

/*
 * Parses addr_str and configures sv accordingly. updater precise
 * the source of the change in the associated message log.
 * Returns NULL on success, error message string otherwise.
 */
const char *server_parse_addr_change_request(struct server *sv,
                                             const char *addr_str, const char *updater);

/*
 * Parses maxconn_str and configures sv accordingly.
 * Returns NULL on success, error message string otherwise.
 */
const char *server_parse_maxconn_change_request(struct server *sv,
					       const char *maxconn_str);

/* Shutdown all connections of a server. The caller must pass a termination
 * code in <why>, which must be one of SF_ERR_* indicating the reason for the
 * shutdown.
 */
void srv_shutdown_streams(struct server *srv, int why);

/* Shutdown all connections of all backup servers of a proxy. The caller must
 * pass a termination code in <why>, which must be one of SF_ERR_* indicating
 * the reason for the shutdown.
 */
void srv_shutdown_backup_streams(struct proxy *px, int why);

void srv_append_status(struct buffer *msg, struct server *s, struct check *,
		       int xferred, int forced);

void srv_set_stopped(struct server *s, const char *reason, struct check *check);
void srv_set_running(struct server *s, const char *reason, struct check *check);
void srv_set_stopping(struct server *s, const char *reason, struct check *check);

/* Enables admin flag <mode> (among SRV_ADMF_*) on server <s>. This is used to
 * enforce either maint mode or drain mode. It is not allowed to set more than
 * one flag at once. The equivalent "inherited" flag is propagated to all
 * tracking servers. Maintenance mode disables health checks (but not agent
 * checks). When either the flag is already set or no flag is passed, nothing
 * is done. If <cause> is non-null, it will be displayed at the end of the log
 * lines to justify the state change.
 */
void srv_set_admin_flag(struct server *s, enum srv_admin mode, const char *cause);

/* Disables admin flag <mode> (among SRV_ADMF_*) on server <s>. This is used to
 * stop enforcing either maint mode or drain mode. It is not allowed to set more
 * than one flag at once. The equivalent "inherited" flag is propagated to all
 * tracking servers. Leaving maintenance mode re-enables health checks. When
 * either the flag is already cleared or no flag is passed, nothing is done.
 */
void srv_clr_admin_flag(struct server *s, enum srv_admin mode);

/* Calculates the dynamic persitent cookie for a server, if a secret key has
 * been provided.
 */
void srv_set_dyncookie(struct server *s);

int srv_check_reuse_ws(struct server *srv);
const struct mux_ops *srv_get_ws_proto(struct server *srv);

/* increase the number of cumulated connections on the designated server */
static inline void srv_inc_sess_ctr(struct server *s)
{
	_HA_ATOMIC_INC(&s->counters.cum_sess);
	HA_ATOMIC_UPDATE_MAX(&s->counters.sps_max,
			     update_freq_ctr(&s->sess_per_sec, 1));
}

/* set the time of last session on the designated server */
static inline void srv_set_sess_last(struct server *s)
{
	s->counters.last_sess = now.tv_sec;
}

/* returns the current server throttle rate between 0 and 100% */
static inline unsigned int server_throttle_rate(struct server *sv)
{
	struct proxy *px = sv->proxy;

	/* when uweight is 0, we're in soft-stop so that cannot be a slowstart,
	 * thus the throttle is 100%.
	 */
	if (!sv->uweight)
		return 100;

	return (100U * px->lbprm.wmult * sv->cur_eweight + px->lbprm.wdiv - 1) / (px->lbprm.wdiv * sv->uweight);
}

/*
 * Return true if the server has a zero user-weight, meaning it's in draining
 * mode (ie: not taking new non-persistent connections).
 */
static inline int server_is_draining(const struct server *s)
{
	return !s->uweight || (s->cur_admin & SRV_ADMF_DRAIN);
}

/* Puts server <s> into maintenance mode, and propagate that status down to all
 * tracking servers.
 */
static inline void srv_adm_set_maint(struct server *s)
{
	srv_set_admin_flag(s, SRV_ADMF_FMAINT, NULL);
	srv_clr_admin_flag(s, SRV_ADMF_FDRAIN);
}

/* Puts server <s> into drain mode, and propagate that status down to all
 * tracking servers.
 */
static inline void srv_adm_set_drain(struct server *s)
{
	srv_set_admin_flag(s, SRV_ADMF_FDRAIN, NULL);
	srv_clr_admin_flag(s, SRV_ADMF_FMAINT);
}

/* Puts server <s> into ready mode, and propagate that status down to all
 * tracking servers.
 */
static inline void srv_adm_set_ready(struct server *s)
{
	srv_clr_admin_flag(s, SRV_ADMF_FDRAIN);
	srv_clr_admin_flag(s, SRV_ADMF_FMAINT);
}

/* appends an initaddr method to the existing list. Returns 0 on failure. */
static inline int srv_append_initaddr(unsigned int *list, enum srv_initaddr addr)
{
	int shift = 0;

	while (shift + 3 < 32 && (*list >> shift))
		shift += 3;

	if (shift + 3 > 32)
		return 0;

	*list |= addr << shift;
	return 1;
}

/* returns the next initaddr method and removes it from <list> by shifting
 * it right (implying that it MUST NOT be the server's. Returns SRV_IADDR_END
 * at the end.
 */
static inline enum srv_initaddr srv_get_next_initaddr(unsigned int *list)
{
	enum srv_initaddr ret;

	ret = *list & 7;
	*list >>= 3;
	return ret;
}

static inline void srv_use_conn(struct server *srv, struct connection *conn)
{
	unsigned int curr;

	curr = _HA_ATOMIC_ADD_FETCH(&srv->curr_used_conns, 1);

	/* It's ok not to do that atomically, we don't need an
	 * exact max.
	 */
	if (srv->max_used_conns < curr)
		srv->max_used_conns = curr;

	if (srv->est_need_conns < curr)
		srv->est_need_conns = curr;
}

#endif /* _HAPROXY_SERVER_H */

/*
 * Local variables:
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 */
