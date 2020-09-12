#!/bin/bash -e

TVM_HOME="$1"

# Zephyr
pip3 install --user -U west
echo 'export PATH=$HOME/.local/bin:"$PATH"' >> ~/.profile
source ~/.profile
echo PATH=$PATH
west init --mr v2.3.0 ~/zephyr
cd ~/zephyr
west update
west zephyr-export

cd ~
echo "Downloading zephyr SDK..."
wget --no-verbose https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.11.3/zephyr-sdk-0.11.3-setup.run
chmod +x zephyr-sdk-0.11.3-setup.run
./zephyr-sdk-0.11.3-setup.run -- -d ~/zephyr-sdk -y

# TVM
# NOTE: TVM is presumed to be mounted already by Vagrantfile.
cd "${TVM_HOME}"

if [ -e build ]; then
    mv build build-moved-aside-$(date +%Y-%m-%dT%H-%M-%S)
fi
mkdir build
cp cmake/config.cmake build/
cd build
sed -i 's/USE_MICRO OFF/USE_MICRO ON/' config.cmake
sed -i 's/USE_LLVM OFF/USE_LLVM ON/' config.cmake
cmake ..
make -j4

# Poetry
curl -sSL https://raw.githubusercontent.com/python-poetry/poetry/master/get-poetry.py | python3
cat <<EOF >>~/.bashrc
source $HOME/.poetry/env
export ZEPHYR_BASE=$HOME/zephyr/zephyr
EOF
source $HOME/.poetry/env

poetry install
poetry run pip3 install -r ~/zephyr/zephyr/scripts/requirements.txt
