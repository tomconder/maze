#!/bin/sh

pushd . > /dev/null
cd "../sponge/src"
echo "Running clang-format in $(pwd)"
find . -regex '.*\.[chm]p*' -exec clang-format -i {} \;
popd > /dev/null

pushd . > /dev/null
cd "../game/src"
echo "Running clang-format in $(pwd)"
find . -regex '.*\.[chm]p*' -exec clang-format -i {} \;
popd > /dev/null

echo "clang-format complete!"
