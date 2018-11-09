from conans import ConanFile, CMake
import os


class CradleConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    requires = \
        "catch2/2.3.0@bincrafters/stable", \
        "FakeIt/2.0.4@gasuketsu/stable"
    generators = "cmake"
    default_options = \
        "FakeIt:integration=catch", \
        "*:shared=False"
    if "APPVEYOR" not in os.environ:
        # AppVeyor provides Boost directly (and seems to have trouble building
        # it through Conan), so omit Boost if we're running on AppVeyor.
        requires = requires + ("boost/1.68.0@conan/stable", )
        default_options = default_options + \
           ("boost:without_atomic=True",
            "boost:without_chrono=True",
            "boost:without_container=True",
            "boost:without_context=True",
            "boost:without_coroutine=True",
            "boost:without_coroutine2=True",
            "boost:without_graph=True",
            "boost:without_graph_parallel=True",
            "boost:without_log=True",
            "boost:without_math=True",
            "boost:without_mpi=True",
            "boost:without_serialization=True",
            "boost:without_signals=True",
            "boost:without_test=True",
            "boost:without_timer=True",
            "boost:without_type_erasure=True",
            "boost:without_wave=True")

    def imports(self):
        dest = os.getenv("CONAN_IMPORT_PATH", "bin")
        self.copy("*.dll", dst=dest, src="bin")
        self.copy("*.dylib*", dst=dest, src="lib")
