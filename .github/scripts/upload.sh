#!/bin/bash

set -ex

cd "$(dirname "$0")/../.."

tag=$1

if gh release list | grep $tag; then
    archive_filename="xla_extension-$XLA_TARGET_PLATFORM-$XLA_TARGET.tar.gz"

    # Uploading is the final action after several hour long build,
    # so in case of any temporary network failures we want to retry
    # a number of times
    for i in {1..10}; do
        gh release upload --clobber $tag "$archive_filename" && break
        echo "Upload failed, retrying in 30s"
        sleep 30
    done
else
    echo "::error::Release $tag not found"
    exit 1
fi
