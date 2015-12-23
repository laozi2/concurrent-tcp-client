#include "common.h"
#include "flog.h"

static config_t config;
config_t* p_config = NULL;

static void check_config();
static void make_tmp_test_data();
static int load_test_data();

//config data
static const char* server_name = "server1";  //optional, default $ip:$port
static const char* ip = "127.0.0.1";
static unsigned short port = 6666;
static unsigned int connection_pool_n = 10; //optional, default 1
static unsigned int requests = 10;  //optional, default 1
static unsigned int sleep_min = 10; //optional, default 0
static unsigned int sleep_max = 1000;  //optional, default 0, >=sleep_min
static unsigned int read_max_buf_len = 1024; //optional, default 1024
static unsigned int retry = 0x01; //optional, default 0
static unsigned int is_random = 0;//0x01; //optional, default 0
static const char* test_data_file = "test_data/test_data.bin";

void
check_config()
{
	//if(name)
}

/*
* if ok, p_all_config will be set
* if failed, return -1; and all resources are cleared. p_config will be NULL
*/
int 
make_test_config()
{
	assert(!p_config); //only create config once
	
	int i;	
	
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
	p_config->read_max_buf_len = read_max_buf_len;
	p_config->test_data_file = test_data_file;
	
	//make_tmp_test_data();
	if (load_test_data() < 0) {
		goto failed;
	}
	
	p_config->def_con_config.retry = retry;
	p_config->def_con_config.sleep_min = sleep_min;
	p_config->def_con_config.sleep_max = sleep_max;
	p_config->def_con_config.random = is_random;
	p_config->def_con_config.requests = requests;
	
	p_config->con_config = calloc(sizeof(connection_config_t), p_config->connection_pool_n);
	for (i = 0; i < p_config->connection_pool_n; i++) {
		p_config->con_config[i].retry = p_config->def_con_config.retry;
		p_config->con_config[i].sleep_min = p_config->def_con_config.sleep_min;
		p_config->con_config[i].sleep_max = p_config->def_con_config.sleep_max;
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
make_tmp_test_data()
{	
	static const char* send_str = "/user/login2?user=Zmojianliang@163%2Ecom&password=d2dc0e04e5ee1740&client=Android-SM-A7000&client_id=4Q9DQ9Nge9WQ6DE1CFAf6bUd&client_app=CamScanner_AD_1_LITE@3%2E9%2E1%2E20151020_Market_360&type=email";
	int send_len = strlen(send_str) + 4;
	//if (send_len > READ_BUF_MAX_LEN) {
		assert(send_len <= READ_BUF_MAX_LEN);
	//}
	
	p_config->test_data_n = 1;
	p_config->test_data = calloc(sizeof(test_data_t), p_config->test_data_n);
	
	uint32_t n_net = htonl(send_len);
  
	p_config->test_data[0].start = malloc(send_len);
	memcpy(p_config->test_data[0].start, &n_net, 4);
	memcpy(p_config->test_data[0].start + 4, send_str, send_len - 4);
	p_config->test_data[0].end = p_config->test_data[0].start + send_len;
}

//will not free test_data when failed
static int 
load_test_data()
{
	FILE *fp;
	int i,n,totoal_len;
	uint32_t n_net;
	char buf[4];
	
	fp = fopen(p_config->test_data_file,"rb");
	if (NULL == fp) {
		LOG_WARN("load_test_data failed");
		return -1;
	}
	
	n = fread(buf, 1, 4, fp);
	if (n != 4) {
		LOG_WARN("fread failed %d",n);
		return -1;
	}
	
	totoal_len = ntohl(*(uint32_t*)buf);
	if (totoal_len <= 0 || totoal_len > 10000) { //array 1~1000
		LOG_WARN("totoal_len beyond 1~1000",totoal_len);
		fclose(fp);
		return -1;
	}
	
	LOG_DEBUG("array test data %d",totoal_len);
	
	p_config->test_data_n = totoal_len;
	p_config->test_data = calloc(sizeof(test_data_t), p_config->test_data_n);
	
	for (i = 0; i < p_config->test_data_n; i++) {
		n = fread(buf, 1, 4, fp);
		if (n != 4) {
			LOG_WARN("fread failed %d != 4",n);
			fclose(fp);
			return -1;
		}
		
		totoal_len = ntohl(*(uint32_t*)buf);
		if (totoal_len <= 4 || totoal_len > 102400) {  //5~100k
			LOG_WARN("totoal_len %d beyond 5byte~100k",totoal_len);
			fclose(fp);
			return -1;
		}
		
		n_net = htonl(totoal_len);
		p_config->test_data[i].start = malloc(totoal_len);
		memcpy(p_config->test_data[i].start, &n_net, 4);
		p_config->test_data[i].end = p_config->test_data[i].start + totoal_len;
		
		n = fread(p_config->test_data[i].start + 4, 1, totoal_len - 4, fp);
		if (n != totoal_len - 4) {
			LOG_WARN("fread failed %d != %d",n, totoal_len -4);
			fclose(fp);
			return -1;
		}
	}
	
	fclose(fp);
	return 0;
}