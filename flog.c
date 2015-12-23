// flog.c
#include "flog.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <assert.h>


#define DATE_START  7
#define TIME_START  (DATE_START + 11)


typedef struct FLog
{
    /// 日志文件名
    char file_name[NLOG_MAX_PATH];

    /// 单个日志文件最大文件大小
    size_t max_size;

    /// 日志级别
    LogLevel max_level;

    /// 日志文件文件描述符
    FILE * file;

    /// 今天开始时刻
    time_t mid_night;

    int enable_usec;

    int enable_pack_print;
    
    int binited;
}FLog;

static FLog g_sFlog;

static int FLog_open();
static void FLog_close();
static int FLog_log(LogLevel level, const char* fmt, ...);
static int FLog_strformatreplace(char * srcstr, char * desstr);
static int FLog_vlog(int level, const char * fmt, va_list ap);

static char level_str_[][64] = {
    "\033[1;31m2008-11-07 09:35:00 FATAL ", 
    "\033[1;33m2008-11-07 09:35:00 ERROR ", 
    "\033[1;35m2008-11-07 09:35:00 WARN  ", 
    "\033[1;32m2008-11-07 09:35:00 INFO  ", 
    "\033[0;00m2008-11-07 09:35:00 DEBUG ",
    "\033[0;00m2008-11-07 09:35:00 TRACE ",
};

static char level_str_usec_[][64] = {
    "\033[1;31m2008-11-07 09:35:00.000000 FATAL ", 
    "\033[1;33m2008-11-07 09:35:00.000000 ERROR ", 
    "\033[1;35m2008-11-07 09:35:00.000000 WARN  ", 
    "\033[1;32m2008-11-07 09:35:00.000000 INFO  ", 
    "\033[0;00m2008-11-07 09:35:00.000000 DEBUG ",
    "\033[0;00m2008-11-07 09:35:00.000000 TRACE ",
};

//Public
int InitFLog(Flogconf logconf)
{
    assert(g_sFlog.binited == 0);
    strncpy(g_sFlog.file_name,logconf.file_name,NLOG_MAX_PATH);
    g_sFlog.max_size = (logconf.max_size > LOGFILE_DEFMAXSIZE)?LOGFILE_DEFMAXSIZE:logconf.max_size;
    g_sFlog.file = NULL;
    g_sFlog.max_level = (logconf.max_level > L_LEVEL_MAX)?L_LEVEL_MAX:logconf.max_level;
    g_sFlog.enable_usec = logconf.enable_usec;
    
    if (0 > FLog_open()) {
        return -1;
    }
    g_sFlog.binited = 1;
    
    return 0;
}

//Public
void ExitFlog()
{
    FLog_close();
}

static int FLog_open()
{
    assert(g_sFlog.binited == 0);
    
    int i = 0;
    char name[NLOG_MAX_PATH];
    size_t len = 0;

    strncpy(name, g_sFlog.file_name, NLOG_MAX_PATH);
    len = strlen(name);

    time_t t;
    time(&t);
    struct tm lt = *localtime(&t);
    strftime(name + len, NLOG_MAX_PATH - len, "-%Y%m%d-%H%M%S.log", &lt);

    g_sFlog.file = fopen(name, "a+");
    if (NULL == g_sFlog.file) {
        return -1;
    }

    strftime(name, 12, "%Y-%m-%d", &lt);
    for (i = 0; i < L_LEVEL_MAX; i++) {
        memcpy(level_str_[i] + DATE_START, name, 10);
    }
    
    for (i = 0; i < L_LEVEL_MAX; i++) {
        memcpy(level_str_usec_[i] + DATE_START, name, 10);
    }

    lt.tm_hour = lt.tm_min = lt.tm_sec = 0;
    g_sFlog.mid_night = mktime(&lt);
    
    return 0;
}

static void FLog_close()
{
    if (0 == g_sFlog.binited) {
        return ;
    }

    fclose(g_sFlog.file);
    g_sFlog.file = NULL;
}

//Public
int FLog_log(LogLevel level, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int ret = FLog_vlog(level, fmt, ap); // not safe
    va_end(ap);
    return ret;
}

//Public
int FLog_log_fatal(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int ret = FLog_vlog(L_FATAL, fmt, ap);
    va_end(ap);
    return ret;
}

//Public
int FLog_log_error(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int ret = FLog_vlog(L_ERROR, fmt, ap);
    va_end(ap);
    return ret;
}

//Public
int FLog_log_warn(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int ret = FLog_vlog(L_WARN, fmt, ap);
    va_end(ap);
    return ret;
}

static int FLog_strformatreplace(char * srcstr, char * desstr)
{
    if (NULL == srcstr || NULL == desstr) {
        return -1;
    }

    if (strlen(srcstr) >= strlen(desstr)) {
        return -1;
    }
    unsigned int j = 0;
    desstr[j++] = srcstr[0];
    unsigned int i = 0;
    for (i = 1; i<strlen(srcstr); i++) {
        if (srcstr[i-1] == '%' && (srcstr[i] == 's' || srcstr[i] == 'S')) {
            if (j+5 >= strlen(desstr)) {
                return -1;
            }
            desstr[j++] = '.';
            desstr[j++] = '5';
            desstr[j++] = '1';
            desstr[j++] = '2';
            desstr[j++] = 's';
        }
        else {
            if (j >= strlen(desstr)) {
                return -1;
            }
            desstr[j++] = srcstr[i];
        }
    }
    if (j >= strlen(desstr)) {
        return -1;
    }
    
    desstr[j++] = '\0';
    
    return 0;
}

