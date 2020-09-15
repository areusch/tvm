#!/bin/bash -e

if [ $# -ne 1 ]; then
    echo "usage: $0 <version>"
    exit 2
fi

cd "$(dirname $0)"
if [ ! -e api-token ]; then
    echo "must create a file named 'api-token' in $(pwd)"
    echo "file contents:"
    echo api_token = "<VAGRANT_CLOUD_API_TOKEN>"
    exit 2
fi

ALL_PROVIDERS=( virtualbox )
for provider in "${ALL_PROVIDERS[@]}"; do
    set -x
    packer build -var-file=api-token -var "provider=${provider}" -var "version=$1" packer.hcl
done
