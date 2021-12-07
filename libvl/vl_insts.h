#ifndef _VL_INSTS_H
#define _VL_INSTS_H
#include <stdint.h>

#if NOGEM5
/* w/o gem5 support of new instructions, fake them with pure assembly blocks */

/* Select cacheline and set its tag enable push by virtual address until PoC */
#define VL_DCSVAC(Rn) {                                                       \
    /* Rn[63:0]: pass cacheline address */                                    \
    Rn = Rn;                                                                  \
}
/* Push previously selected cacheline to virtual address by PoC */
#define VL_DCPSVAC(Rs, Rn) {                                                  \
    /* Rn[63:0]: pass cacheline address */                                    \
    /* Rs[0]: return result of DC PSVAC, 0 - succeed, 1 - fail */             \
    Rn = Rn;                                                                  \
    Rs = 0;                                                                   \
}
/* Pull previously selected cacheline from virtual address by PoC */
#define VL_DCFSVAC(Rn) {                                                      \
    /* Rn[63:0]: pass cacheline address */                                    \
    Rn = Rn;                                                                  \
}

#else /* NOGEM5 false */
#include "gem5/m5ops.h"
/* include gem5 header and use new instructions */

/* Select cacheline and set its tag enable push by virtual address until PoC */
#define VL_DCSVAC(Rn) {                                                       \
    /* Rn[63:0]: pass cacheline address */                                    \
    __asm__ volatile (                                                        \
            "mov    x8,  %[cva] \n\r"                                         \
            ".word  0xd508b008  \n\r"                                         \
            :                                                                 \
            : [cva]"r"(Rn)                                                    \
            : "x8"                                                            \
            );                                                                \
}
#if NEEDDBM
/* Push previously selected cacheline to virtual address by PoC */
#define VL_DCPSVAC(Rs, Rn) {                                                  \
    /* Rn[63:0]: pass cacheline address */                                    \
    /* Rs[0]: return result of DC PSVAC, 0 - succeed, 1 - fail */             \
    __asm__ volatile (                                                        \
            "mov    x8,  %[cva] \n\r"                                         \
            "dmb    ish         \n\r"                                         \
            ".word  0xd508b028  \n\r"                                         \
            "mov    %[res],  x8 \n\r"                                         \
            : [res]"=r"(Rs)                                                   \
            : [cva]"r"(Rn)                                                    \
            : "x8"                                                            \
            );                                                                \
}
/* Pull previously selected cacheline from virtual address by PoC */
#define VL_DCFSVAC(Rn) {                                                      \
    /* Rn[63:0]: pass cacheline address */                                    \
    __asm__ volatile (                                                        \
            "mov    x8,  %[cva] \n\r"                                         \
            "dmb    ish         \n\r"                                         \
            ".word  0xd508b048  \n\r"                                         \
            :                                                                 \
            : [cva]"r"(Rn)                                                    \
            : "x8"                                                            \
            );                                                                \
}
#else /* NEEDDBM false, DC PSVAC, DC FSVAC are barrier themselves */
/* Push previously selected cacheline to virtual address by PoC */
#define VL_DCPSVAC(Rs, Rn) {                                                  \
    /* Rn[63:0]: pass cacheline address */                                    \
    /* Rs[0]: return result of DC PSVAC, 0 - succeed, 1 - fail */             \
    __asm__ volatile (                                                        \
            "mov    x8,  %[cva] \n\r"                                         \
            ".word  0xd508b028  \n\r"                                         \
            "mov    %[res],  x8 \n\r"                                         \
            : [res]"=r"(Rs)                                                   \
            : [cva]"r"(Rn)                                                    \
            : "x8"                                                            \
            );                                                                \
}
/* Pull previously selected cacheline from virtual address by PoC */
#define VL_DCFSVAC(Rn) {                                                      \
    /* Rn[63:0]: pass cacheline address */                                    \
    __asm__ volatile (                                                        \
            "mov    x8,  %[cva] \n\r"                                         \
            ".word  0xd508b048  \n\r"                                         \
            :                                                                 \
            : [cva]"r"(Rn)                                                    \
            : "x8"                                                            \
            );                                                                \
}
#endif /* NEEDDBM */
#endif /* NOGEM5 */

