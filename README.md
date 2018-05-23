This project is currently heavily work-in-progress.

# Setup

```bash
git submodule init

# patch dune-ix, so that it maps the pages necessary for allocating physical pages in the virtual environment in ring 0
cd sources/vendor/dune-ix
git apply ../dune-ix.patch

# currently only the dune platform is supported
cd ../dangless
PLATFORM=dune make
```
