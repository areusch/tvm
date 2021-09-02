#!/bin/bash
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

set -e
set -u
set -x
set -o pipefail

if [ $# -eq 0 ]; then
    echo "usage: $0 <requirements_glob> [<requirements_glob> ...]"
    exit 2
fi

SCRIPT_DIR="$(pwd)/$(dirname "$0")"

pip3 install -U pip

# NOTE: dependencies already handled by poetry.
PIP_ARGS=( pip3 install --no-deps )
SCRIPT_DIR="$(dirname "$0")"
for piece in "$@"; do
    piece_files=$(ls -1 $piece)
    for file_name in ${piece_files}; do
        PIP_ARGS+=( -r "${file_name}" )
    done
done

"${PIP_ARGS[@]}"
