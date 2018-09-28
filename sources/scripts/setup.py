#!/usr/bin/env python3
# PYTHON_ARGCOMPLETE_OK
import sys
import os.path
import shutil

DEBUG = True
CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))

if not os.path.exists(os.path.join(CURRENT_DIR, "README.md")):
    msg = "This script is meant to be referenced through its symlink in the project root (parent folder of 'sources')\n"

    if __name__ == "__main__":
        sys.stderr.write("Error: " + msg)
        sys.exit(1)
    else:
        raise RuntimeError(msg)

sys.path.insert(0, os.path.join(CURRENT_DIR, "vendor", "infra"))

import infra


class DanglessMalloc(infra.Instance):
    name = "dangless-malloc"

    def __init__(self, dune=None):
        self._runtime = LibDanglessMalloc(dune)

    def add_build_args(self, parser):
        parser.add_argument("--dangless-profile",
            dest="dangless_profile",
            default="release",
            help="Dangless profile to use (equivalent to passing PROFILE=value to 'make config'): 'debug' or 'release'. Defaults to 'release'."
        )

        parser.add_argument("--dangless-platform",
            dest="dangless_platform",
            default="dune",
            help="Dangless platform to use (equivalent to passing PLATFORM=value to 'make config'): 'dune' or 'rumprun'. Defaults to 'dune'."
        )

        parser.add_argument("--dangless-debug",
            action="store_true",
            dest="dangless_debug",
            default=False,
            help="Activates debug mode for Dangless. Sets PROFILE=debug regardless of --dangless-profile, and disables optimizations. Defaults to False."
        )

        parser.add_argument("--dangless-config",
            action="append",
            dest="dangless_config",
            default=[],
            help="Dangless configuration options, to be passed to 'make config' as-is."
        )

    def add_run_args(self, parser):
        pass

    def dependencies(self):
        yield self._runtime

    def configure(self, ctx):
        self._runtime.configure(ctx)

    def prepare_run(self, ctx):
        pass


class Dune(infra.Package):
    def __init__(self, variant="dune-ix"):
        self._variant = variant
        self._dir = os.path.join(
            CURRENT_DIR,
            "vendor",
            variant
        )
        self._lib_path = os.path.join(self._dir, "libdune", "libdune.a")

    @property
    def root_dir(self):
        return self._dir

    @property
    def libpath(self):
        return self._lib_path

    def ident(self):
        return self._variant

    def is_fetched(self, ctx):
        return True

    def fetch(self, ctx):
        pass

    def is_built(self, ctx):
        return os.path.exists(self._lib_path)

    def build(self, ctx):
        os.chdir(self._dir)

        infra.util.run(ctx, [
            "sudo",
            "make",
            "-j" + str(ctx.jobs)
        ])

    def is_installed(self, ctx):
        install_dir = self.path(ctx, "install")
        if not os.path.exists(install_dir):
            return False

        os.chdir(install_dir)
        return os.path.exists("libdune.a")

    def install(self, ctx):
        install_dir = self.path(ctx, "install")
        if not os.path.exists(install_dir):
            os.makedirs(install_dir)

        os.chdir(install_dir)
        shutil.copyfile(self._lib_path, "./libdune.a")

    def is_clean(self, ctx):
        return super(Dune, self).is_clean(ctx) \
            and not os.path.exists(self._lib_path)

    def clean(self, ctx):
        super(Dune, self).clean(ctx)

        os.chdir(self._dir)
        infra.util.run(ctx, [
            "sudo",
            "make",
            "clean"
        ])

    def configure(self, ctx):
        if not os.path.exists("/dev/dune"):
            raise RuntimeError("Dune cannot be configured, as the Dune kernel module is not active!")

        ctx.cflags += [
            "-no-pie"
        ]

        ctx.ldflags += [
            "-no-pie",
            "-L" + self.path(ctx, "install"),
            "-ldune",
            "-pthread",
            "-ldl"
        ]

