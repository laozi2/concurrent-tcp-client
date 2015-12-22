#include "common.h"
#include "ngx_event_timer.h"
#include "flog.h"

int epfd = -1;
int stop = 0;
connection_t* p_connection_pool = NULL;

static void make_request(connection_t* c);
static int add_read_event(connection_t* c);
static int add_write_event(connection_t* c);
static int start_connect(connection_t* c);
static void close_connection(connection_t* c);
static void destroy_connection_pool();
static void handle_after_request(connection_t* c);

static int 
add_read_event(connection_t* c)
{
    int op;
    
    if (c->ready_read) {
        return 0;
    }
    
    struct epoll_event ev;
    ev.data.ptr = c;
    ev.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
    op = (c->active) ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;

    if (epoll_ctl(epfd, op, c->fd, &ev) == -1) {
        return -1;
    }
    
    c->active = 1;
    c->ready_read = 1;
    c->ready_write = 0;
    return 0;
}

static int 
add_write_event(connection_t* c)
{
    int op;
    
    if (c->ready_write) {
        return 0;
    }
    
    struct epoll_event ev;
    ev.data.ptr = c;
    ev.events = EPOLLOUT | EPOLLET;
    op = (c->active) ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;

    if (epoll_ctl(epfd, op, c->fd, &ev) == -1) {
        return -1;
    }
    
    c->active = 1;
    c->ready_write = 1;
    c->ready_read = 0;
    return 0;
}

static int 
start_connect(connection_t* c)
{
    struct sockaddr* server_addr = (struct sockaddr*)&(p_config->server_addr);
    int socklen = p_config->socklen;
    int reuseaddr = 1;
    
    int fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        return -1;
    }
    
    ngx_nonblocking (fd);
    
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
               (const void *) &reuseaddr, sizeof(int))
        == -1) {
        goto failed;
    }
    
    if (-1 == connect(fd, server_addr, socklen)) {
        if (EINPROGRESS != errno) {
            perror("connect()");
            goto failed;
        }
    }
    
    //connect ok or EINPROGRESS
    
    c->fd = fd;
    
    if (add_write_event(c) == -1) {
        goto failed;
    }
    
    c->state = ST_CONNECTING;
    return 0;
    
failed:
    LOG_WARN("[%u][%d]start_connect failed",c->pos,fd);
    close(fd);
    return -1;
}

int start_all_connection()
{
    assert(p_connection_pool && p_config);
    
    int i, ret;
    
    int ok = 0;
    for (i = 0; i < p_config->connection_pool_n; i++) {
        ret = start_connect(p_connection_pool + i);
        if (ret == -1) {
        
        }
        else {
            ok = 1;
        }
    }
    
    return (1 == ok) ? 0 : -1;
}

static void
close_connection(connection_t* c)
{
    LOG_DEBUG("[%u][%d]close_connection",c->pos,c->fd);
    
    if (c->fd != -1) {
        close(c->fd);
        c->fd = -1;
    }
    
    c->active = 0;
    c->ready_read = 0;
    c->ready_write = 0;
    c->state = ST_CLOSED;
}

//need call after p_config
int create_connection_pool()
{
    assert(p_config && !p_connection_pool);
    
    unsigned int i;
    unsigned int connection_pool_n;

    connection_pool_n = p_config->connection_pool_n;

    p_connection_pool = calloc(sizeof(connection_t), connection_pool_n);
    assert(p_connection_pool);
    
    for (i = 0; i < connection_pool_n; i++) {
        p_connection_pool[i].fd = -1;
        p_connection_pool[i].pos = i;
        p_connection_pool[i].conf = &p_config->con_config[i];
        //p_connection_pool[i].send_buf = 
        //p_connection_pool[i].read_buf = 
        //p_connection_pool[i].requests_fd = 0;
        //p_connection_pool[i].active = 0;
        //p_connection_pool[i].ready_read = 0;
        //p_connection_pool[i].ready_write = 0;
        p_connection_pool[i].state = ST_CLOSED;
    }
    
    //create recv buf
    p_connection_pool[0].read_buf.start = (char*)malloc(READ_BUF_MAX_LEN * connection_pool_n);
    for (i = 1; i < connection_pool_n; i++) {
        p_connection_pool[i].read_buf.start = p_connection_pool[0].read_buf.start + i * 1024;
    }
    
    return 0;
}

//need call befor p_config
static void 
destroy_connection_pool()
{
    int i;
    
    if (NULL == p_connection_pool) {
        return;
    }
    
    for (i = 0; i < p_config->connection_pool_n; i++) {
        if (p_connection_pool[i].fd != -1) {
            close(p_connection_pool[i].fd);
        }
    }
    
    free(p_connection_pool[0].read_buf.start);
    free(p_connection_pool);
    p_connection_pool = NULL;
}

void clean_all_resource()
{
    destroy_connection_pool();
    clear_test_config();
    
    if (epfd != -1) {
        close(epfd);
        epfd = -1;
        //log
    }
    
    LOG_INFO("clean_all_resource done");
}