/* Update address pointing to cacheline with next cacheline address */
#define VL_Next(pRn) {                                                        \
    /* Rn[63:0]: pass cacheline base address */                               \
    /* Rn[63:0]: return next cacheline address */                             \
    volatile void *cva_mAng1ed = (volatile void*)*(pRn);                      \
    __asm__ volatile (                                                        \
            "ldrb  w10,     [%[cva], #63]        \n\r"                        \
            "sxtb  x10,     w10                  \n\r"                        \
            "lsl   x10,     x10,     #6          \n\r"                        \
            "add   %[cva],  %[cva],  x10         \n\r"                        \
            : [cva]"+r"(cva_mAng1ed)                                          \
            :                                                                 \
            /* x10 - Ctrl.Next */                                             \
            : "x10"                                                           \
            );                                                                \
    *(pRn) = cva_mAng1ed;                                                     \
}

/* Enqueue a byte */
#define VL_ENQB(pRs, Rt, Rn) {                                                \
    /* Rt[7:0]: pass byte data to store */                                    \
    /* Rn[63:0]: pass producer cacheline address */                           \
    /* Rs[0]: return if the cacheline is full (1) or not (0) */               \
    /* Rs[1]: return if the data is successfully pushed (0) or failed (1) */  \
    uint64_t res_mAng1ed;                                                     \
    __asm__ volatile (                                                        \
            /* Step 0: check cacheline not full (62 > Ptr >= 0) */            \
            /* load control region [Next 6-bit][Size 2-bit][Ptr 6-bit] */     \
            "ENQB_Check_%=:   add   x8,  %[cva], #62         \n\r"            \
            "                 ldxrb w8,  [x8]                \n\r"            \
            "                 and   w8,  w8,  0x03F          \n\r"            \
            "                 cmp   w8,  #61                 \n\r"            \
            "                 b.gt  ENQB_Full_%=             \n\r"            \
            /* Step 1: get full address and store */                          \
            "ENQB_Store_%=:   add   x9,  x8,  %[cva]         \n\r"            \
            "                 strb  %w[val],  [x9]           \n\r"            \
            /* Step 2: decrement Ptr and check if underflow */                \
            "ENQB_PtrDec_%=:  subs  w8,  w8,  #1             \n\r"            \
            /* Step 3: update decreased Ptr to control region */              \
            "                 and   w8,  w8,  0x3F           \n\r"            \
            /*"                 orr   w8,  w8,  0x00           \n\r" */       \
            "                 strb  w8,  [%[cva], #62]       \n\r"            \
            /* Step 4: set Rs 00 for done, 01 for filled up (needs next) */   \
            "ENQB_ClrRs_%=:   mov   %[res],   0x0            \n\r"            \
            "                 b.pl  ENQB_End_%=              \n\r"            \
            "ENQB_Next_%=:    add   %[res],   %[res],  0x1   \n\r"            \
            "                 b     ENQB_End_%=              \n\r"            \
            /* step 1-f: pushing to a cacheline already full, Rs = 11 */      \
            "ENQB_Full_%=:    mov   %[res],   0x3            \n\r"            \
            "ENQB_End_%=:                                    \n\r"            \
            : [res]"=r" (res_mAng1ed)                                         \
            : [val]"r" (Rt), [cva]"r" (Rn)                                    \
            /* x8 - Ctrl/Ptr, x9 - addr_dst */                                \
            : "x8", "x9", "cc", "memory"                                      \
            );                                                                \
    *(pRs) = res_mAng1ed;                                                     \
    /* Rs[1] 1 - data in Rt is not pushed, 0 - committed to cacheline */      \
    /* Rs[0] 1 - cacheline pointed by Rn is full, 0 - not full */             \
}

