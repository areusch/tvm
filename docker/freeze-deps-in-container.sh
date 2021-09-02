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

set -eux

if [ "$#" -ne 1 ]; then
    echo "usage: $0 <output_base>"
    exit 2
fi

SCRIPT_DIR="$(pwd)/$(dirname "$0")"
DEPS_VENV="${SCRIPT_DIR}/poetry-venv"
python3 -mvenv "${DEPS_VENV}"

# NOTE: install pip separately to work around poetry bug.
#"${DEPS_VENV}/bin/pip" install -U pip
#"${DEPS_VENV}/bin/pip" install -U poetry


cd "${SCRIPT_DIR}"
cd "$(git rev-parse --show-toplevel)"
#python3 python/gen_requirements.py --modify-pyproject-toml pyproject.toml
export LC_ALL=C.UTF-8
#"${DEPS_VENV}/bin/poetry" lock -vv

python3 python/export_constraints.py --output-base "$1"
