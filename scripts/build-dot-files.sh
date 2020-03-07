#!/bin/bash
set -e
if [ $(basename $(pwd)) != "www" ]; then
    echo "This script must be run from the www directory!"
    exit 1
fi
dot -Tsvg -o data-graph.svg data-graph.dot
dot -Tsvg -o alia-flow.svg reactive-flow.dot
sed -i 's/label="alia controller function"; style=dashed;/style=invis;/' \
    reactive-flow.dot
dot -Tsvg -o reactive-flow.svg reactive-flow.dot
rm *.dot