/* Enqueue a half word */
#define VL_ENQH(pRs, Rt, Rn) {                                                \
    /* Rt[15:0]: pass half word data to store */                              \
    /* Rn[63:0]: pass producer cacheline address */                           \
    /* Rs[0]: return if the cacheline is full (1) or not (0) */               \
    /* Rs[1]: return if the data is successfully pushed (0) or failed (1) */  \
    uint64_t res_mAng1ed;                                                     \
    __asm__ volatile (                                                        \
            /* Step 0: check cacheline not full (61 > Ptr >= 0) */            \
            /* load control region [Next 6-bit][Size 2-bit][Ptr 6-bit] */     \
            "ENQH_Check_%=:   add   x8,  %[cva], #62         \n\r"            \
            "                 ldxrb w8,  [x8]                \n\r"            \
            "                 and   w8,  w8,  0x03F          \n\r"            \
            "                 cmp   w8,  #60                 \n\r"            \
            "                 b.gt  ENQH_Full_%=             \n\r"            \
            /* Step 1: get full address and store */                          \
            "ENQH_Store_%=:   add   x9,  x8,  %[cva]         \n\r"            \
            "                 strh  %w[val],  [x9]           \n\r"            \
            /* Step 2: decrement Ptr and check if underflow */                \
            "ENQH_PtrDec_%=:  subs  w8,  w8,  #2             \n\r"            \
            /* Step 3: update decreased Ptr to control region */              \
            "                 and   w8,  w8,  0x3F           \n\r"            \
            "                 orr   w8,  w8,  0x40           \n\r"            \
            "                 strb  w8,  [%[cva], #62]       \n\r"            \
            /* Step 4: set Rs 00 for done, 01 for filled up (needs next) */   \
            "ENQH_ClrRs_%=:   mov   %[res],   0x0            \n\r"            \
            "                 b.pl  ENQH_End_%=              \n\r"            \
            "ENQH_Next_%=:    add   %[res],   %[res],  0x1   \n\r"            \
            "                 b     ENQH_End_%=              \n\r"            \
            /* step 1-f: pushing to a cacheline already full, Rs = 11 */      \
            "ENQH_Full_%=:    mov   %[res],   0x3            \n\r"            \
            "ENQH_End_%=:                                    \n\r"            \
            : [res]"=r" (res_mAng1ed)                                         \
            : [val]"r" (Rt), [cva]"r" (Rn)                                    \
            /* x8 - Ctrl/Ptr, x9 - addr_dst */                                \
            : "x8", "x9", "cc", "memory"                                      \
            );                                                                \
    *(pRs) = res_mAng1ed;                                                     \
    /* Rs[1] 1 - data in Rt is not pushed, 0 - committed to cacheline */      \
    /* Rs[0] 1 - cacheline pointed by Rn is full, 0 - not full */             \
}

/* Enqueue a word */
#define VL_ENQ32(pRs, Rt, Rn) {                                               \
    /* Rt[31:0]: pass word data to store */                                   \
    /* Rn[63:0]: pass producer cacheline address */                           \
    /* Rs[0]: return if the cacheline is full (1) or not (0) */               \
    /* Rs[1]: return if the data is successfully pushed (0) or failed (1) */  \
    uint64_t res_mAng1ed;                                                     \
    __asm__ volatile (                                                        \
            /* Step 0: check cacheline not full (57 > Ptr >= 0) */            \
            /* load control region [Next 6-bit][Size 2-bit][Ptr 6-bit] */     \
            "ENQ32_Check_%=:  add   x8,  %[cva], #62         \n\r"            \
            "                 ldxrb w8,  [x8]                \n\r"            \
            "                 and   w8,  w8,  0x03F          \n\r"            \
            "                 cmp   w8,  #56                 \n\r"            \
            "                 b.gt  ENQ32_Full_%=            \n\r"            \
            /* Step 1: get full address and store */                          \
            "ENQ32_Store_%=:  add   x9,  x8,  %[cva]         \n\r"            \
            "                 str   %w[val],  [x9]           \n\r"            \
            /* Step 2: decrement Ptr and check if underflow */                \
            "ENQ32_PtrDec_%=: subs  w8,  w8,  #4             \n\r"            \
            /* Step 3: update decreased Ptr to control region */              \
            "                 and   w8,  w8,  0x3F           \n\r"            \
            "                 orr   w8,  w8,  0x80           \n\r"            \
            "                 strb  w8,  [%[cva], #62]       \n\r"            \
            /* Step 4: set Rs 00 for done, 01 for filled up (needs next) */   \
            "ENQ32_ClrRs_%=:  mov   %[res],   0x0            \n\r"            \
            "                 b.pl  ENQ32_End_%=             \n\r"            \
            "ENQ32_Next_%=:   add   %[res],   %[res],  0x1   \n\r"            \
            "                 b     ENQ32_End_%=             \n\r"            \
            /* step 1-f: pushing to a cacheline already full, Rs = 11 */      \
            "ENQ32_Full_%=:   mov   %[res],   0x3            \n\r"            \
            "ENQ32_End_%=:                                   \n\r"            \
            : [res]"=r" (res_mAng1ed)                                         \
            : [val]"r" (Rt), [cva]"r" (Rn)                                    \
            /* x8 - Ctrl/Ptr, x9 - addr_dst */                                \
            : "x8", "x9", "cc", "memory"                                      \
            );                                                                \
    *(pRs) = res_mAng1ed;                                                     \
    /* Rs[1] 1 - data in Rt is not pushed, 0 - committed to cacheline */      \
    /* Rs[0] 1 - cacheline pointed by Rn is full, 0 - not full */             \
}

