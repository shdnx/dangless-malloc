This project is currently heavily work-in-progress.

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
git submodule init
git submodule update

# patch dune-ix, so that it maps the pages necessary for allocating physical pages in the virtual environment in ring 0
cd vendor/dune-ix
git apply ../dune-ix.patch

# need sudo, because it's building a kernel module
sudo make

# insert the Dune kernel module
sudo insmod kern/dune.ko

# change permissions on /dev/dune so we can run Dune apps without 'sudo'
sudo chmod a+rw /dev/dune

cd ../../sources

# currently only the dune platform is really supported
# some other configuration options can be used, see make/buildconfig-details.mk
PLATFORM=dune DUNE_ROOT=../vendor/dune-ix make config
make

# run unit tests
make test
```

# License

The 3-Clause BSD License, see `LICENSE`.
