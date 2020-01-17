from conans import ConanFile, CMake
import os


class AliaConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    requires = ("Catch/1.12.1@bincrafters/stable", )
    generators = "cmake"
    default_options = "*:shared=False"

    def imports(self):
        dest = os.getenv("CONAN_IMPORT_PATH", "bin")
        self.copy("*.dll", dst=dest, src="bin")
        self.copy("*.dylib*", dst=dest, src="lib")