/* Enqueue a double word */
#define VL_ENQ64(pRs, Rt, Rn) {                                               \
    /* Rt[63:0]: pass double word data to store */                            \
    /* Rn[63:0]: pass producer cacheline address */                           \
    /* Rs[0]: return if the cacheline is full (1) or not (0) */               \
    /* Rs[1]: return if the data is successfully pushed (0) or failed (1) */  \
    uint64_t res_mAng1ed;                                                     \
    __asm__ volatile (                                                        \
            /* Step 0: check cacheline not full (49 > Ptr >= 0) */            \
            /* load control region [Next 6-bit][Size 2-bit][Ptr 6-bit] */     \
            "ENQ64_Check_%=:  add   x8,  %[cva], #62         \n\r"            \
            "                 ldxrb w8,  [x8]                \n\r"            \
            "                 and   w8,  w8,  0x03F          \n\r"            \
            "                 cmp   w8,  #48                 \n\r"            \
            "                 b.gt  ENQ64_Full_%=            \n\r"            \
            /* Step 1: get full address and store */                          \
            "ENQ64_Store_%=:  add   x9,  x8,  %[cva]         \n\r"            \
            "                 str   %[val],   [x9]           \n\r"            \
            /* Step 2: decrement Ptr and check if underflow */                \
            "ENQ64_PtrDec_%=: subs  w8,  w8,  #8             \n\r"            \
            /* Step 3: update decreased Ptr to control region */              \
            "                 and   w8,  w8,  0x3F           \n\r"            \
            "                 orr   w8,  w8,  0xC0           \n\r"            \
            "                 strb  w8,  [%[cva], #62]       \n\r"            \
            /* Step 4: set Rs 00 for done, 01 for filled up (needs next) */   \
            "ENQ64_ClrRs_%=:  mov   %[res],   0x0            \n\r"            \
            "                 b.pl  ENQ64_End_%=             \n\r"            \
            "ENQ64_Next_%=:   add   %[res],   %[res],  0x1   \n\r"            \
            "                 b     ENQ64_End_%=             \n\r"            \
            /* step 1-f: pushing to a cacheline already full, Rs = 11 */      \
            "ENQ64_Full_%=:   mov   %[res],   0x3            \n\r"            \
            "ENQ64_End_%=:                                   \n\r"            \
            : [res]"=r" (res_mAng1ed)                                         \
            : [val]"r" (Rt), [cva]"r" (Rn)                                    \
            /* x8 - Ctrl/Ptr, x9 - addr_dst */                                \
            : "x8", "x9", "cc", "memory"                                      \
            );                                                                \
    *(pRs) = res_mAng1ed;                                                     \
    /* Rs[1] 1 - data in Rt is not pushed, 0 - committed to cacheline */      \
    /* Rs[0] 1 - cacheline pointed by Rn is full, 0 - not full */             \
}

