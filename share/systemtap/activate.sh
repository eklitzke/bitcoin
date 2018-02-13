#!/bin/bash
#
# Convenience functions for working with the local builds.

if [ ! -x ./src/bitcoind ]; then
  echo "Please run this from the root project directory"
  return 1
fi

if [ -n "${_ORIG_STAP_VARS}" ]; then
  echo "Cowardly refusing to activate twice"
  return 1
fi

# Save the original value of these variables
_ORIG_STAP_VARS=("$PATH" "$PS1" "$SYSTEMTAP_TAPSET")

# Add ./src to PATH (hacky, but unfortunately required)
PATH="$PWD/src:$PATH"

# Add our tapset to the default search path
SYSTEMTAP_TAPSET="$PWD/share/systemtap/tapset"

# Mark PS1
PS1="(stap) $PS1"

# This function will restore everything to its original state.
deactivate() {
  PATH="${_ORIG_STAP_VARS[0]}"
  PS1="${_ORIG_STAP_VARS[1]}"
  SYSTEMTAP_TAPSET="${_ORIG_STAP_VARS[2]}"
  if [ -z "${SYSTEMTAP_TAPSET}" ]; then
    unset SYSTEMTAP_TAPSET
  fi
  unset _ORIG_STAP_VARS
  unset -f deactivate
}

export SYSTEMTAP_TAPSET
export _ORIG_STAP_VARS
export -f deactivate