//Public
int FLog_log_info(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int ret = FLog_vlog(L_INFO, fmt, ap);
    va_end(ap);
    return ret;
}

//Public
int FLog_log_trace(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int ret = FLog_vlog(L_TRACE, fmt, ap);
    va_end(ap);
    return ret;
}

//Public
int FLog_log_debug(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int ret = FLog_vlog(L_DEBUG, fmt, ap);
    va_end(ap);
    return ret;
}

static int FLog_vlog(int level, const char * fmt, va_list ap)
{
    if (g_sFlog.binited == 0 || level > g_sFlog.max_level) {
        return -1;
    }

    struct tm tm_now;
    struct timeval tv;
    struct timezone tz;
    gettimeofday(&tv, &tz);
    time_t now = tv.tv_sec;

    int t_diff = (int)(now - g_sFlog.mid_night);
    if (t_diff > 24 * 60 * 60) {
        FLog_close();
        FLog_open();
        t_diff -= 24 * 60 * 60;
    }
    
    localtime_r(&now, &tm_now);
    if (g_sFlog.enable_usec) {
        sprintf(((char*)level_str_usec_[level]+TIME_START), "%02d:%02d:%02d.%06ld",                    
            tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec, tv.tv_usec);
        level_str_usec_[level][strlen(level_str_usec_[level])] = ' ';
        fputs(level_str_usec_[level], g_sFlog.file);
    }
    else {
        sprintf(((char*)level_str_[level]+TIME_START), "%02d:%02d:%02d",                    
            tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec);
        level_str_[level][strlen(level_str_[level])] = ' ';
        fputs(level_str_[level], g_sFlog.file);
    }

    char strformat[128] = "";
    if (0 == FLog_strformatreplace((char *) fmt, strformat)) {
        vfprintf(g_sFlog.file, strformat, ap);
    }
    else {
        vfprintf(g_sFlog.file, fmt, ap);
    }
    
        
    // reset color
    if (fmt[strlen(fmt) - 1] != '\n') {
        fputc('\n', g_sFlog.file);
    }
    
    if ((size_t)ftell(g_sFlog.file) > g_sFlog.max_size) {
        FLog_close();
        FLog_open();
    }

    return 0;
}


static const char chex[] = "0123456789ABCDEF";

//Public
int FLog_log_hex_prefix(unsigned char * prefix,unsigned char * data, size_t len, LogLevel level)
{
    FLog_log(level, "%s", prefix);
    return FLog_log_hex(data, len, level);
}

//Public
int FLog_log_hex(unsigned char * data, size_t len, LogLevel level)
{
    size_t i, j, k, l;

    if (level > g_sFlog.max_level ||NULL == data|| NULL == g_sFlog.file) {
        return -1;
    }
    
    //DON'T disable hex_print when level is  l_info, l_warn....
    if (!g_sFlog.enable_pack_print && level > L_INFO) {
        return -1;
    }

    char msg_str[128] = {0};

    msg_str[0] = '[';
    msg_str[5] = '0';
    msg_str[6] = ']';
    msg_str[59] = ' ';
    msg_str[60] = '|';
    msg_str[77] = '|';
    msg_str[78] = 0;
    k = 6;
    for (j = 0; j < 16; j++) {
        if ((j & 0x03) == 0) {
            msg_str[++k] = ' ';
        }
        k += 3;
        msg_str[k] = ' ';
    }
    for (i = 0; i < len / 16; i++) {
        msg_str[1] = chex[i >> 12];
        msg_str[2] = chex[(i >> 8)&0x0F];
        msg_str[3] = chex[(i >>4)&0x0F];
        msg_str[4] = chex[i &0x0F];
        k = 7;
        l = i * 16;
        memcpy(msg_str + 61, data + l, 16);
        for (j = 0; j < 16; j++) {
            if ((j & 0x03) == 0) {
                k++;
            }
            msg_str[k++] = chex[data[l] >> 4];
            msg_str[k++] = chex[data[l++] & 0x0F];
            k++;
            if (!isgraph(msg_str[61 + j])) {
                msg_str[61 + j]= '.';
            }
        }
        msg_str[127] = 0;
        fprintf(g_sFlog.file, "# %s\n", msg_str);
    }
    
    msg_str[1] = chex[i >> 12];
    msg_str[2] = chex[(i >> 8)&0x0F];
    msg_str[3] = chex[(i >>4)&0x0F];
    msg_str[4] = chex[i &0x0F];
    
    k = 7;
    l = i * 16;
    memcpy(msg_str + 61, data + l, len % 16);
    for (j = 0; j < len % 16; j++) {
        if ((j & 0x03) == 0) {
            k++;
        }
        msg_str[k++] = chex[data[l] >> 4];
        msg_str[k++] = chex[data[l++] & 0x0F];
        k++;
        if (!isgraph(msg_str[61 + j])) {
            msg_str[61 + j]= '.';
        }
    }
    for (; j < 16; j++) {
        if ((j & 0x03) == 0) {
            k++;
        }
        msg_str[k++] = ' ';
        msg_str[k++] = ' ';
        k++;
        msg_str[61 + j]= ' ';
    }
    msg_str[127] = 0;
    fprintf(g_sFlog.file, "# %s\n", msg_str);

    return 0;
}
