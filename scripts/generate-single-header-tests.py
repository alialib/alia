from pathlib import Path
import re

input_dir = Path("unit_tests")
output_dir = Path("single_header_tests")


def copy_file(path):
    (output_dir / path).parent.mkdir(parents=True, exist_ok=True)
    with open(str(output_dir / path), 'w') as output:
        if str(path) == "runner.cpp":
            output.write('#define ALIA_IMPLEMENTATION\n')
            output.write('#include "alia.hpp"\n')

        with open(str(input_dir / path)) as f:
            lines = f.readlines()

        for line in lines:
            # Replace alia headers with the single header.
            if re.match(r'^\s*#include <(alia/[a-z0-9/_\.]+)>', line):
                output.write('#include "alia.hpp"\n')
            else:
                output.write(line)


source_paths = Path(input_dir).glob('**/*.?pp')
for path in source_paths:
    copy_file(path.relative_to(input_dir))
