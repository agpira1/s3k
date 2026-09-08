#pragma once

#define PLATFORM_VIRT
#include "plat/config.h"

// Number of user processes. Every process exists from boot; there is no
// process creation. Raise this before starting a second program.
#define S3K_PROC_CNT 2

// Number of capabilities per process. Indices 0-9 of process 0 are filled
// from INIT_CAPS, the rest are free.
#define S3K_CAP_CNT 32

// Number of IPC channels.
#define S3K_CHAN_CNT 2

// Number of slots per period
#define S3K_SLOT_CNT 32ull

// Length of slots in ticks.
#define S3K_SLOT_LEN (S3K_RTC_HZ / S3K_SLOT_CNT)

// Scheduler time
#define S3K_SCHED_TIME (S3K_SLOT_LEN / 10)

// If debugging, comment
//#define NDEBUG
#define VERBOSITY 0
