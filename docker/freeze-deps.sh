#/bin/bash -eux
#
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
#
# Freeze Python dependencies used across mutiple CI images.
# Generally speaking, the CI images used in TVM can be grouped by underlying
# architecture. Here, we specifically mean any piece of the architecture which
# may contribute to pip filtering, such as:
# - CPU architecture family (i386, x86_64, arm)
# - Operating System
#
# CI images which belong to the same group should use the same exact Python
# packages between them to minimize divergence in the CI results. The exact
# assignments of CI images to groups can be found in the tlcpack/tlc-pack
# repository. This script is included in the TVM repository along with the
# automation which consumes it output in building the CI images.
#
# To use this script, choose a CI image from the group. This script will freeze
# dependencies in that container and modify pyproject.toml and poetry.lock in the
# tvm root directory. These modified files should then be placed into the tvm
# directory when building the other containers in the same group.

set -eux

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <ci_image_name> <output_base>"
    exit 2
fi

CI_IMAGE_NAME="$1"
OUTPUT_BASE="$2"

SCRIPT_DIR="$(dirname "$0")"

base_image=$(cat "${SCRIPT_DIR}/Dockerfile.$1" | grep '^FROM ' | sed -E 's/^FROM[ ]+(.+)$/\1/g')
if [ -z "${base_image}" ]; then
    echo "Failed to extract base image from Dockerfile.$1"
    echo "Did not find a line like 'FROM abc'"
    exit 2
fi

# Now we need to create a copy of the Dockerfile that runs only up to ubuntu_install_python_package.sh.
cat "${SCRIPT_DIR}/Dockerfile.${CI_IMAGE_NAME}" | \
    sed -n '/RUN .*\/ubuntu_install_python_package.sh/q; p' \
        >"${SCRIPT_DIR}/Dockerfile.${CI_IMAGE_NAME}-freeze-deps"

#"${SCRIPT_DIR}/build.sh" "${CI_IMAGE_NAME}-freeze-deps"
"${SCRIPT_DIR}/bash.sh" -it "tvm.${CI_IMAGE_NAME}-freeze-deps:latest" docker/freeze-deps-in-container.sh "${OUTPUT_BASE}"
