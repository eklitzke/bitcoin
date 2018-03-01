# Using SystemTap With Bitcoin

SystemTap is a Linux project analogous to DTrace. You write code in the
SystemTap scripting language, and it gets compiled to native code that runs in
the Linux kernel. The SystemTap language is powerful and allows you to use
programming constructs like loops, functions, hash tables, etc. Scripts register
callbacks that are triggered by userspace or kernel probe points, or events like
timers. Probe points added to userspace applications like Bitcoin incur an
overhead of only about 2-3 CPU cycles when the probe is not actively being used.
This makes probes a scalable way of add profiling events to userspace
applications, and can often be left in. For a general overview of SystemTap,
refer to the [SystemTap wiki](https://sourceware.org/systemtap/wiki).

Bitcoin can be built on Linux hosts with SystemTap probes. If a Bitcoin build is
not SystemTap-enabled (i.e. the default), all of the probe points become no-ops
and are eliminated by the compiler. Building Bitcoin with SystemTap probes is
primarily useful for developers working on the Bitcoin source code who want to
do low-level performance testing or analysis of the internals of Bitcoin, e.g.
to evaluate access of mutexes or to evaluate the interaction between Bitcoin
code and the glibc memory allocator.

## The Basics

First make sure you have a SystemTap enabled build. Information about creating a
SystemTap enabled build can be found in the [Unix build
docs](build-unix.md#systemtap-probes).

### Giving Yourself SystemTap Permissions

Once you have a build with SystemTap probes, you'll need to make sure your
account is configured to actually let you use SystemTap. This is required
because SystemTap scripts run in a kernel context, and therefore unprivileged
users are not allowed to execute SystemTap scripts. At a minimum you should be
in the `stapusr` group:

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

Because SystemTap scripts are compiled into kernel modules, you must have kernel
headers installed to run SystemTap scripts:

```bash
# Debian/Ubuntu
sudo apt-get install linux-headers-`uname -r`

# Fedora
sudo dnf install kernel-headers
```

## An Example Bitcoin SystemTap Script

The SystemTap language is a curly-brace language that C programmers should feel
at home in. The following is a simple example showing a simple (but interesting)
script that interact with Bitcoin probe points. This script registers callbacks
that run when:

 * `AppInitMain()` starts
 * `UpdateTip()` finds a new block at the tip
 * `IsInitialBlockDownload()` latches to false

The demo script can be found at `share/systemtap/demo.stp`, and is also
reproduced below:

```
global start_time

// Return relative time since the process was attached.
function timestr:string () {
  t = gettimeofday_us() - start_time
  secs = t / 1000000
  micros = t % 1000000
  return sprintf("%lu.%06lu", secs, micros)
}

// Run once when SystemTap attaches to the process.
probe begin {
  start_time = gettimeofday_us()
  printf("%s Attached to bitcoind process\n", timestr())
}

// Executed when AppInitMain() starts.
probe bitcoin.init_main {
  printf("%s AppInitMain with datadir=%s, config=%s\n", timestr(), datadir, config);
}

// Executed by UpdateTip() for each new block.
probe bitcoin.update_tip {
  comment = ""
  if (height == 0)
    comment = " (genesis)"
  printf("%s UpdateTip: new block %s at height %d%s\n", timestr(), block, height, comment)
}

// Executed when IsInitialBlockDownload() latches to false.
probe bitcoin.finish_ibd {
  printf("%s IBD finished\n", timestr())
}
```

This script takes advantage of the Bitcoin tapset defined in
`share/systemtap/tapset`, which provides aliases and names for the probe points
and their parameters. Assuming you're working out of the root directory of the
Bitcoin project, you can trace `bitcoind` in regtest mode using the following
commands:

```bash
# XXX: Required for local development, see "Bitcoin Tapset" below.
$ . share/systemtap/activate.sh

# You should see bitcoin tapset probes when running this.
$ stap -L 'bitcoin.*'
bitcoin.cache_flush coins:long bytes:long $arg1:long $arg2:long
bitcoin.cache_hit
bitcoin.cache_miss
...

# Use stap to trace bitcoind in regtest mode.
$ stap -c "bitcoind -regtest" share/systemtap/demo.stp
0.000000 Attached to bitcoind process
0.036602 AppInitMain with datadir=/home/evan/.bitcoin/regtest, config=/home/evan/.bitcoin/bitcoin.conf
0.785688 UpdateTip: new block 0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206 at height 0 (genesis)
```

If you have a wallet-enabled build you can use the `generate` RPC to create new
blocks:

```bash
# Generate coins in another terminal.
$ bitcoin-cli -regtest generate 5
[
  "0cf14192cab9c8c84ae26b4765ef0c3ea879fb61f0d02820f640bd72976ebfe2",
  "4c5b6282ceea0f3d72afdc3d828230ddc5cfba6118aa25a31e201f4f908d968e",
  "52265017f9658fd54bf1324e455253b14165c9db137a83463c1f1e59d42c88ca",
  "6c5fe60d657d47b7e6502cf55f232f58cd40b6b202a0f561eb58a9ec50ae6f2b",
  "7db8d490b36627a432879bc9799310ec3bb11194b38710e0d497f2fe96119852"
]
```

In the terminal running `stap` you'll see output indicating that IBD has
completed, and UpdateTip events:

```bash
# Output seen in the original stap terminal.
5.068598 IBD finished
5.068626 UpdateTip: new block 0cf14192cab9c8c84ae26b4765ef0c3ea879fb61f0d02820f640bd72976ebfe2 at height 1
5.070004 UpdateTip: new block 4c5b6282ceea0f3d72afdc3d828230ddc5cfba6118aa25a31e201f4f908d968e at height 2
5.070999 UpdateTip: new block 52265017f9658fd54bf1324e455253b14165c9db137a83463c1f1e59d42c88ca at height 3
5.071802 UpdateTip: new block 6c5fe60d657d47b7e6502cf55f232f58cd40b6b202a0f561eb58a9ec50ae6f2b at height 4
5.072559 UpdateTip: new block 7db8d490b36627a432879bc9799310ec3bb11194b38710e0d497f2fe96119852 at height 5
```

You can also use `stap -x PID` to remotely attach to a process. This is useful
if you're running `bitcoind` in daemon mode.

### Bitcoin Tapset

There's an initial tapset defined at `share/systemtap/tapset/bitcoin.stp`. The
idea is to provide a simplified tapset which abstracts the low-level bits. It's
very incomplete, and doesn't properly rename arguments.You can list probe points
in the tapset using `stap -L 'bitcoin.*'`.

**Note For Local Development**: The tapset defines probes for the target
`process("bitcoind")`. The parameter to `process()` must either be an absolute
path, or an executable that can be found by search `$PATH`. This isn't a problem
if you've actually installed things with `make install`, but can be inconvenient
if you're accustomed to working out of the root of the Bitcoin source tree.

There is a helper script at `share/systemtap/activate.sh`. When sourced, it adds
`$PWD/src` to `$PATH` and sets and exports `SYSTEMTAP_TAPSET`. If you're
uncomfortable doing this, you can always use `-I share/systemtap/tapset` when
invoking `stap` to add the tapset directory to the `stap` search path. With
regard to locating your local `bitcoind` executable, any of the following
techniques will work:

 * Change your working directory to `src/` when running `stap` commands
 * Temporarily add the `src/` directory at the front of `$PATH`
 * Temporarily change the `@BITCOIND` macro defined in
   `share/systemtap/tapset/bitcoin.stp` to use a different executable target,
   such as `src/bitcoind`

If you have an incorrect configuration, the `stap` error messages will look like this:

```
$ stap -I ./share/systemtap/tapset -c "./src/bitcoind -regtest" ./share/systemtap/demo.stp 
semantic error: while resolving probe point: identifier 'process' at ./share/systemtap/tapset/bitcoin.stp:14:27
        source: probe bitcoin.init_main = process(@BITCOIND).mark("init_main") {
                                          ^

semantic error: cannot find executable 'bitcoind'

semantic error: while resolving probe point: identifier 'bitcoin' at ./share/systemtap/demo.stp:18:7
        source: probe bitcoin.init_main {
                      ^

semantic error: no match
```

## Adding New Probe Points

Adding a new probe point is simple. Edit the file `src/probes.d` and add a new
probe definition, e.g.:

```dtrace
provider bitcoin
{
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
if (PROBE_HELLO_WORLD_ENABLED())
    PROBE_HELLO_WORLD();
```

The `src/probes.h` header file itself is generated in one of two ways:

 * If SystemTap is enabled, it's generated using
   [dtrace(1)](https://sourceware.org/systemtap/man/dtrace.1.html)
 * If SystemTap is disabled, it's generated from `share/genprobes.sh`

After adding a new probe point it's a good practice to rebuild Bitcoin with
`--disable-systemtap` to make sure there are no errors when probes are disabled.
If you're switching back and forth between systemtap enabled/disabled builds,
you may need to force relinking by deleting the executables/object files before
running `make`.

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
