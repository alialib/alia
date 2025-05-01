from pathlib import Path
import re

input_dir = Path("docs")
output_dir = Path("www")
cpp_dir = Path("src/demos")


def embed_file(output, path, fragment):
    with open(str(cpp_dir / path)) as f:
        lines = f.readlines()
    output.write('```cpp\n')
    inside_fragment = False
    for line in lines:
        if line.strip() == '/// [' + fragment + ']':
            inside_fragment = not inside_fragment
        else:
            if inside_fragment:
                output.write(line)
    output.write('```\n')


def copy_file(path):
    with open(str(output_dir / path), 'w') as output:
        with open(str(input_dir / path)) as f:
            lines = f.readlines()
        for line in lines:
            match = re.match(
                r'^\[source\]\(([a-z0-9/_\-\.]+) \'\:include \:fragment\=([a-z0-9/_\-\.]+)\'\)',
                line)
            if match:
                embed_file(output, match.group(1), match.group(2))
            else:
                output.write(line)


source_paths = Path(input_dir).glob('**/*.md')
for path in source_paths:
    copy_file(path.relative_to(input_dir))