class LibDanglessMalloc(infra.Package):
    def __init__(self, dune=None):
        self._dune = dune if dune is not None else Dune()

        self._source_dir = os.path.join(CURRENT_DIR, "sources")
        self._build_dir = os.path.join(self._source_dir, "build")

    def get_profile(self, ctx):
        return ctx.args.dangless_profile if not ctx.args.dangless_debug else "debug"

    def get_platform(self, ctx):
        return ctx.args.dangless_platform

    def _get_bin_dir(self, ctx):
        return os.path.join(self._build_dir, self.get_platform(ctx) + "_" + self.get_profile(ctx))

    def _get_bin_path(self, ctx):
        return os.path.join(self._get_bin_dir(ctx), "libdangless.a")

    def ident(self):
        return "libdangless"

    def dependencies(self):
        yield self._dune

    def is_fetched(self, ctx):
        return True

    def fetch(self, ctx):
        pass

    def is_built(self, ctx):
        return os.path.exists(self._get_bin_path(ctx))

    def build(self, ctx):
        os.chdir(self._source_dir)

        infra.util.run(ctx, [
            "make",
            "config",
            "PLATFORM=" + self.get_platform(ctx),
            "PROFILE=" + self.get_profile(ctx),
            "DUNE_ROOT=" + self._dune.root_dir
        ] + ctx.args.dangless_config)

        infra.util.run(ctx, [
            "make",
            "-j" + str(ctx.jobs)
        ])

    def is_installed(self, ctx):
        install_dir = self.path(ctx, "install")
        if not os.path.exists(install_dir):
            return False

        os.chdir(install_dir)
        return os.path.exists("libdangless.a")

    def install(self, ctx):
        install_dir = self.path(ctx, "install")
        if not os.path.exists(install_dir):
            os.makedirs(install_dir)

        os.chdir(install_dir)
        shutil.copyfile(self._get_bin_path(ctx), "./libdangless.a")

    def is_clean(self, ctx):
        return super(LibDanglessMalloc, self).is_clean(ctx) \
            and not os.path.exists(self._get_bin_path(ctx))

    def clean(self, ctx):
        super(LibDanglessMalloc, self).clean(ctx)

        os.chdir(self._source_dir)
        infra.util.run(ctx, [
            "make",
            "clean"
        ])

    def configure(self, ctx):
        if ctx.args.dangless_debug:
            # we cannot define -DDEBUG, because -DNDEBUG is unconditionally added by infra
            debug_flags = [
                "-O0",
                "-ggdb"
            ]

            ctx.cflags += debug_flags
            ctx.cxxflags += debug_flags


        common_flags = [
            "-pthread"
        ]

        ctx.cflags += common_flags
        ctx.cxxflags += common_flags

        ctx.ldflags += [
            "-L" + self.path(ctx, "install"),
            "-Wl,-whole-archive",
            "-ldangless",
            "-Wl,-no-whole-archive",
            "-pthread",
            "-ldl"
        ]

        self._dune.configure(ctx)


class DuneOnly(infra.Instance):
    name = "dune-only"

    def __init__(self, dune=None):
        self._runtime = LibDuneAutoEnter(dune)

    def add_build_args(self, parser):
        parser.add_argument("--dune-only-debug",
            action="store_true",
            dest="duneonly_debug",
            default=False,
            help="Activates debug mode for dune-autoenter, causing it to print a message to stderr upon initializing and entering Dune. Defaults to False."
        )

    def dependencies(self):
        yield self._runtime

    def configure(self, ctx):
        self._runtime.configure(ctx)

    def prepare_run(self, ctx):
        pass


class LibDuneAutoEnter(infra.Package):
    def __init__(self, dune=None):
        self._dune = dune if dune is not None else Dune()

        self._dir = os.path.join(
            CURRENT_DIR,
            "sources",
            "dune-autoenter"
        )
        self._lib_path = os.path.join(self._dir, "build", "libdune-autoenter.a")

    def ident(self):
        return "libdune-autoenter"

    def dependencies(self):
        yield self._dune

    def is_fetched(self, ctx):
        return True

    def fetch(self, ctx):
        pass

    def is_built(self, ctx):
        return os.path.exists(self._lib_path)

    def build(self, ctx):
        os.chdir(self._dir)

        extra_args = []
        if ctx.args.duneonly_debug:
            extra_args.append("PROFILE=debug")

        infra.util.run(ctx, [
            "make",
            "-j" + str(ctx.jobs),
            "DUNE_ROOT=" + self._dune.root_dir
        ] + extra_args)

    def is_installed(self, ctx):
        install_dir = self.path(ctx, "install")
        if not os.path.exists(install_dir):
            return False

        os.chdir(install_dir)
        return os.path.exists("libdune-autoenter.a")

    def install(self, ctx):
        install_dir = self.path(ctx, "install")
        if not os.path.exists(install_dir):
            os.makedirs(install_dir)

        os.chdir(install_dir)
        shutil.copyfile(self._lib_path, "./libdune-autoenter.a")

    def is_clean(self, ctx):
        return super(LibDuneAutoEnter, self).is_clean(ctx) \
            and not os.path.exists(self._lib_path)

    def clean(self, ctx):
        super(LibDuneAutoEnter, self).clean(ctx)

        os.chdir(self._dir)
        infra.util.run(ctx, [
            "make",
            "clean"
        ])

    def configure(self, ctx):
        ctx.ldflags += [
            "-L" + self.path(ctx, "install"),
            "-ldune-autoenter",
            "-pthread",
            "-ldl"
        ]

        self._dune.configure(ctx)


class Baseline(infra.Instance):
    name = "baseline"

    def __init__(self):
        pass

    def dependencies(self):
        return
        yield

    def configure(self, ctx):
        pass

    def prepare_run(self, ctx):
        pass


if __name__ == "__main__":
    setup = infra.Setup(__file__)

    # llvm = infra.packages.LLVM(
    #     version = "6.0.0",
    #     compiler_rt = False,
    #     patches = []
    # )
    # setup.add_instance(infra.instances.Clang(llvm))

    dune = Dune()
    setup.add_instance(Baseline())
    setup.add_instance(DuneOnly(dune))
    setup.add_instance(DanglessMalloc(dune))

    setup.add_target(infra.targets.SPEC2006(
        source = os.path.join(CURRENT_DIR, "vendor", "spec2006"),
        source_type = "installed"
    ))

    setup.main()
