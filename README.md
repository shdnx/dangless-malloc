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
 - Enabled transparent hugepages:
    - `/sys/kernel/mm/transparent_hugepage/enabled` is set to `madvise` or `always`
    - `/proc/sys/vm/nr_hugepages` is a sufficiently large number (200 does the trick on my machine)

Besides this, my code requires a relatively recent version of GCC (or a compiler that supports the GNU extensions to C11). GCC version 5 or newer should work.

# Setup

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

# insert the Dune kernel module
sudo insmod kern/dune.ko

# change permissions on /dev/dune so we can run Dune apps without 'sudo'
sudo chmod a+rw /dev/dune

cd ../../sources

# currently only the dune platform is really supported
# some other configuration options can be used, see make/buildconfig-details.mk
# use PROFILE=release for benchmarking and so
make config PROFILE=debug PLATFORM=dune DUNE_ROOT=../vendor/dune-ix
make

# run unit tests
make test

# run "Hello world" test app
make testapp APP=hello-world
```

# License

The 3-Clause BSD License, see `LICENSE`.
