# Bitcoin Systemtap Probes

Bitcoin can be built on Linux hosts with SystemTap probes. This is primarily
useful for developers working on the Bitcoin source code who want to do
low-level performance testing or analysis of the internals of Bitcoin, e.g.
access to mutexes or evaluating what the glibc memory allocator is doing.

First make sure you have a SystemTap enabled build. Information about creating a
SystemTap enabled build can be found in the [Unix build docs](build-unix.md).

## The Basics

To actually use SystemTap at a minimum you should be in the `stapusr` group:

```bash
# Add yourself to the stapusr group.
sudo gpasswd -a $USER stapusr
```

If you're on a single-user machine the easiest way to use SystemTap is to also
add yourself to the `stapdev` and `stapsys` groups. If you're not in these
groups you'll also have to set up `stap-server` to authenticate user initiated
SystemTap requests. To add yourself to all three groups:

```bash
# Add yourself to all stap groups.
for grp in stap{dev,usr,sys}; do
    sudo gpasswd -a $USER $grp
done
```

As usual you will have to log out and create a new session for the new groups to
apply (or use the `newgrp`) command. You can check what groups your current
session has using the `groups` command:

```bash
# Check that you see stapusr etc. when running groups.
$ groups
satoshi wheel stapusr stapsys stapdev  # etc.
```

### Kernel Headers

You need kernel headers installed to run SystemTap scripts:

```bash
# Debian/Ubuntu.
sudo apt-get install linux-headers-`uname -r`

# Fedora.
sudo dnf install kernel-headers
```

### Listing Bitcoin Probes

You can generally use `stap -L` to inspect what probes are available in a file,
and what arguments they provide. The following command should list SystemTap
probes (update the path to `bitcoind` as necessary):

```bash
$ stap -L 'process("/usr/bin/bitcoind").mark("*")'
process("/usr/bin/bitcoind").mark("cache_flush") $arg1:long $arg2:long
process("/usr/bin/bitcoind").mark("cache_hit")
process("/usr/bin/bitcoind").mark("cache_miss")
process("/usr/bin/bitcoind").mark("finish_ibd")
process("/usr/bin/bitcoind").mark("init_main") $arg1:long $arg2:long
process("/usr/bin/bitcoind").mark("read_block_from_disk") $arg1:long $arg2:long
process("/usr/bin/bitcoind").mark("update_tip") $arg1:long $arg2:long $arg3:long $arg4:long
```

## A Simple Bitcoin SystemTap Script

The following is a "Hello World" example for Bitcoin. You can find the following
script in `contrib/systemtap/helloworld.stp`:

```systemtap
probe process("./src/bitcoind").mark("init_main") {
  datadir = user_string($arg1)
  config = user_string($arg2)
  printf("AppInitMain with datadir=%s, config=%s\n", datadir, config);
}

probe process("./src/bitcoind").mark("update_tip") {
  height = $arg1
  printf("UpdateTip for new block at height %d\n", height)
}
```

**Important:** The path to `bitcoind` in the script must be an absolute path, or
be a valid relative path for your current working directory. SystemTap scripts
can be parameterized to with command line arguments, but this doesn't apply to
`process()` statements due to an idiosyncrasy in how the scripts are compiled.

Assuming you're using `./src/bitcoind` as your path, you should see output
similar to the following when launching bitcoind in regtest mode:

```bash
# Use stap to trace bitcoind in regtest/foreground mode.
$ stap -c "./src/bitcoind -regtest -daemon=0" ./contrib/systemtap/helloworld.stp
AppInitMain started with datadir=/home/evan/.bitcoin/regtest, config=/home/evan/.bitcoin/bitcoin.conf
```

If you have a wallet-enabled build you can use the `generate` RPC to create new
blocks, which will cause the `stap` terminal to print UpdateTip messages:

```bash
# Generate coins in another terminal.
$ ./src/bitcoin-cli -regtest generate 10
```

In the terminal running `stap` you'll see output that looks like this:

```
UpdateTip for new block at height 1
UpdateTip for new block at height 2
UpdateTip for new block at height 3
UpdateTip for new block at height 4
UpdateTip for new block at height 5
UpdateTip for new block at height 6
UpdateTip for new block at height 7
UpdateTip for new block at height 8
UpdateTip for new block at height 9
UpdateTip for new block at height 10
```

You can also use `stap -x PID` to remotely attach to a process. This is useful
if you're running `bitcoind` in daemon mode.

## Adding New Probe Points

Adding a new probe point is simple. Edit the file `src/probes.d` and add a new
probe definition, e.g.:

```
provider bitcoin {
  ...
  probe hello_world();  // our new probe
};
```

Running `make` will now generate a new `src/probes.h` header files with the
macros:

 * `PROBE_HELLO_WORLD_ENABLED()` checks if the probe is active
 * `PROBE_HELLO_WORLD()` triggers the probe

In general the pattern you'll see in the code is like this:

```c
// XXX: Braces guarding this if statement are important.
if (PROBE_HELLO_WORLD_ENABLED()) {
    PROBE_HELLO_WORLD();
}
```

