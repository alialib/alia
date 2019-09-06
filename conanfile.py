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
        requires = requires + ("boost_any/1.68.0@bincrafters/stable",
                               "boost_lexical_cast/1.68.0@bincrafters/stable")

    def imports(self):
        dest = os.getenv("CONAN_IMPORT_PATH", "bin")
        self.copy("*.dll", dst=dest, src="bin")
        self.copy("*.dylib*", dst=dest, src="lib")
