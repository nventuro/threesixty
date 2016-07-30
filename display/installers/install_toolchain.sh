#!/bin/bash 

# Install dependencies
sudo apt-get install -y lib32ncurses5

# Download toolchain
cd ~
wget https://launchpad.net/gcc-arm-embedded/5.0/5-2016-q2-update/+download/gcc-arm-none-eabi-5_4-2016q2-20160622-linux.tar.bz2
TOOLCHAIN_ZIPPED_FILE=gcc-arm-none-eabi.tar.bz2
mv gcc-arm-none-eabi* $TOOLCHAIN_ZIPPED_FILE

# Install
echo "Installing"
cd /usr/local
sudo tar xjf ~/$TOOLCHAIN_ZIPPED_FILE

# Delete temporary files
echo "Removing temporary files"
cd ~
rm $TOOLCHAIN_ZIPPED_FILE

echo "Installation complete"