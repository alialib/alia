from pathlib import Path
import re
import os
import datetime


def generate_single_header(single_header_path, source_dir, module):
    with open(single_header_path, 'w') as output:
        with open('LICENSE.txt') as f:
            lines = f.readlines()
            for line in lines:
                output.write(('// ' + line).strip() + '\n')

        output.write('\n')

        timestamp = datetime.datetime.utcnow().replace(
            tzinfo=datetime.timezone.utc).astimezone().replace(
                microsecond=0).isoformat()
        if 'TRAVIS_TAG' in os.environ:
            version = "{} ({})".format(os.environ['TRAVIS_TAG'],
                                       os.environ['TRAVIS_COMMIT'])
        elif 'TRAVIS_BRANCH' in os.environ:
            version = "{} branch ({})".format(os.environ['TRAVIS_BRANCH'],
                                              os.environ['TRAVIS_COMMIT'])
        else:
            version = "(local)"
        output.write('// alia.hpp - {} - generated {}\n'.format(
            version, timestamp))
        output.write('\n')

        output.write('#ifndef ALIA_' + module.upper() + '_HPP\n')
        output.write('#define ALIA_' + module.upper() + '_HPP\n')

        header_states = {}

        included_standard_headers = set()

        def add_external_include(path):
            if path not in included_standard_headers:
                output.write('#include <{}>\n'.format(path))
                included_standard_headers.add(path)

        def add_header_file(path):
            path = path.replace('\\', '/')
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
            # (This is the end of the include guard, but we can't recognize it
            # based solely on its content.)
            while lines[-1].strip() == "":
                del lines[-1]
            if lines[-1].strip() != "#endif":
                raise Exception(path + ': header files must end with #endif')
            del lines[-1]

            for line in lines:
                # Recursively process #include'd alis files so that they're
                # inlined.
                internal_include = re.match(
                    r'^\s*#include <(alia/[a-z0-9/_\.]+)>', line)
                if internal_include:
                    add_header_file('src/' + internal_include.group(1))
                    continue

                # For external includes, only add it if it's not already there.
                external_include = re.match(r'^\s*#include <([a-z0-9/_\.]+)>',
                                            line)
                if external_include:
                    add_external_include(external_include.group(1))
                    continue

                # Strip out include guards.
                if re.match(r'^\s*#(ifndef|define) ALIA_[A-Z0-9_]+_HPP', line):
                    continue

                output.write(line)

            header_states[path] = 'done'

        def add_source_file(path):
            path = path.replace('\\', '/')

            with open(path) as f:
                lines = f.readlines()

            for line in lines:
                # All internal headers should've already been included.
                internal_include = re.match(
                    r'^\s*#include <(alia/[a-z0-9/_\.]+)>', line)
                if internal_include:
                    continue

                # For external includes, only add it if it's not already there.
                external_include = re.match(r'^\s*#include <([a-z0-9/_\.]+)>',
                                            line)
                if external_include:
                    add_external_include(external_include.group(1))
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
