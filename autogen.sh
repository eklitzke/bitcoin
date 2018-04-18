#!/bin/sh
# Copyright (c) 2013-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

set -e
cd "$(dirname "$0")"
if [ -z "${LIBTOOLIZE}" ] && GLIBTOOLIZE="command -v glibtoolize"; then
  LIBTOOLIZE="${GLIBTOOLIZE}"
  export LIBTOOLIZE
fi
if ! command -v autoreconf >/dev/null; then
  echo "configuration failed, please install autoconf first"
  exit 1
fi
autoreconf --install --force --warnings=all
