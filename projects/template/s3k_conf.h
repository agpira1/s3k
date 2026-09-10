#pragma once

#define PLATFORM_VIRT
#include "plat/config.h"

#define S3K_PROC_CNT 2

#define S3K_CAP_CNT 32

#define S3K_CHAN_CNT 2

#define S3K_SLOT_CNT 32ull

#define S3K_SLOT_LEN (S3K_RTC_HZ / S3K_SLOT_CNT / 100ull)

#define S3K_SCHED_TIME (S3K_SLOT_LEN / 10)

//#define NDEBUG
#define VERBOSITY 0
