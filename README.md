This project is currently heavily work-in-progress.

# Setup

```bash
git submodule init
git submodule update

# patch dune-ix, so that it maps the pages necessary for allocating physical pages in the virtual environment in ring 0
cd sources/vendor/dune-ix
git apply ../dune-ix.patch

# need sudo, because it's building a kernel module
sudo make

# insert the Dune kernel module
sudo insmod kern/dune.ko

cd ../dangless

# currently only the dune platform is really supported
# some other configuration options can be used, see make/buildconfig-details.mk
PLATFORM=dune DUNE_ROOT=../vendor/dune-ix make config
make
```

# License

The 3-Clause BSD License, see `LICENSE`.
