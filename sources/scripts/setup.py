#!/usr/bin/env python3
# PYTHON_ARGCOMPLETE_OK
import sys
import os.path
import shutil

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
        parser.add_argument("--dangless-debug",
            action="store_true",
            dest="dangless_debug",
            default=False,
            help="Configures Dangless to be built in debug mode instead of release. Defaults to False."
        )

        parser.add_argument("--dangless-config",
            action="append",
            dest="dangless_config",
            default=[],
            help="Add an extra Dangless CMake configuration option, each to be passed to 'cmake' prefixed with '-D'"
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
        self.dune = dune if dune is not None else Dune()

        self.source_dir = os.path.join(CURRENT_DIR, "sources")
        self.build_dir = os.path.join(self.source_dir, "build")
        self.bin_name = "libdangless_malloc.a"
        self.bin_path = os.path.join(self.build_dir, self.bin_name)

    def ident(self):
        return "libdangless_malloc"

    def dependencies(self):
        yield self.dune

    def is_fetched(self, ctx):
        return True

    def fetch(self, ctx):
        pass

    def is_built(self, ctx):
        return os.path.exists(self.bin_path)

    def build(self, ctx):
        if not os.path.exists(self.build_dir):
            os.makedirs(self.build_dir)

        os.chdir(self.build_dir)

        infra.util.run(ctx,
            [
                "cmake",
                "-D CMAKE_BUILD_TYPE=" + ("Debug" if ctx.args.dangless_debug else "RelWithDebInfo"),
                "-D DUNE_ROOT=" + self.dune.root_dir
            ] + [
                "-D " + option \
                    for option in ctx.args.dangless_config
            ] + [
                self.source_dir
            ]
        )

        infra.util.run(ctx, [
            "cmake",
            "--build",
            ".",
            "--",
            "-j{}".format(ctx.jobs)
        ])

    def is_installed(self, ctx):
        install_dir = self.path(ctx, "install")
        if not os.path.exists(install_dir):
            return False

        os.chdir(install_dir)
        return os.path.exists(self.bin_name)

    def install(self, ctx):
        install_dir = self.path(ctx, "install")
        if not os.path.exists(install_dir):
            os.makedirs(install_dir)

        os.chdir(install_dir)
        shutil.copyfile(self.bin_path, "./" + self.bin_name)

    def is_clean(self, ctx):
        return super(LibDanglessMalloc, self).is_clean(ctx) \
            and not os.path.exists(self.bin_path)

    def clean(self, ctx):
        super(LibDanglessMalloc, self).clean(ctx)

        os.chdir(self.source_dir)
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
            "-ldangless_malloc",
            "-Wl,-no-whole-archive",
            "-pthread",
            "-ldl"
        ]

        self.dune.configure(ctx)


class DuneOnly(infra.Instance):
    name = "dune-only"

    def __init__(self, dune=None):
        self.runtime = LibDuneAutoEnter(dune)

    def add_build_args(self, parser):
        parser.add_argument("--dune-only-debug",
            action="store_true",
            dest="duneonly_debug",
            default=False,
            help="Activates debug mode for dune-autoenter, causing it to print a message to stderr upon initializing and entering Dune. Defaults to False."
        )

    def dependencies(self):
        yield self.runtime

    def configure(self, ctx):
        self.runtime.configure(ctx)

    def prepare_run(self, ctx):
        pass


