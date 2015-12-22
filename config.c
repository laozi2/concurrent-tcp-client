#include "common.h"
#include "flog.h"

static config_t config;
config_t* p_config = NULL;

static void make_tmp_test_data(test_data_t* test_data);


/*
* if ok, p_all_config will be set
* if failed, return -1; and all resources are cleared. p_all_config will be NULL
*/
int 
make_test_config()
{
	assert(!p_config); //only create config once
	
	config_t *psc;
	int i;
	unsigned int connections = 0;
	
	int n_server_config = 1;
	const char* ip = "127.0.0.1";
	unsigned short port = 6666;
	const char* server_name = "test_server";
	unsigned int connection_pool_n = 10;
	
	p_config = &config;
	strncpy(p_config->server_name, server_name, sizeof(p_config->server_name) / sizeof(char));
	strncpy(p_config->ip, ip, sizeof(p_config->ip) / sizeof(char));
	p_config->port = port;
	p_config->socklen = sizeof(struct sockaddr_in);
	
	memset(&p_config->server_addr, 0, p_config->socklen);
	p_config->server_addr.sin_family = AF_INET;
	p_config->server_addr.sin_addr.s_addr = inet_addr(ip);
	p_config->server_addr.sin_port = htons(port);
	p_config->connection_pool_n = connection_pool_n;
	
	p_config->test_data_n = 1;
	p_config->test_data = calloc(sizeof(test_data_t), p_config->test_data_n);
	make_tmp_test_data(p_config->test_data);
	
	p_config->def_con_config.retry = 1;
	p_config->def_con_config.sleep_ms = 100;
	p_config->def_con_config.random = 0;
	p_config->def_con_config.requests = 10;
	
	p_config->con_config = calloc(sizeof(connection_config_t), p_config->connection_pool_n);
	for (i = 0; i < p_config->connection_pool_n; i++) {
		p_config->con_config[i].retry = p_config->def_con_config.retry;
		p_config->con_config[i].sleep_ms = p_config->def_con_config.sleep_ms;
		p_config->con_config[i].random = p_config->def_con_config.random;
		p_config->con_config[i].requests = p_config->def_con_config.requests; //here
	}

	return 0;
	
failed:
	clear_test_config();

	return -1;
}

void 
clear_test_config()
{
	int i;
	
	if (NULL == p_config) {
		return;
	}
	
	if (p_config->con_config) {
		free(p_config->con_config);
	}
	
	if (p_config->test_data) {
		for (i = 0; i < p_config->test_data_n; i++) {
			if (p_config->test_data[i].start) {
				free(p_config->test_data[i].start);
			}
		}
		
		free(p_config->test_data);
	}
	
	p_config = NULL;
}


static void 
make_tmp_test_data(test_data_t* test_data)
{	
	static const char* send_str = "/user/login2?user=Zmojianliang@163%2Ecom&password=d2dc0e04e5ee1740&client=Android-SM-A7000&client_id=4Q9DQ9Nge9WQ6DE1CFAf6bUd&client_app=CamScanner_AD_1_LITE@3%2E9%2E1%2E20151020_Market_360&type=email";
	int send_len = strlen(send_str) + 4;
	//if (send_len > READ_BUF_MAX_LEN) {
		assert(send_len <= READ_BUF_MAX_LEN);
	//}
	
	
	uint32_t n_net = htonl(send_len);
  
	test_data[0].start = malloc(send_len);
	memcpy(test_data[0].start, &n_net, 4);
	memcpy(test_data[0].start + 4, send_str, send_len - 4);
	test_data[0].end = test_data[0].start + send_len;
}