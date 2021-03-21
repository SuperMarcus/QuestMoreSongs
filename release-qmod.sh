#!/bin/bash

TMP_DIR=`mktemp -d -t questmod`

# Build standalone package
rm -f MoreSongs.qmod MoreSongs.standalone.qmod
cp mod.standalone.json "${TMP_DIR}/mod.json"
zip -j MoreSongs.standalone.qmod "${TMP_DIR}/mod.json"
find ./libs/arm64-v8a \
  ! -name 'libmodloader.so' \
  -type f -exec zip -j MoreSongs.standalone.qmod {} +

# Build normal package
zip -j MoreSongs.qmod mod.json ./libs/arm64-v8a/libmoresongs.so

# Cleanup
rm -rf "${TMP_DIR}"
