// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PROBES_H
#define BITCOIN_PROBES_H

#include <config/bitcoin-config.h>

#ifdef WITH_PROBES
// This file is not checked in: it's generated as part of the build process. For
// the gory details, see configure.ac and src/Makefile.am
#include <probes_impl.h>

#else

// Create an "enabled" macro that always returns false.
#define DEFINE_PROBE_ENABLED(name)\
    inline int BITCOIN_ ## name ## _ENABLED(void) { return 0; }

// Nothing should be enabled.
DEFINE_PROBE_ENABLED(CACHE_FLUSH_START)
DEFINE_PROBE_ENABLED(CACHE_FLUSH_END)
DEFINE_PROBE_ENABLED(CACHE_HIT)
DEFINE_PROBE_ENABLED(CACHE_MISS)
DEFINE_PROBE_ENABLED(FINISH_IBD)
DEFINE_PROBE_ENABLED(CDB_WRITE_BATCH)
DEFINE_PROBE_ENABLED(UPDATE_TIP)

// And these should be no-ops.
inline void BITCOIN_CACHE_FLUSH(size_t, size_t) {}
inline void BITCOIN_CACHE_HIT(void) {}
inline void BITCOIN_CACHE_MISS(void) {}
inline void BITCOIN_FINISH_IBD(void) {}
inline void BITCOIN_CDB_WRITE_BATCH(size_t, bool) {}
inline void BITCOIN_UPDATE_TIP(uint32_t, size_t, size_t, size_t, unsigned int) {}
#endif // WITH_PROBES
#endif // BITCOIN_PROBES_H
