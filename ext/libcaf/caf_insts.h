#ifndef _CAF_INSTS_H
#define _CAF_INSTS_H
#include <stdint.h>

/* Enqueue a 64-bit register value to CAF QMD */
#define CAF_ENQ(Rn, Rt) {                                                     \
    /* Rt[63:12]: QMD device base address */                                  \
    /* Rt[11:6]: qid */                                                       \
    /* Rn[63:0]: value to enqueue */                                          \
    __asm__ volatile (                                                        \
            "mov    x9,  %[val] \n\r"                                         \
            "mov    x8,  %[qmd] \n\r"                                         \
            ".word  0xd509b128  \n\r"                                         \
            :                                                                 \
            : [val]"r"(Rn), [qmd]"r"(Rt)                                      \
            : "x8", "x9"                                                      \
            );                                                                \
}

/* Dequeue a 64-bit register value from CAF QMD */
#define CAF_DEQ(Rn, Rt) {                                                     \
    /* Rt[63:12]: QMD device base address */                                  \
    /* Rt[11:6]: qid */                                                       \
    /* Rn[63:0]: value dequeued */                                            \
    __asm__ volatile (                                                        \
            "mov    x8,  %[qmd] \n\r"                                         \
            ".word  0xd509b508  \n\r"                                         \
            "mov    %[val],  x8 \n\r"                                         \
            : [val]"=r"(Rn)                                                   \
            : [qmd]"r"(Rt)                                                    \
            : "x8"                                                            \
            );                                                                \
}

/* Request credit to enqueue values to CAF QMD */
#define CAF_ENC(Rn, Rt) {                                                     \
    /* Rt[63:12]: QMD device base address */                                  \
    /* Rt[11:6]: qid */                                                       \
    /* Rn[63:0]: #values to enqueue */                                        \
    __asm__ volatile (                                                        \
            "mov    x9,  %[val] \n\r"                                         \
            "mov    x8,  %[qmd] \n\r"                                         \
            ".word  0xd509b928  \n\r"                                         \
            "mov    %[val],  x9 \n\r"                                         \
            : [val]"+r"(Rn)                                                   \
            : [qmd]"r"(Rt)                                                    \
            : "x8", "x9"                                                      \
            );                                                                \
}

/* Request credit to dequeue values from CAF QMD */
#define CAF_DEC(Rn, Rt) {                                                     \
    /* Rt[63:12]: QMD device base address */                                  \
    /* Rt[11:6]: qid */                                                       \
    /* Rn[63:0]: #values to dequeue */                                        \
    __asm__ volatile (                                                        \
            "mov    x9,  %[val] \n\r"                                         \
            "mov    x8,  %[qmd] \n\r"                                         \
            ".word  0xd509bd28  \n\r"                                         \
            "mov    %[val],  x9 \n\r"                                         \
            : [val]"+r"(Rn)                                                   \
            : [qmd]"r"(Rt)                                                    \
            : "x8", "x9"                                                      \
            );                                                                \
}

/* Prepush cachelines to PoC */
#define CAF_PREPUSH(Rn, Rt) {                                                 \
    /* Rt[63:0]: base address */                                              \
    /* Rn[63:0]: #cachelines to prepush */                                    \
    __asm__ volatile (                                                        \
            "                 mov   x9,  %[cnt]              \n\r"            \
            "PREPUSH_Dec_%=:  subs  x9,  x9,     #1          \n\r"            \
            "                 b.mi  PREPUSH_Done_%=          \n\r"            \
            "                 lsl   x8,  x9,     #6          \n\r"            \
            "                 add   x8,  x8,  %[cl]          \n\r"            \
            "                 dc    civac,       x8          \n\r"            \
            "                 b     PREPUSH_Dec_%=           \n\r"            \
            "PREPUSH_Done_%=: nop                            \n\r"            \
            :                                                                 \
            : [cnt]"r"(Rn), [cl]"r"(Rt)                                       \
            : "x8", "x9", "memory"                                            \
            );                                                                \
}

#endif /* _CAF_INSTS_H */