You should use braces to guard the `if` body because the `PROBE_HELLO_WORLD()`
macro will expand to nothing when SystemTap is disabled. **TODO**: Come up with
a better no-op macro to avoid requiring braces.

The `src/probes.h` header file itself is generated in one of two ways:

 * If SystemTap is enabled, it's generated using
   [dtrace(1)](https://sourceware.org/systemtap/man/dtrace.1.html)
 * If SystemTap is disabled, it's generated from `share/genprobes.sh`


## Bitcoin Tapset

**TODO**: When this branch is done there will be a Bitcoin tapset which provides
a simplified API to the SystemTap markers, including things like named arguments
(instead of `$arg1`, `$arg2`, etc.). This is not yet finished, so for now you
need to explicitly name the markers in your stap scripts.

## Advanced SystemTap Topics

For most purposes the Bitcoin probes should be fine. If you want to do low-level
work, you may want to enable glibc and kernel probes, described below.

### GNU libc (glibc) SystemTap Probes

GNU libc (glibc) can be built with SystemTap probes. These add explicit probes
to the `malloc` subsystem, the `pthreads` subsystem, and the dynamic linker.
These are **very** useful if you want to understand lock contention, or what
glibc `malloc()` is doing internally. The easiest way to get a SystemTap-enabled
copy of glibc is to use Fedora, which enables SystemTap probes in glibc by
default. RedHat sponsors most of the SystemTap work, so you may find more up to
date tapsets in Fedora than other distros.

If you're using other Linux distros (including Debian or Ubuntu) you will need
to rebuild glibc to get SystemTap support. You can do this by using
`--enable-systemtap` when building glibc. Be warned that running a custom glibc
is not for the faint of heart, and can cause difficult to debug issues.

If your glibc copy has SystemTap probes, you should see malloc-related probes
listed when running the following command:

```bash
$ stap -L 'process("/lib*/libc.so.6").mark("*")'
process("/usr/lib64/libc-2.26.so").mark("lll_lock_wait_private") $arg1:long
process("/usr/lib64/libc-2.26.so").mark("longjmp") $arg1:long $arg2:long $arg3:long
process("/usr/lib64/libc-2.26.so").mark("longjmp_target") $arg1:long $arg2:long $arg3:long
process("/usr/lib64/libc-2.26.so").mark("memory_arena_new") $arg1:long $arg2:long
process("/usr/lib64/libc-2.26.so").mark("memory_arena_retry") $arg1:long $arg2:long
process("/usr/lib64/libc-2.26.so").mark("memory_arena_reuse") $arg1:long $arg2:long
process("/usr/lib64/libc-2.26.so").mark("memory_arena_reuse_free_list") $arg1:long
process("/usr/lib64/libc-2.26.so").mark("memory_arena_reuse_wait") $arg1:long $arg2:long $arg3:long
process("/usr/lib64/libc-2.26.so").mark("memory_calloc_retry") $arg1:long
process("/usr/lib64/libc-2.26.so").mark("memory_heap_free") $arg1:long $arg2:long
process("/usr/lib64/libc-2.26.so").mark("memory_heap_less") $arg1:long $arg2:long
process("/usr/lib64/libc-2.26.so").mark("memory_heap_more") $arg1:long $arg2:long
process("/usr/lib64/libc-2.26.so").mark("memory_heap_new") $arg1:long $arg2:long
process("/usr/lib64/libc-2.26.so").mark("memory_malloc_retry") $arg1:long
process("/usr/lib64/libc-2.26.so").mark("memory_mallopt") $arg1:long $arg2:long
process("/usr/lib64/libc-2.26.so").mark("memory_mallopt_arena_max") $arg1:long $arg2:long
process("/usr/lib64/libc-2.26.so").mark("memory_mallopt_arena_test") $arg1:long $arg2:long
process("/usr/lib64/libc-2.26.so").mark("memory_mallopt_free_dyn_thresholds") $arg1:long $arg2:long
process("/usr/lib64/libc-2.26.so").mark("memory_mallopt_mmap_max") $arg1:long $arg2:long $arg3:long
process("/usr/lib64/libc-2.26.so").mark("memory_mallopt_mmap_threshold") $arg1:long $arg2:long $arg3:long
process("/usr/lib64/libc-2.26.so").mark("memory_mallopt_mxfast") $arg1:long $arg2:long
process("/usr/lib64/libc-2.26.so").mark("memory_mallopt_perturb") $arg1:long $arg2:long
process("/usr/lib64/libc-2.26.so").mark("memory_mallopt_top_pad") $arg1:long $arg2:long $arg3:long
process("/usr/lib64/libc-2.26.so").mark("memory_mallopt_trim_threshold") $arg1:long $arg2:long $arg3:long
process("/usr/lib64/libc-2.26.so").mark("memory_memalign_retry") $arg1:long $arg2:long
process("/usr/lib64/libc-2.26.so").mark("memory_realloc_retry") $arg1:long $arg2:long
process("/usr/lib64/libc-2.26.so").mark("memory_sbrk_less") $arg1:long $arg2:long
process("/usr/lib64/libc-2.26.so").mark("memory_sbrk_more") $arg1:long $arg2:long
process("/usr/lib64/libc-2.26.so").mark("memory_tunable_tcache_count") $arg1:long $arg2:long
process("/usr/lib64/libc-2.26.so").mark("memory_tunable_tcache_max_bytes") $arg1:long $arg2:long
process("/usr/lib64/libc-2.26.so").mark("memory_tunable_tcache_unsorted_limit") $arg1:long $arg2:long
process("/usr/lib64/libc-2.26.so").mark("setjmp") $arg1:long $arg2:long $arg3:long
```

Likewise, you should see the following `pthreads` probes:

```bash
$ stap -L 'process("/usr/lib*/libpthread*.so").mark("*")'
process("/usr/lib64/libpthread-2.26.so").mark("cond_broadcast") $arg1:long
process("/usr/lib64/libpthread-2.26.so").mark("cond_destroy") $arg1:long
process("/usr/lib64/libpthread-2.26.so").mark("cond_init") $arg1:long $arg2:long
process("/usr/lib64/libpthread-2.26.so").mark("cond_signal") $arg1:long
process("/usr/lib64/libpthread-2.26.so").mark("cond_wait") $arg1:long $arg2:long
process("/usr/lib64/libpthread-2.26.so").mark("lll_lock_wait") $arg1:long $arg2:long
process("/usr/lib64/libpthread-2.26.so").mark("lll_lock_wait_private") $arg1:long
process("/usr/lib64/libpthread-2.26.so").mark("mutex_acquired") $arg1:long
process("/usr/lib64/libpthread-2.26.so").mark("mutex_destroy") $arg1:long
process("/usr/lib64/libpthread-2.26.so").mark("mutex_entry") $arg1:long
process("/usr/lib64/libpthread-2.26.so").mark("mutex_init") $arg1:long
process("/usr/lib64/libpthread-2.26.so").mark("mutex_release") $arg1:long
process("/usr/lib64/libpthread-2.26.so").mark("mutex_timedlock_acquired") $arg1:long
process("/usr/lib64/libpthread-2.26.so").mark("mutex_timedlock_entry") $arg1:long $arg2:long
process("/usr/lib64/libpthread-2.26.so").mark("pthread_create") $arg1:long $arg2:long $arg3:long $arg4:long
process("/usr/lib64/libpthread-2.26.so").mark("pthread_join") $arg1:long
process("/usr/lib64/libpthread-2.26.so").mark("pthread_join_ret") $arg1:long $arg2:long $arg3:long
process("/usr/lib64/libpthread-2.26.so").mark("pthread_start") $arg1:long $arg2:long $arg3:long
process("/usr/lib64/libpthread-2.26.so").mark("rdlock_acquire_read") $arg1:long
process("/usr/lib64/libpthread-2.26.so").mark("rdlock_entry") $arg1:long
process("/usr/lib64/libpthread-2.26.so").mark("rwlock_destroy") $arg1:long
process("/usr/lib64/libpthread-2.26.so").mark("rwlock_unlock") $arg1:long
process("/usr/lib64/libpthread-2.26.so").mark("wrlock_acquire_write") $arg1:long
process("/usr/lib64/libpthread-2.26.so").mark("wrlock_entry") $arg1:long
```

The `pthreads` markers are documented in the file
`nptl/DESIGN-systemtap-probes.txt` in the glibc source code. You can also view
this document online in the [glibc
cgit](https://sourceware.org/git/?p=glibc.git;a=blob_plain;f=nptl/DESIGN-systemtap-probes.txt;hb=HEAD).

### Kernel Debug Symbols

Depending on what type of work you're doing, you may want to install kernel
debug symbols. This is **optional**: SystemTap may complain if you don't have
these available, but they're only needed if you want to access kernel subsystems
from your scripts. This might be useful if you want to look at how Bitcoin is
interacting with the kernel VFS or networking subsystems, but is not necessary
if you're just doing userspace level introspection of Bitcoin.

Depending on what Linux distro you're using you can install kernel debug symbols
with one of the following commands:

```bash
# Install kernel debug symbols on Debian.
sudo apt-get install linux-image-`uname -r`-dbg

# Install kernel debug symbols on Fedora.
sudo dnf debuginfo-install kernel
```

## Additional Resources

For more information about SystemTap:

 * Comprehensive documentation can be found
   [here](https://sourceware.org/systemtap/documentation.html), including a
   language reference.
 * If you want to dive in without reading the language reference, a good resource
   is the [SystemTap Examples](https://sourceware.org/systemtap/examples/) page.
 * You can find information about the SystemTap mailing lists and git access
   [here](https://sourceware.org/systemtap/getinvolved.html).
 * On Freenode you can visit the [#systemtap](irc://irc.freenode.net/systemtap)
   IRC channel (low volume, but you'll often get help if you wait long enough).