class LibDuneAutoEnter(infra.Package):
    def __init__(self, dune=None):
        self.dune = dune if dune is not None else Dune()

        self.source_dir = os.path.join(
            CURRENT_DIR,
            "dune-autoenter"
        )
        self.build_dir = os.path.join(self.source_dir, "build")
        self.lib_name = "libdune-autoenter.a"
        self.lib_path = os.path.join(self.build_dir, self.lib_name)

    def ident(self):
        return "libdune-autoenter"

    def dependencies(self):
        yield self.dune

    def is_fetched(self, ctx):
        return True

    def fetch(self, ctx):
        pass

    def is_built(self, ctx):
        return os.path.exists(self.lib_path)

    def build(self, ctx):
        if not os.path.exists(self.build_dir):
            os.makedirs(self.build_dir)

        os.chdir(self.build_dir)

        infra.util.run(ctx, [
            "cmake",
            "-D CMAKE_BUILD_TYPE=" + ("Debug" if ctx.args.duneonly_debug else "RelWithDebInfo"),
            "-D DUNE_ROOT=" + self.dune.root_dir,
            self.source_dir
        ])

        infra.util.run(ctx, [
            "make",
            "-j" + str(ctx.jobs)
        ])

    def is_installed(self, ctx):
        install_dir = self.path(ctx, "install")
        if not os.path.exists(install_dir):
            return False

        os.chdir(install_dir)
        return os.path.exists(self.lib_name)

    def install(self, ctx):
        install_dir = self.path(ctx, "install")
        if not os.path.exists(install_dir):
            os.makedirs(install_dir)

        os.chdir(install_dir)
        shutil.copyfile(self.lib_path, "./" + self.lib_name)

    def is_clean(self, ctx):
        return super(LibDuneAutoEnter, self).is_clean(ctx) \
            and not os.path.exists(self.lib_path)

    def clean(self, ctx):
        super(LibDuneAutoEnter, self).clean(ctx)

        os.chdir(self.source_dir)
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

        self.dune.configure(ctx)


class BaselineReportTLB(infra.Instance):
    name = "baseline-report-tlb"

    def __init__(self):
        self.runtime = LibPerfTLBReport()

    def add_build_args(self, parser):
        pass

    def dependencies(self):
        yield self.runtime

    def configure(self, ctx):
        self.runtime.configure(ctx)

    def prepare_run(self, ctx):
        pass


class LibPerfTLBReport(infra.Package):
    def __init__(self, dune=None):
        self.source_dir = os.path.join(
            CURRENT_DIR,
            "libperf-tlb-report"
        )
        self.build_dir = os.path.join(self.source_dir, "build")
        self.lib_name = "libperf-tlb-report.a"
        self.lib_path = os.path.join(self.build_dir, self.lib_name)

    def ident(self):
        return "libperf-tlb-report"

    def dependencies(self):
        return
        yield

    def is_fetched(self, ctx):
        return True

    def fetch(self, ctx):
        pass

    def is_built(self, ctx):
        return os.path.exists(self.lib_path)

    def build(self, ctx):
        if not os.path.exists(self.build_dir):
            os.makedirs(self.build_dir)

        os.chdir(self.build_dir)

        infra.util.run(ctx, [
            "cmake",
            "-D CMAKE_BUILD_TYPE=RelWithDebInfo",
            "-D STANDALONE=True",
            self.source_dir
        ])

        infra.util.run(ctx, [
            "make",
            "-j" + str(ctx.jobs)
        ])

    def is_installed(self, ctx):
        install_dir = self.path(ctx, "install")
        if not os.path.exists(install_dir):
            return False

        os.chdir(install_dir)
        return os.path.exists(self.lib_name)

    def install(self, ctx):
        install_dir = self.path(ctx, "install")
        if not os.path.exists(install_dir):
            os.makedirs(install_dir)

        os.chdir(install_dir)
        shutil.copyfile(self._lib_path, "./" + self.lib_name)

    def is_clean(self, ctx):
        return super(LibPerfTLBReport, self).is_clean(ctx) \
            and not os.path.exists(self.lib_path)

    def clean(self, ctx):
        super(LibPerfTLBReport, self).clean(ctx)

        os.chdir(self.build_dir)
        infra.util.run(ctx, [
            "make",
            "clean"
        ])

    def configure(self, ctx):
        ctx.ldflags += [
            "-L" + self.path(ctx, "install"),
            "-Wl,--whole-archive",
            "-lperf-tlb-report",
            "-Wl,--no-whole-archive"
        ]


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

    setup.add_instance(Baseline())
    setup.add_instance(BaselineReportTLB())

    dune = Dune()
    setup.add_instance(DuneOnly(dune))
    setup.add_instance(DanglessMalloc(dune))

    setup.add_target(infra.targets.SPEC2006(
        source = os.path.join(CURRENT_DIR, "vendor", "spec2006"),
        source_type = "installed"
    ))

    setup.main()
