#include "SEGGER_RTT.h"
#include "stdio.h"

#define RTT_LOG_BUF_SIZE 256
static char rrt_log_buf[RTT_LOG_BUF_SIZE] = {0};

int rtt_log(const char *fmt,...)
{
    int n;
    va_list args;
    
    va_start(args,fmt);
    n = vsnprintf(rrt_log_buf,sizeof(rrt_log_buf),fmt,args);
    if(n > 256)
    {
        SEGGER_RTT_Write(0,rrt_log_buf,256);
    }
    else
    {
        SEGGER_RTT_Write(0,rrt_log_buf,n);
    }
    va_end(args);
    return n;
}


int APP_PRINTF(const char *fmt,...)
{
    int n;
    va_list args;
    
    va_start(args,fmt);
    n = vsnprintf(rrt_log_buf,sizeof(rrt_log_buf),fmt,args);
    if(n > 256)
    {
        SEGGER_RTT_Write(0,rrt_log_buf,256);
    }
    else
    {
        SEGGER_RTT_Write(0,rrt_log_buf,n);
    }
    va_end(args);
    return n;
}
