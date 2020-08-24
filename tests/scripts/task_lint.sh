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
set -o pipefail

cleanup()
{
  rm -rf /tmp/$$.*
}
trap cleanup 0


echo "Check file types..."
python3 tests/lint/check_file_type.py

echo "Check ASF license header..."
tests/lint/check_asf_header.sh

echo "Check codestyle of c++ code..."
tests/lint/cpplint.sh

echo "clang-format check..."
tests/lint/clang_format.sh

echo "Check codestyle of python code..."
tests/lint/pylint.sh
echo "Check codestyle of jni code..."
tests/lint/jnilint.sh

echo "Check documentations of c++ code..."
tests/lint/cppdocs.sh
