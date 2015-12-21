
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_EVENT_TIMER_H_INCLUDED_
#define _NGX_EVENT_TIMER_H_INCLUDED_

#include "common.h"

typedef ngx_rbtree_key_t      ngx_msec_t;
typedef ngx_rbtree_key_int_t  ngx_msec_int_t;

#define NGX_TIMER_INFINITE  (ngx_msec_t) -1

#define NGX_TIMER_LAZY_DELAY  300

#define ngx_add_timer        ngx_event_add_timer
#define ngx_del_timer        ngx_event_del_timer


ngx_int_t ngx_event_timer_init();
ngx_msec_t ngx_event_find_timer(void);
void ngx_event_expire_timers(void);

void ngx_time_update(void);


//#if (NGX_THREADS)
//extern ngx_mutex_t  *ngx_event_timer_mutex;
//#endif


extern ngx_thread_volatile ngx_rbtree_t  ngx_event_timer_rbtree;
extern volatile ngx_msec_t  ngx_current_msec;


static ngx_inline void
ngx_event_del_timer(connection_t *c)
{
    //ngx_log_debug2(NGX_LOG_DEBUG_EVENT, ev->log, 0,
    //               "event timer del: %d: %M",
    //                ngx_event_ident(ev->data), ev->timer.key);

    //ngx_mutex_lock(ngx_event_timer_mutex);

    ngx_rbtree_delete(&ngx_event_timer_rbtree, &c->timer);

    //ngx_mutex_unlock(ngx_event_timer_mutex);

//#if (NGX_DEBUG)
//    ev->timer.left = NULL;
//    ev->timer.right = NULL;
//    ev->timer.parent = NULL;
//#endif

    c->timer_set = 0;
}


static ngx_inline void
ngx_event_add_timer(connection_t *c, ngx_msec_t timer)
{
    ngx_msec_t      key;
    ngx_msec_int_t  diff;

    key = ngx_current_msec + timer;

    if (c->timer_set) {

        /*
         * Use a previous timer value if difference between it and a new
         * value is less than NGX_TIMER_LAZY_DELAY milliseconds: this allows
         * to minimize the rbtree operations for fast connections.
         */

        diff = (ngx_msec_int_t) (key - c->timer.key);

        if (ngx_abs(diff) < NGX_TIMER_LAZY_DELAY) {
            //ngx_log_debug3(NGX_LOG_DEBUG_EVENT, ev->log, 0,
            //               "event timer: %d, old: %M, new: %M",
            //                ngx_event_ident(ev->data), ev->timer.key, key);
            return;
        }

        ngx_del_timer(c);
    }

    c->timer.key = key;

    //ngx_log_debug3(NGX_LOG_DEBUG_EVENT, ev->log, 0,
    //               "event timer add: %d: %M:%M",
    //                ngx_event_ident(ev->data), timer, ev->timer.key);

    //ngx_mutex_lock(ngx_event_timer_mutex);

    ngx_rbtree_insert(&ngx_event_timer_rbtree, &c->timer);

    //ngx_mutex_unlock(ngx_event_timer_mutex);

    c->timer_set = 1;
}


#endif /* _NGX_EVENT_TIMER_H_INCLUDED_ */
