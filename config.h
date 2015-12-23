#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <netinet/in.h>

typedef struct config_s config_t;
typedef struct connection_config_s connection_config_t;
typedef struct test_data_s test_data_t;

struct test_data_s {
    u_char* start;
    u_char* end;
};

struct connection_config_s {
    unsigned int requests;
	unsigned int sleep_ms;
    unsigned retry:1;
    unsigned random:1;
};

struct config_s {
    char server_name[32];
    char ip[16];
    unsigned short port;
    struct sockaddr_in server_addr;
    socklen_t       socklen;
    unsigned int connection_pool_n;
    test_data_t* test_data;
    unsigned int test_data_n;
    connection_config_t def_con_config;
    connection_config_t *con_config;
};

#endif //_CONFIG_H_