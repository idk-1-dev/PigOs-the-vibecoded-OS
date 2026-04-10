#pragma once
// pigOS lwIP port - architecture definitions
#include <stdint.h>

typedef uint8_t  u8_t;
typedef int8_t   s8_t;
typedef uint16_t u16_t;
typedef int16_t  s16_t;
typedef uint32_t u32_t;
typedef int32_t  s32_t;
typedef uint64_t u64_t;
typedef int64_t  s64_t;

typedef uintptr_t mem_ptr_t;

typedef int sys_prot_t;

#define LWIP_ERR_T int

#define U16_F "hu"
#define S16_F "hd"
#define X16_F "hx"
#define U32_F "u"
#define S32_F "d"
#define X32_F "x"
#define SZT_F "zu"

#define PACK_STRUCT_FIELD(x) x
#define PACK_STRUCT_STRUCT __attribute__((packed))
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_END

#define LWIP_PLATFORM_DIAG(x) do {} while(0)
#define LWIP_PLATFORM_ASSERT(x) do { for(;;); } while(0)

#define LWIP_RAND() ({ static uint32_t _s=0xDEAD1337; _s^=_s<<13; _s^=_s>>17; _s^=_s<<5; _s; })

// NO_SYS critical section stubs
static inline sys_prot_t sys_arch_protect(void){ return 0; }
static inline void sys_arch_unprotect(sys_prot_t pval){ (void)pval; }

// Byte swap functions for lwIP
#define lwip_htons(x) __builtin_bswap16(x)
#define lwip_htonl(x) __builtin_bswap32(x)
#define lwip_ntohs(x) __builtin_bswap16(x)
#define lwip_ntohl(x) __builtin_bswap32(x)