/* Dequeue a byte */
#define VL_DEQB(pRs, pRt, Rn) {                                               \
    /* Rn[63:0]: pass consumer cacheline address */                           \
    /* Rt[7:0]: return byte data  */                                          \
    /* Rs[0]: return if the cacheline is full (1) or not (0) */               \
    /* Rs[1]: return if the data is successfully poped (0) or failed (1) */   \
    uint64_t res_mAng1ed;                                                     \
    uint8_t val_mAng1ed;                                                      \
    __asm__ volatile (                                                        \
            /* Step 0: check cacheline not empty (62 > Ptr >= 0) */           \
            /* load control region [Next 6-bit][Size 2-bit][Ptr 6-bit] */     \
            "DEQB_Check_%=:   add   x8,  %[cva], #62         \n\r"            \
            "                 ldxrb w8,  [x8]                \n\r"            \
            "                 and   w8,  w8,  0x03F          \n\r"            \
            "                 cmp   w8,  #61                 \n\r"            \
            "                 b.gt  DEQB_Empty_%=            \n\r"            \
            /* Step 1: get full address and load */                           \
            "DEQB_Load_%=:    add   x9,  x8,  %[cva]         \n\r"            \
            "                 ldrb  %w[val],  [x9]           \n\r"            \
            /* Step 2: decrement Ptr and check if underflow */                \
            "DEQB_PtrDec_%=:  subs  w8,  w8,  #1             \n\r"            \
            /* Step 3: update decreased Ptr to control region */              \
            "                 and   w8,  w8,  0x3F           \n\r"            \
            /*"                 orr   w8,  w8,  0x00           \n\r" */       \
            "                 strb  w8,  [%[cva], #62]       \n\r"            \
            /* Step 4: set Rs 00 for done, 01 for used up (needs next) */     \
            "DEQB_ClrRs_%=:   mov   %[res],   0x0            \n\r"            \
            "                 b.pl  DEQB_End_%=              \n\r"            \
            "DEQB_Next_%=:    add   %[res],   %[res],  0x1   \n\r"            \
            "                 b     DEQB_End_%=              \n\r"            \
            /* Step 1-E: popping from an empty cacheline, Rs = 11 */          \
            "DEQB_Empty_%=:   mov   %[res],   0x3            \n\r"            \
            "DEQB_End_%=:                                    \n\r"            \
            : [res]"=r" (res_mAng1ed), [val]"=r" (val_mAng1ed)                \
            : [cva]"r" (Rn)                                                   \
            /* x8 - Ctrl/Ptr, x9 - addr_src */                                \
            : "x8", "x9", "cc", "memory"                                      \
            );                                                                \
    *(pRs) = res_mAng1ed;                                                     \
    /* Rs[1] 1 - data in Rt is not valid, 0 - valid data from cacheline */    \
    /* Rs[0] 1 - cacheline pointed by Rn is empty, 0 - not empty */           \
    *(pRt) = val_mAng1ed;                                                     \
    /* Rt[7:0] with valid data dequeued if Rs[1] is 0 */                      \
}

