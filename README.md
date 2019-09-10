*Please note that this project is currently heavily work-in-progress.*

Project website, including presentation, presentation slides and preliminary results: https://dangless.gaborkozar.me

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
 - Root (sudo) privileges
 - Enabled and sufficient number of hugepages (see below)

The remaining requirements are fairly usual:

 - A recent C compiler that supports C11 and the GNU extensions (either GCC or Clang will work)
 - Python 3.6.1 or newer
 - CMake 3.5.2 or newer

## Hugepages

Besides the above, Dune requires some 2 MB hugepages to be available. It will use these during initialization (calling `mmap()` with `MAP_HUGETLB`) for allocating memory for the safe stacks. It will also try to use huge pages for the guest's page allocator, although it will gracefully fall back if there are not enough huge pages available.

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

In my tests, it appears that at minimum **71** free huge pages are required to satisfy Dune, although it's not quite clear to me as to why: by default for 2 safe stacks of size 2 MB each, we should only need 2 hugepages.

You can dedicate more huge pages by modifying `/proc/sys/vm/nr_hugepages` (again, you'll need to use `su` to do so), or by executing:

```bash
sudo sysctl -w vm.nr_hugepages=<NUM>
```

... where `<NUM>` should be replaced by the desired number, of course.

When there isn't sufficient number of huge pages available, Dangless will fail while trying to enter Dune mode, and you will see output much like this:

>
  dune: failed to mmap() hugepage of size 2097152 for safe stack 0
  dune: setup_safe_stack() failed
  dune: create_percpu() failed
  Dangless: failed to enter Dune mode: Cannot allocate memory

# Setup

## Building and configuring Dangless

First, download the Dangless dependencies (most importantly, Dune), registered as Git submodules:

```bash
git submodule init
git submodule update
```

Then we have to apply the Dune patches and build it:

```bash
cd vendor/dune-ix

# patch dune, so that the physical page metadata is accessible inside the guest, allowing us to e.g. mess with the pagetables
git apply ../dune-ix-guestppages.patch

# patch dune, so that we can register a prehook function to run before system calls are passed to the host kernel
git apply ../dune-ix-vmcallprehook.patch

# patch dune, so that it doesn't kill the process with SIGTERM when handling the exit_group syscall - this causes runs to be registered as failures when they succeeded
git apply ../dune-ix-nosigterm.patch

# need sudo, because it's building a kernel module
sudo make
```

Now configure and build Dangless using CMake:

```bash
cd ../../sources

# you can also choose to build to a different directory
mkdir build
cd build

# you can specify your configuration options here, or e.g. use ninja (-GNinja) instead of make
cmake -D CMAKE_BUILD_TYPE=Debug -D OVERRIDE_SYMBOLS=ON -D REGISTER_PREINIT=ON -D COLLECT_STATISTICS=OFF ..
make
```

You should be able to see `libdangless_malloc.a` and `dangless_user.make` afterwards in the build directory.

You can see what configuration options were used to build Dangless by listing the CMake cache:

```bash
$ cmake -LH
-- Cache values
// Whether to allow dangless to gracefully handle running out of virtual memory and continue operating as a proxy to the underlying memory allocator.
ALLOW_SYSMALLOC_FALLBACK:BOOL=ON

// Whether Dangless should automatically dedicate any unused PML4 pagetable entries (large unused virtual memory regions) for its virtual memory allocator. If disabled, user code will have to call dangless_dedicate_vmem().
AUTODEDICATE_PML4ES:BOOL=ON

// Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.
CMAKE_BUILD_TYPE:STRING=Debug

// Install path prefix, prepended onto install directories.
CMAKE_INSTALL_PREFIX:PATH=/usr/local

// Whether to collect statistics during runtime about Dangless usage. If enabled, statistics are printed after every run to stderr. These are only for local developer use and are not uploaded anywhere.
COLLECT_STATISTICS:BOOL=OFF

// Debug mode for dangless_malloc.c
DEBUG_DGLMALLOC:BOOL=OFF

// Debug mode for vmcall_fixup.c
DEBUG_DUNE_VMCALL_FIXUP:BOOL=OFF
...
```

You can also use a CMake GUI such [CCMake\(https://cmake.org/cmake/help/latest/manual/ccmake.1.html), or check the [main CMake file](sources/CMakeLists.txt) for the list of available configuration options, their description and default values.

### Build user applications

Dangless generates a file `build/dangless_user.make` which contains Makefile variables that are useful for building user applications that rely on Dangless, such `DANGLESS_USER_CFLAGS` and `DANGLESS_USER_LDFLAGS`.

### Errors about relocations

On some systems, while linking user applications to Dangless, you may get errors like this:

> /usr/bin/ld: /.../libdangless.a(vmcall_handle_fork_asm.o): relocation R_X86_64_32S against undefined symbol 'g_current_syscall_return_addr' can not be used when making a PIE object; recompile with -fPIC
> /usr/bin/ld: /.../libdune.a(entry.o): relocation R_X86_64_32S against '.text' can not be used when making a PIE object; recompile with -fPIC
> /usr/bin/ld: /.../libdune.a(dune.o): relocation R_X86_64_32S against undefined symbol '\_\_dune\_vmcall\_hook\_running' can not be used when making a PIE object; recompile with -fPIC
> /usr/bin/ld: final link failed: Nonrepresentable section on output
> collect2: error: ld returned 1 exit status

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
make vtest

# run the trivially simple "Hello world" test app
testapps/hello-world/hello-world
```

# Troubleshooting

## Applications getting stuck

On occasion, the Dune kernel module seems to get moody, and applications in Dune mode trying to terminate fail to do so, regardless of their method of exit (`exit()`, `signal()`, `abort()`, etc.) and get stuck. At this point, any process attempting to interact with `/dev/dune` will get stuck as well.

Unfortunately, killing the processes seems ineffective, as is `rmmod dune -f`. The only workaround that I know of is restarting the computer.

# License

The 3-Clause BSD License, see `LICENSE`.
