// Copyright (c) 2015-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MALLOC_INFO_H
#define BITCOIN_MALLOC_INFO_H

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <cassert>
#include <cstddef>

#if HAVE_MALLOC_H
#include <malloc.h>
#else
struct mallinfo {
    int arena;    /* non-mmapped space allocated from system */
    int ordblks;  /* number of free chunks */
    int smblks;   /* number of fastbin blocks */
    int hblks;    /* number of mmapped regions */
    int hblkhd;   /* space in mmapped regions */
    int usmblks;  /* always 0, preserved for backwards compatibility */
    int fsmblks;  /* space available in freed fastbin blocks */
    int uordblks; /* total allocated space */
    int fordblks; /* total free space */
    int keepcost; /* top-most, releasable (via malloc_trim) space */
};

struct mallinfo mallinfo(void);
#endif // HAVE_MALLOC_H

struct mallinfo GetMallinfo(const char *whence = nullptr);

/** Compute the total memory used by allocating alloc bytes. */
inline size_t MallocUsage(size_t alloc)
{
    // Measured on libc6 2.19 on Linux.
    if (alloc == 0) {
        return 0;
    } else if (sizeof(void*) == 8) {
        return ((alloc + 31) >> 4) << 4;
    } else if (sizeof(void*) == 4) {
        return ((alloc + 15) >> 3) << 3;
    } else {
        assert(0);
    }
}

#endif // BITCOIN_MALLOC_INFO_H
