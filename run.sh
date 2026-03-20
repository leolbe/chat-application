#!/usr/bin/env bash

DIR="$(dirname "$0")"
BUILDDIR="$DIR/build"

if [ -e "$BUILDDIR" ]; then
    if [ ! -d "$BUILDDIR" ]; then
        echo "'build' already exists!"
        exit 1
    fi
else
    cmake -B "$BUILDDIR"
fi

cmake --build "$BUILDDIR"

"$BUILDDIR/chat_application"