//no futher handler
static void
handle_after_request(connection_t* c)
{
    c->state = ST_IDLE;

    if (c->conf->sleep_ms > 0) {
        ngx_add_timer(c, c->conf->sleep_ms);
        return;
    }
    //add_write_event(c);
    
    handle_send_event(c);
}

static void 
make_request(connection_t* c)
{
    c->send_buf.start = p_config->test_data[0].start;
    c->send_buf.last = p_config->test_data[0].end;
    c->send_buf.pos = c->send_buf.start;
}

void 
handle_send_event(connection_t* c)
{
    int n;
    
    if (c->conf->requests-- == 0) {
        return;
    }
    
    if (c->state == ST_CONNECTING || c->state == ST_IDLE) {
        c->state = ST_SENDING;
    
        make_request(c);
        LOG_INFO("[%u][%d]-----------start new request--------------",c->pos,c->fd);
    }
    
    if (c->state != ST_SENDING) {
        return;
    }
    
    int send_len = c->send_buf.last - c->send_buf.pos;
    n = send(c->fd, c->send_buf.pos, send_len, 0);
    LOG_DEBUG("[%u][%d]send %d bytes",c->pos,c->fd,n);
    if (n > 0) {
        c->send_buf.pos += n;
    
        if (n < send_len) {
            c->ready_write = 0;
            add_write_event(c);
            return;
        }
    
        //start to recv response
        c->state = ST_SENT;
        add_read_event(c);
        c->state = ST_RECVING;
        c->read_buf.read_len = -4;
        return;
        }
    
    if (n == 0) {
        c->ready_write = 0;
        add_write_event(c);
    
        return;
    }
    
    //n < 0
    if (errno == EAGAIN || errno == EINTR) {
        c->ready_write = 0;
        add_write_event(c);
    
        return;
    }
    
    //error
    close_connection(c);
    
    if (c->conf->retry) {
        LOG_INFO("[%u][%d]send error, retry to connect",c->pos,c->fd);
        if (start_connect(c) == -1) {
        
        }
    
        return;
    }
}


void 
handle_read_event(connection_t* c)
{
    int read_len = c->read_buf.read_len;
    size_t size = 0;
    int n;
    char* pos = NULL;
    
    if (c->state != ST_RECVING) {
        return;
    }
    
    if (read_len < 0) {
        size = -read_len;
        pos = &(c->read_buf.head[4 - size]);
    }
    else {
        size = c->read_buf.end - c->read_buf.start - read_len;
        pos = c->read_buf.start + read_len;
    }
    
    //read
    do {
        n = recv(c->fd, pos, size, 0);
        LOG_DEBUG("[%u][%d]recv %d bytes",c->pos,c->fd,n);
        if (n > 0) {
    
            if ((size_t) n < size) {
                if (read_len < 0) {
                    read_len += n;
                    pos = &(c->read_buf.head[4 + read_len]);
                }
                else {
                    read_len -= n;
                    pos = c->read_buf.start + read_len;
                }
                c->read_buf.read_len = read_len;
        
                size -= n;
                continue;//until AGAIN
            }
        
            //read head ok
            if (read_len < 0) {
                read_len = ntohl(*(uint32_t*)(c->read_buf.head));
                if (read_len <= 4 || read_len >= READ_BUF_MAX_LEN) {
                    LOG_WARN("[%u][%d]protocol error, head %d",c->pos,c->fd,read_len);
                    goto failed;
                }
        
                c->read_buf.read_len = read_len;
                size = read_len - 4;
                pos = c->read_buf.start;
        
                continue;
            }
        
            //read all done
            c->state = ST_RECVED;
            handle_after_request(c);
            return;
       }
    
        //closed
        if (n == 0) {
            goto failed;
        }
    
        //AGAIN
        if (errno == EAGAIN || errno == EINTR) {
            c->ready_read = 0;
            add_read_event(c);
            return;
        }
    
        //error
        goto failed;
    
    } while(1);

failed:
    //error
    close_connection(c);
    
    if (c->conf->retry) {
        LOG_WARN("[%u][%d]recv error, retry to connect",c->pos,c->fd);
        if (start_connect(c) == -1) {
        
        }
    
        return;
    }
}

void 
handle_close_event(connection_t* c)
{
    close_connection(c);
    
    if (c->conf->retry) {
        LOG_WARN("[%u][%d]handle_close_event, retry to connect",c->pos,c->fd);
        if (start_connect(c) == -1) {
        
        }
    
        return;
    }
}

void 
handle_timer_event(connection_t* c)
{
    int state = c->state;
    
    switch (state) {
    case ST_IDLE:
        LOG_DEBUG("handle_timer_event %d",state);
        handle_send_event(c);
        break;
    default :
        LOG_WARN("handle_timer_event %d",state);
        break;
    }
}


