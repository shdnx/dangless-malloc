*Please note that this project is currently heavily work-in-progress.*

# What & why

Manual memory management in languages such as C and C++ allows the programmer to have low-level, precise control over their application's execution and performance, at the cost of additional programming burden. It's easy to make mistakes with manual memory management, for example:

 - Memory leak: allocate memory, but never free it
 - Double free: attempt to free the same memory region twice
 - Dangling pointer: free memory while still retaining a pointer to it
 - Out-of-bounds access: attempt to access memory outside of allocated regions

Such bugs are often subtle, difficult to detect, diagnose, and fix, and worse, are often exploitable by attackers. This lead to a demand for tools and technologies that can erase or mitigate these problems, in part giving rise to programming languages with automatic memory management, such as Java and C#.

Dangless-malloc is a memory allocator that replaces the built-in memory allocations functions such as `malloc()`, `calloc()` and `free()`. It aims to guarantee that even if a dangling pointer remains and is dereferenced, it will not point to a valid memory region, that is, the memory region will not be re-used until we can reasonably guarantee that it is no longer referenced.

Normally, achieving this would necessarily either waste huge amounts of physical memory, or perform a large number of `mprotect()` system calls to invalidate virtual memory pages while not deallocating them, causing a significant overhead.

Dangless-malloc works around this problem by moving the entire process into its own virtual environment ("virtual machine") using the [Dune](https://github.com/ix-project/dune) library. Inside that virtual environment, the process effectively runs in ring 0, making it possible to modify pagetables directly, without having to perform a system call.

# System requirements

Most requirements are posed by [Dune](https://github.com/ix-project/dune):

 - A 64-bit x86 Linux environment
 - A relatively recent Intel CPU with VT-x support
 - Kernel version of 4.4.0 or older
 - Installed kernel headers for the running kernel
 - Root privileges
 - Enabled and sufficient number of hugepages (see below)
 - A recent C compiler that supports C11 and the GNU extensions (either GCC or Clang will work)

## Hugepages

Besides the above, Dune requires some 2 MB hugepages to be available.

To make sure that some huge pages remain available, it's recommended to limit or disable transparent hugepages by setting `/sys/kernel/mm/transparent_hugepage/enabled` to `madvise` or `never` (you will need to use `su` if you want to change it).

Then, you can check the number of huge pages available:

```bash
$ cat /proc/meminfo | grep Huge
AnonHugePages:     49152 kB
HugePages_Total:     512
HugePages_Free:      512
HugePages_Rsvd:        0
HugePages_Surp:        0
Hugepagesize:       2048 kB
```

In my tests, it appears that at minimum **71** free huge pages are required to satisfy Dune. (TODO: why?)

You can dedicate more huge pages by modifying `/proc/sys/vm/nr_hugepages` (again, you'll need to use `su` to do so), or by executing:

```bash
sudo sysctl -w vm.nr_hugepages=<NUM>
```

... where `<NUM>` should be replaced by the desired number, of course.

When there isn't sufficient number of huge pages available, Dangless will fail while trying to enter into Dune mode, and you will see output much like this:

>
  dune: failed to mmap() hugepage of size 2097152 for safe stack 0
  dune: setup_safe_stack() failed
  dune: create_percpu() failed
  Dangless: failed to enter Dune mode: Cannot allocate memory

# Setup

## Building

Building Dangless and its dependencies, you have to do this only once:

```bash
# initialize and download dependencies (notably dune)
git submodule init
git submodule update

cd vendor/dune-ix

# patch dune, so that the physical page metadata is accessible inside the guest, allowing us to e.g. mess with the pagetables
git apply ../dune-ix-guestppages.patch

# patch dune, so that we can register a prehook function to run before system calls are passed to the host kernel
git apply ../dune-ix-vmcallprehook.patch

# patch dune, so that it doesn't kill the process with SIGTERM when handling the exit_group syscall - this causes runs to be registered as failures when they succeeded
git apply ../dune-ix-nosigterm.patch

# need sudo, because it's building a kernel module
sudo make

cd ../../sources

# currently only the dune platform is really supported
# some other configuration options can be used, see make/buildconfig-details.mk
# use PROFILE=release for benchmarking and so
make config PROFILE=debug PLATFORM=dune DUNE_ROOT=../vendor/dune-ix
make
```

### Build user applications

Dangless generates a file `sources/build/dune_<profile>/user.mk` which contains Makefile variables that are useful for building user applications that rely on Dangless, such `DANGLESS_USER_CFLAGS` and `DANGLESS_USER_LDFLAGS`.

### Errors about relocations

On some systems, while linking user applications to Dangless, you may get errors like this:

>
  /usr/bin/ld: /.../libdangless.a(vmcall_handle_fork_asm.o): relocation R_X86_64_32S against undefined symbol 'g_current_syscall_return_addr' can not be used when making a PIE object; recompile with -fPIC
  /usr/bin/ld: /.../libdune.a(entry.o): relocation R_X86_64_32S against '.text' can not be used when making a PIE object; recompile with -fPIC
  /usr/bin/ld: /.../libdune.a(dune.o): relocation R_X86_64_32S against undefined symbol '__dune_vmcall_hook_running' can not be used when making a PIE object; recompile with -fPIC
  /usr/bin/ld: final link failed: Nonrepresentable section on output
  collect2: error: ld returned 1 exit status

This means that on your system, your C compiler (usually GCC) defaults to generating *position independent code* (PIC) and so is trying to build all executables as *position independent executable*  (PIE). This unfortunately doesn't work with Dune, so it has to be disabled. You can do that by adding `-no-pie` to your `CFLAGS` and `LDFLAGS`.

## Every time the machine is rebooted

The Dune kernel module has to be loaded and permissions relaxed on `/dev/dune` (unless you want to keep using `sudo` for everything that uses Dune):

```bash
cd vendor/dune-ix

# insert the Dune kernel module
sudo insmod kern/dune.ko

# change permissions on /dev/dune so we can run Dune apps without 'sudo'
sudo chmod a+rw /dev/dune
```

## Testing

```bash
# run unit tests
make test

# run the trivially simple "Hello world" test app
make testapp APP=hello-world
```

# Troubleshooting

## Applications getting stuck

On occasion, the Dune kernel module seems to get moody, and applications in Dune mode trying to terminate fail to do so, regardless of their method of exit (`exit()`, `signal()`, `abort()`, etc.) and get stuck. At this point, any process attempting to interact with `/dev/dune` will get stuck as well.

Unfortunately, killing the processes seems ineffective, as is `rmmod dune -f`. The only workaround that I know of is restarting the computer.

# License

The 3-Clause BSD License, see `LICENSE`.