/* Dequeue a half word */
#define VL_DEQH(pRs, pRt, Rn) {                                               \
    /* Rn[63:0]: pass consumer cacheline address */                           \
    /* Rt[16:0]: return half word data  */                                    \
    /* Rs[0]: return if the cacheline is full (1) or not (0) */               \
    /* Rs[1]: return if the data is successfully poped (0) or failed (1) */   \
    uint64_t res_mAng1ed;                                                     \
    uint16_t val_mAng1ed;                                                     \
    __asm__ volatile (                                                        \
            /* Step 0: check cacheline not empty (61 > Ptr >= 0) */           \
            /* load control region [Next 6-bit][Size 2-bit][Ptr 6-bit] */     \
            "DEQH_Check_%=:   add   x8,  %[cva], #62         \n\r"            \
            "                 ldxrb w8,  [x8]                \n\r"            \
            "                 and   w8,  w8,  0x03F          \n\r"            \
            "                 cmp   w8,  #60                 \n\r"            \
            "                 b.gt  DEQH_Empty_%=            \n\r"            \
            /* Step 1: get full address and load */                           \
            "DEQH_Load_%=:    add   x9,  x8,  %[cva]         \n\r"            \
            "                 ldrh  %w[val],  [x9]           \n\r"            \
            /* Step 2: decrement Ptr and check if underflow */                \
            "DEQH_PtrDec_%=:  subs  w8,  w8,  #2             \n\r"            \
            /* Step 3: update decreased Ptr to control region */              \
            "                 and   w8,  w8,  0x3F           \n\r"            \
            "                 orr   w8,  w8,  0x40           \n\r"            \
            "                 strb  w8,  [%[cva], #62]       \n\r"            \
            /* Step 4: set Rs 00 for done, 01 for used up (needs next) */     \
            "DEQH_ClrRs_%=:   mov   %[res],   0x0            \n\r"            \
            "                 b.pl  DEQH_End_%=              \n\r"            \
            "DEQH_Next_%=:    add   %[res],   %[res],  0x1   \n\r"            \
            "                 b     DEQH_End_%=              \n\r"            \
            /* Step 1-E: popping from an empty cacheline, Rs = 11 */          \
            "DEQH_Empty_%=:   mov   %[res],   0x3            \n\r"            \
            "DEQH_End_%=:                                    \n\r"            \
            : [res]"=r" (res_mAng1ed), [val]"=r" (val_mAng1ed)                \
            : [cva]"r" (Rn)                                                   \
            /* x8 - Ctrl/Ptr, x9 - addr_src */                                \
            : "x8", "x9", "cc", "memory"                                      \
            );                                                                \
    *(pRs) = res_mAng1ed;                                                     \
    /* Rs[1] 1 - data in Rt is not valid, 0 - valid data from cacheline */    \
    /* Rs[0] 1 - cacheline pointed by Rn is empty, 0 - not empty */           \
    *(pRt) = val_mAng1ed;                                                     \
    /* Rt[15:0] with valid data dequeued if Rs[1] is 0 */                     \
}

/* Dequeue a word */
#define VL_DEQ32(pRs, pRt, Rn) {                                              \
    /* Rn[63:0]: pass consumer cacheline address */                           \
    /* Rt[31:0]: return word data  */                                         \
    /* Rs[0]: return if the cacheline is full (1) or not (0) */               \
    /* Rs[1]: return if the data is successfully poped (0) or failed (1) */   \
    uint64_t res_mAng1ed;                                                     \
    uint32_t val_mAng1ed;                                                     \
    __asm__ volatile (                                                        \
            /* Step 0: check cacheline not empty (57 > Ptr >= 0) */           \
            /* load control region [Next 6-bit][Size 2-bit][Ptr 6-bit] */     \
            "DEQ32_Check_%=:  add   x8,  %[cva], #62         \n\r"            \
            "                 ldxrb w8,  [x8]                \n\r"            \
            "                 and   w8,  w8,  0x03F          \n\r"            \
            "                 cmp   w8,  #56                 \n\r"            \
            "                 b.gt  DEQ32_Empty_%=           \n\r"            \
            /* Step 1: get full address and load */                           \
            "DEQ32_Load_%=:   add   x9,  x8,  %[cva]         \n\r"            \
            "                 ldr   %w[val],  [x9]           \n\r"            \
            /* Step 2: decrement Ptr and check if underflow */                \
            "DEQ32_PtrDec_%=: subs  w8,  w8,  #4             \n\r"            \
            /* Step 3: update decreased Ptr to control region */              \
            "                 and   w8,  w8,  0x3F           \n\r"            \
            "                 orr   w8,  w8,  0x80           \n\r"            \
            "                 strb  w8,  [%[cva], #62]       \n\r"            \
            /* Step 4: set Rs 00 for done, 01 for used up (needs next) */     \
            "DEQ32_ClrRs_%=:  mov   %[res],   0x0            \n\r"            \
            "                 b.pl  DEQ32_End_%=             \n\r"            \
            "DEQ32_Next_%=:   add   %[res],   %[res],  0x1   \n\r"            \
            "                 b     DEQ32_End_%=             \n\r"            \
            /* Step 1-E: popping from an empty cacheline, Rs = 11 */          \
            "DEQ32_Empty_%=:  mov   %[res],   0x3            \n\r"            \
            "DEQ32_End_%=:                                   \n\r"            \
            : [res]"=r" (res_mAng1ed), [val]"=r" (val_mAng1ed)                \
            : [cva]"r" (Rn)                                                   \
            /* x8 - Ctrl/Ptr, x9 - addr_src */                                \
            : "x8", "x9", "cc", "memory"                                      \
            );                                                                \
    *(pRs) = res_mAng1ed;                                                     \
    /* Rs[1] 1 - data in Rt is not valid, 0 - valid data from cacheline */    \
    /* Rs[0] 1 - cacheline pointed by Rn is empty, 0 - not empty */           \
    *(pRt) = val_mAng1ed;                                                     \
    /* Rt[31:0] with valid data dequeued if Rs[1] is 0 */                     \
}

