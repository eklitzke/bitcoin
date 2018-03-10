// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <malloc_info.h>

#include <probes.h>

#ifndef HAVE_MALLOC_H
#include <cstring>

struct mallinfo mallinfo(void) {
    struct mallinfo info;
    ::memset(&info, 0, sizeof info);
    return info;
}
#endif

struct mallinfo GetMallinfo(const char *whence) {
    struct mallinfo info = mallinfo();
    if (PROBE_MALLOC_INFO_ENABLED())
        PROBE_MALLOC_INFO(whence, info.uordblks, info.fordblks);
    return info;
}
