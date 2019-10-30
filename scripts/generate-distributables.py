from pathlib import Path
import re


def generate_single_header(single_header_path, source_dir, module):
    with open(single_header_path, 'w') as output:
        with open('LICENSE.txt') as f:
            lines = f.readlines()
            for line in lines:
                output.write(('// ' + line).strip() + '\n')

        output.write('\n')

        output.write('#ifndef ALIA_' + module.upper() + '_HPP\n')
        output.write('#define ALIA_' + module.upper() + '_HPP\n')

        header_states = {}

        def add_header_file(path):
            if path in header_states:
                if header_states[path] == 'done':
                    return
                else:
                    raise Exception(path + ': cyclical #includes detected')
            else:
                header_states[path] = 'processing'

            with open(path) as f:
                lines = f.readlines()

            # Check that the last (non-empty) line is an #endif and remove it.
            # (This is the end of the include guard, but we can't recognize it based
            # solely on its content.)
            while lines[-1].strip() == "":
                del lines[-1]
            if lines[-1].strip() != "#endif":
                raise Exception(path + ': header files must end with #endif')
            del lines[-1]

            for line in lines:
                # Recursively process #include'd files so that they're inlined.
                include = re.match(r'^\s*#include <(alia/[a-z0-9/_\.]+)>',
                                   line)
                if include:
                    add_header_file('src/' + include.group(1))
                    continue

                # Strip out include guards.
                if re.match(r'^\s*#(ifndef|define) ALIA_[A-Z0-9_]+_HPP', line):
                    continue

                output.write(line)

            header_states[path] = 'done'

        def add_source_file(path):
            with open(path) as f:
                lines = f.readlines()

            for line in lines:
                # All internal headers should've already been included.
                include = re.match(r'^\s*#include <(alia/[a-z0-9/_\.]+)>',
                                   line)
                if include:
                    continue

                output.write(line)

        header_paths = Path(source_dir).glob('**/*.hpp')
        for path in header_paths:
            add_header_file(str(path))

        output.write('#ifdef ALIA_IMPLEMENTATION\n')

        source_paths = Path(source_dir).glob('**/*.cpp')
        for path in source_paths:
            add_source_file(str(path))

        output.write('#endif\n')

        output.write('#endif\n')


generate_single_header('alia.hpp', 'src/alia', 'core')
