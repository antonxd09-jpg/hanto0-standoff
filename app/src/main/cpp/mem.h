#pragma once
#include <cstdint>
class Mem {
public:
    static bool attach(int pid); static void detach(); static bool ok();
    static uintptr_t base(const char* so_name);
    static size_t read(uintptr_t addr, void* out, size_t n);
    static size_t write(uintptr_t addr, const void* in, size_t n);
    template<typename T> static T rd(uintptr_t a) { T v{}; read(a,&v,sizeof(T)); return v; }
    template<typename T> static bool wr(uintptr_t a, T v) { return write(a,&v,sizeof(T))==sizeof(T); }
};