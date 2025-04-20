#!/bin/sh

echo "Running clang-format"
find ../../sponge/src ../../game/src -regex '.*\.[chm]p*' -exec clang-format -i --style=file {} \;

echo "clang-format complete!"
