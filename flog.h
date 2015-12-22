#ifndef _FLOG_H_
#define _FLOG_H_

#ifdef __cplusplus
extern "C" {
#endif 

#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>

#ifndef NLOG_MAX_PATH
#define NLOG_MAX_PATH 256
#endif

#define LOGFILE_DEFMAXSIZE (100 * 1024 * 1024)

typedef enum LogLevel
{
    L_FATAL = 0,   
    L_ERROR,     
    L_WARN,   
    L_INFO,        
    L_DEBUG,  
    L_TRACE,
    L_LEVEL_MAX
}LogLevel;

typedef struct Flogconf
{
    /// 日志文件名
    char file_name[NLOG_MAX_PATH];

    /// 单个日志文件最大文件大小
    size_t max_size;

    /// 日志级别
    LogLevel max_level;

    int enable_usec;

    int enable_pack_print;//是否打开16进制pack打印功能
}Flogconf;

#define DEFLOGCONF {"logtest",LOGFILE_DEFMAXSIZE,L_LEVEL_MAX,0,1}
//推荐:max_level:L_LEVEL_MAX , enable_pack_print:1 ,enable_usec:0


int InitFLog(Flogconf logconf);
void ExitFlog(); 
// for linux(gcc)
#define CHECK_FORMAT(i, j)  //__attribute__((format(printf, i, j)))
int FLog_log_fatal(const char* fmt, ...) CHECK_FORMAT(2, 3);
int FLog_log_error(const char* fmt, ...) CHECK_FORMAT(2, 3);
int FLog_log_warn(const char* fmt, ...) CHECK_FORMAT(2, 3);
int FLog_log_info(const char* fmt, ...) CHECK_FORMAT(2, 3);
int FLog_log_trace(const char* fmt, ...) CHECK_FORMAT(2, 3);
int FLog_log_debug(const char* fmt, ...) CHECK_FORMAT(2, 3);
#undef CHECK_FORMAT


int FLog_log_hex_prefix(unsigned char * prefix,unsigned char * data, size_t len, LogLevel level);
int FLog_log_hex(unsigned char * data, size_t len, LogLevel level);


#define LOG(level) LOG_##level

#define LOG_0 LOG_FATAL
#define LOG_1 LOG_ERROR
#define LOG_2 LOG_WARN
#define LOG_3 LOG_INFO
#define LOG_4 LOG_DEBUG
#define LOG_5 LOG_TRACE


#define LOG_FATAL FLog_log_fatal
#define LOG_ERROR FLog_log_error
#define LOG_WARN FLog_log_warn
#define LOG_INFO FLog_log_info
#define LOG_TRACE FLog_log_trace
#define LOG_DEBUG FLog_log_debug

#define LOG_HEX(data, len, level) FLog_log_hex((unsigned char *)(data), (len), (level))
#define LOG_HEX_PREFIX(prefix, data, len, level) FLog_log_hex_prefix((unsigned char *)(prefix), (unsigned char *)(data), (len), (level))


#define LOG_INIT(logconf) InitFLog(logconf)
#define LOG_EXIT   ExitFlog()


/*
usage:
    Flogconf logconf = DEFLOGCONF;
    if(0 > LOG_INIT(logconf)){
        printf("log failed\n");
        return -1;
    }
    int a = 100;
    LOG_FATAL("test %d",a);
    ....
    LOG_EXIT;
*/


#ifdef __cplusplus
}
#endif 

#endif