/* Dequeue a double word */
#define VL_DEQ64(pRs, pRt, Rn) {                                              \
    /* Rn[63:0]: pass consumer cacheline address */                           \
    /* Rt[63:0]: return double word data  */                                  \
    /* Rs[0]: return if the cacheline is full (1) or not (0) */               \
    /* Rs[1]: return if the data is successfully poped (0) or failed (1) */   \
    uint64_t res_mAng1ed;                                                     \
    uint64_t val_mAng1ed;                                                     \
    __asm__ volatile (                                                        \
            /* Step 0: check cacheline not empty (49 > Ptr >= 0) */           \
            /* load control region [Next 6-bit][Size 2-bit][Ptr 6-bit] */     \
            "DEQ64_Check_%=:  add   x8,  %[cva], #62         \n\r"            \
            "                 ldxrb w8,  [x8]                \n\r"            \
            "                 and   w8,  w8,  0x03F          \n\r"            \
            "                 cmp   w8,  #48                 \n\r"            \
            "                 b.gt  DEQ64_Empty_%=           \n\r"            \
            /* Step 1: get full address and load */                           \
            "DEQ64_Load_%=:   add   x9,  x8,  %[cva]         \n\r"            \
            "                 ldr   %[val],   [x9]           \n\r"            \
            /* Step 2: decrement Ptr and check if underflow */                \
            "DEQ64_PtrDec_%=: subs  w8,  w8,  #8             \n\r"            \
            /* Step 3: update decreased Ptr to control region */              \
            "                 and   w8,  w8,  0x3F           \n\r"            \
            "                 orr   w8,  w8,  0xC0           \n\r"            \
            "                 strb  w8,  [%[cva], #62]       \n\r"            \
            /* Step 4: set Rs 00 for done, 01 for used up (needs next) */     \
            "DEQ64_ClrRs_%=:  mov   %[res],   0x0            \n\r"            \
            "                 b.pl  DEQ64_End_%=             \n\r"            \
            "DEQ64_Next_%=:   add   %[res],   %[res],  0x1   \n\r"            \
            "                 b     DEQ64_End_%=             \n\r"            \
            /* Step 1-E: popping from an empty cacheline, Rs = 11 */          \
            "DEQ64_Empty_%=:  mov   %[res],   0x3            \n\r"            \
            "DEQ64_End_%=:                                   \n\r"            \
            : [res]"=r" (res_mAng1ed), [val]"=r" (val_mAng1ed)                \
            : [cva]"r" (Rn)                                                   \
            /* x8 - Ctrl/Ptr, x9 - addr_src */                                \
            : "x8", "x9", "cc", "memory"                                      \
            );                                                                \
    *(pRs) = res_mAng1ed;                                                     \
    /* Rs[1] 1 - data in Rt is not valid, 0 - valid data from cacheline */    \
    /* Rs[0] 1 - cacheline pointed by Rn is empty, 0 - not empty */           \
    *(pRt) = val_mAng1ed;                                                     \
    /* Rt[63:0] with valid data dequeued if Rs[1] is 0 */                     \
}

#endif /* _VL_INSTS_H */
