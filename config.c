#include "common.h"
#include "flog.h"

static all_config_t all_config;
all_config_t* p_all_config = NULL;

static void make_tmp_test_data(test_data_t* test_data);


/*
* if ok, p_all_config will be set
* if failed, return -1; and all resources are cleared. p_all_config will be NULL
*/
int 
make_test_config()
{
	assert(!p_all_config); //only create config once
	
	server_config_t *psc;
	int i,j;
	unsigned int connections = 0;
	
	int n_server_config = 1;
	const char* ip = "127.0.0.1";
	unsigned short port = 6666;
	
	all_config.config_file = NULL;
	all_config.server_config_n = n_server_config;
	
	psc = calloc(sizeof(server_config_t), n_server_config);
	all_config.psc = psc;
	p_all_config = &all_config;
	
	for (i = 0; i < n_server_config; i++) {
		memset(&psc[i].server_name,0,sizeof(psc[i].server_name));
		memcpy(&psc[i].ip,ip,sizeof(psc[i].ip));
		psc[i].port = port;
		
		psc[i].socklen = sizeof(psc[i].server_addr);
		memset(&psc[i].server_addr,0,psc[i].socklen);
		psc[i].server_addr.sin_family = AF_INET;
		psc[i].server_addr.sin_addr.s_addr = inet_addr(ip);
		psc[i].server_addr.sin_port = htons(port);
		
		psc[i].test_data_n = 1;
		psc[i].test_data = calloc(sizeof(test_data_t), psc[i].test_data_n);
		make_tmp_test_data(psc[i].test_data);
		
		psc[i].connection_pool_n = 10; //here
		connections += psc[i].connection_pool_n;
		//if (connections > all_config.connection_pool_n) {
			//wrong
		//	goto failed;
		//}
		
		psc[i].con_config = calloc(sizeof(connection_config_t), psc[i].connection_pool_n);
		for (j = 0; j < psc[i].connection_pool_n; j++) {
			psc[i].con_config[j].retry = 1;
			psc[i].con_config[j].random = 0;
			psc[i].con_config[j].requests = 10; //here
		}
	}
	
	all_config.connection_pool_n = connections;
	
	return 0;
	
failed:
	clear_test_config();

	return -1;
}

void 
clear_test_config()
{
	server_config_t *psc;
	int i,j;
	
	if (NULL == p_all_config) {
		return;
	}
	
	psc = all_config.psc;
	
	if (psc) {
		for (i = 0; i < all_config.server_config_n; i++) {
			if (psc[i].con_config) {
				free(psc[i].con_config);
			}
			if (psc[i].test_data) {
				for (j = 0; j < psc[i].test_data_n; j++) {
					if (psc[i].test_data[j].start) {
						free(psc[i].test_data[j].start);
					}	
				}
				free(psc[i].test_data);
			}
		}
		
		free(psc);
	}
	
	//clear config_file
	//clear test_data_files
	
	p_all_config = NULL;
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