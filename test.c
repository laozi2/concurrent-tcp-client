#include "common.h"
#include "flog.h"
#include "ngx_event_timer.h"


static void signal_handler(int x);

int main(int argc, char* argv[])
{
    //atexit(stop_thread);
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    //signal(SIGABRT,signal_handler);
    
    struct epoll_event event_list[EPOLLWAITNUM];
    connection_t* c;
    uint32_t      revents;
    int i;
    ngx_msec_t  wait_time,delta;
    
    //Flogconf logconf = DEFLOGCONF;
    Flogconf logconf = {"/tmp/logtest",LOGFILE_DEFMAXSIZE,L_LEVEL_MAX,0,1};
    if (0 > LOG_INIT(logconf)) {
        printf("LOG_INIT() failed\n");
        return -1;
    }

    srand((unsigned)time(NULL));
    
    //config
    if (0 != make_test_config()) {
        LOG_ERROR("make_test_config() failed");
        goto done;
    }
    
    if (0 != ngx_event_timer_init()) {
        LOG_ERROR("ngx_event_timer_init() failed");
        goto done;
    }
    
    if (0 != create_connection_pool()) {
        LOG_ERROR("create_connection_pool() failed");
        goto done;
    }
    
    epfd = epoll_create(EPOLLMAXEVENTS);    //生成用于处理accept的epoll专用的文件描述符, 指定生成描述符的最大范围为256 
    if (epfd == -1) {
        LOG_ERROR("epoll_create()");
        goto done;
    }
    
    if (0 != start_all_connection()) {
        LOG_ERROR("start_all_connection() failed");
        goto done;
    }
    
    while (1) {
        //if (stop) {printf("stop"); break;}
        
        wait_time = ngx_event_find_timer();
        
        LOG_DEBUG("wait_time %d",wait_time);
        
        delta = ngx_current_msec;
        
        int nfds = epoll_wait(epfd, event_list, EPOLLWAITNUM, wait_time);
        if (-1 == nfds) {
            LOG_ERROR("epoll_wait");
            goto done;
        }
        
        ngx_time_update();
        
        delta = ngx_current_msec - delta;
        LOG_DEBUG("delta %d",delta);
        
        if (nfds == 0) {
            if (wait_time == NGX_TIMER_INFINITE) {
                //return NGX_OK;
                
                LOG_ERROR("epoll_wait() returned no events without timeout");
                goto done;
            }
            
            LOG_DEBUG("nfds 0");
            if (wait_time == 0 || delta) {
                ngx_event_expire_timers();
            }
            continue;
        }
    
        for (i = 0; i < nfds; ++i) {
            
            revents = event_list[i].events;
            c = event_list[i].data.ptr;
            
            if (revents & EPOLLRDHUP) {
                LOG_DEBUG("[%ul][%d]EPOLLRDHUP handle_close_event",c->pos,c->fd);
                handle_close_event(c);
                continue;
            }
            
            if (revents & EPOLLIN) {
                LOG_DEBUG("[%u][%d]EPOLLIN handle_read_event",c->pos,c->fd);
                handle_read_event(c);
                continue;
            }
            
            if (revents & EPOLLOUT) {
                LOG_DEBUG("[%u][%d]EPOLLOUT handle_send_event",c->pos,c->fd);
                handle_send_event(c);
                continue;
            } 
            
            //error
            LOG_WARN("[%u][%d]error handle_close_event",c->pos,c->fd);
            handle_close_event(c);
            //continue;
        }                        //for
        
        if (delta) {
            ngx_event_expire_timers();
        }
    }                            //while

done:
    LOG_DEBUG("program done");
    clean_all_resource();

    LOG_EXIT;

    return 0;
}

static void 
signal_handler(int x)
{
    clean_all_resource();
    LOG_EXIT;
    
    exit(1);
}