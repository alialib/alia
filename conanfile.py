from conans import ConanFile, CMake
import os


class AliaConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    requires = ("Catch/1.12.1@bincrafters/stable", )
    generators = "cmake"
    default_options = "*:shared=False"

    # AppVeyor provides Boost directly (and seems to have trouble building it
    # through Conan), so omit Boost if we're running on AppVeyor.
    if "APPVEYOR" not in os.environ:
        requires = requires + ("boost_any/1.68.0@bincrafters/stable",
                               "boost_lexical_cast/1.68.0@bincrafters/stable")

    def imports(self):
        dest = os.getenv("CONAN_IMPORT_PATH", "bin")
        self.copy("*.dll", dst=dest, src="bin")
        self.copy("*.dylib*", dst=dest, src="lib")
