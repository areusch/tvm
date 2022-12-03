#!/bin/bash -e

set -eux
(cd build-gpu && cmake -GNinja .. && ninja) &&

(cd test-data && make)
python3 tests/python/relay/aot/test_cpp_aot.py -k 'test_conv2d[cuda-False]' -s -v
