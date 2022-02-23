#pragma GCC push_options
#pragma GCC optimize ("O0")

#include <iostream>
#include <x86intrin.h>
#include <vector>
#include <numeric>

#define ASM asm __volatile__

#define PAGE_SIZE               (4096)
#define TLB_SET_SIZE            (4096)                

struct  __attribute__ ((aligned (PAGE_SIZE))) Page { char x [PAGE_SIZE] = {0}; };

struct  Page*       TARGET_PAGE;
std::vector<Page*>  TLB_SET;
std::vector<Page*>  CACHE_SET;

/* ----------------------------------------------- */
static inline void flush(void *p)
{
    ASM("clflush 0(%0)\n":: "c"(p): "rax");
}

/* ----------------------------------------------- */
static inline uint64_t measure_latency(const void *addr) 
{
    volatile unsigned long time;
    ASM (
        "  mfence             \n"
        "  lfence             \n"
        "  rdtsc              \n"
        "  lfence             \n"
        "  movl %%eax, %%esi  \n"
        "  movl (%1), %%eax   \n"
        "  lfence             \n"
        "  rdtsc              \n"
        "  subl %%esi, %%eax  \n"
        : "=a" (time)
        : "c" (addr)
        :  "%esi", "%edx");
    return time;
}

/* ----------------------------------------------- */
inline void access_memory(const void *addr) 
{
    ASM("movq (%0), %%rax;\n mfence \n" :: "c" (addr): "rax");
}

/* ----------------------------------------------- */
long __attribute__((optimize("O0"))) tlb_miss_latency()
{
    // Fill tlb with random pages
    flush(&TARGET_PAGE->x[0]);
    
    size_t i = 0;
    for (i = 0; i < TLB_SET.size(); i++)
    {
        access_memory(&TLB_SET[i]->x[128]);   
    }

    // This will create TLB & Cache miss
    long cache_tlb_miss = measure_latency(&TARGET_PAGE->x[0]);

    flush(&TARGET_PAGE->x[0]);

    // This will not create cache miss
    long cache_miss = measure_latency(&TARGET_PAGE->x[0]);
    return(cache_tlb_miss-cache_miss); 
}


int main()
{
    TARGET_PAGE = new Page();
    
    size_t i = 0;
    for (i = 0; i < TLB_SET_SIZE; i++)
    {
        TLB_SET.push_back(new Page());
    }

    std::vector<int> times;
    for (i = 0; i < 10000; i++)
    {
        times.push_back(tlb_miss_latency());

        flush(TARGET_PAGE);
        size_t k;
        for (k = 0; k < TLB_SET.size(); k++)
        {
            flush(&TLB_SET[k]);
        }
    }

    size_t k;
    for (k = 0; k < times.size(); k++)
    {
        if(times[k] > 0){
            std::cout << times[k] << std::endl;
        }
    }

    return 0;
}

#pragma GCC pop_options