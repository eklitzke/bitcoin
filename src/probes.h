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
// Nothing should be enabled
#define BITCOIN_CACHE_FLUSH_START_ENABLED() 0
#define BITCOIN_CACHE_FLUSH_END_ENABLED() 0
#define BITCOIN_CACHE_HIT_ENABLED() 0
#define BITCOIN_CACHE_MISS_ENABLED() 0
#define BITCOIN_FINISH_IBD_ENABLED() 0
#define BITCOIN_CDB_WRITE_BATCH_ENABLED() 0

// And these should be no-ops.
inline void BITCOIN_CACHE_FLUSH_START(size_t arg0) {}
inline void BITCOIN_CACHE_FLUSH_END(void) {}
inline void BITCOIN_CACHE_HIT(void) {}
inline void BITCOIN_CACHE_MISS(void) {}
inline void BITCOIN_FINISH_IBD(void) {}
inline void BITCOIN_CDB_WRITE_BATCH_ENABLED(bool arg0) {}
#endif // WITH_PROBES
#endif // BITCOIN_PROBES_H
