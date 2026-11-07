#include "mem.h"
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstdio>
#include <cstring>
#include <android/log.h>
#define LOG(...) __android_log_print(ANDROID_LOG_INFO,"hanto",__VA_ARGS__)
static int g_pid=0, g_fd=-1; static bool g_ok=false;
bool Mem::attach(int pid) {
    if (pid<=0) return false; g_pid=pid;
    char path[64]; snprintf(path,sizeof(path),"/proc/%d/mem",pid);
    g_fd=open(path,O_RDWR); if (g_fd<0){LOG("open mem fail");return false;}
    g_ok=true; return true;
}
void Mem::detach(){ if(g_fd>=0)close(g_fd); g_fd=-1; g_ok=false; g_pid=0; }
bool Mem::ok(){ return g_ok; }
size_t Mem::read(uintptr_t addr, void* out, size_t n){
    if(!g_ok)return 0; struct iovec l{out,n}, r{(void*)addr,n};
    ssize_t res=process_vm_readv(g_pid,&l,1,&r,1,0);
    if(res<=0){ if(pread(g_fd,out,n,(off_t)addr)==(ssize_t)n)return n; return 0; }
    return (size_t)res;
}
size_t Mem::write(uintptr_t addr, const void* in, size_t n){
    if(!g_ok)return 0; struct iovec l{(void*)in,n}, r{(void*)addr,n};
    ssize_t res=process_vm_writev(g_pid,&l,1,&r,1,0);
    if(res<=0){ if(pwrite(g_fd,in,n,(off_t)addr)==(ssize_t)n)return n; return 0; }
    return (size_t)res;
}
uintptr_t Mem::base(const char* so){
    char path[64]; snprintf(path,sizeof(path),"/proc/%d/maps",g_pid);
    FILE* f=fopen(path,"r"); if(!f)return 0; char line[512];
    while(fgets(line,sizeof(line),f)){
        if(strstr(line,so)&&strstr(line,"r-xp")){ uintptr_t a=strtoull(line,nullptr,16); fclose(f); return a; }
    }
    fclose(f); return 0;
}