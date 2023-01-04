#!/bin/sh

cd "$(dirname $0)/../sponge/src"
echo "Running clang-format in $(pwd)"
find . -regex '.*\.[chm]p*' -exec clang-format -i {} \;

cd "$(dirname $0)/../game/src"
echo "Running clang-format in $(pwd)"
find . -regex '.*\.[chm]p*' -exec clang-format -i {} \;

echo "clang-format complete!"
