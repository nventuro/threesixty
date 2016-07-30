#!/bin/bash 

# Download openocd
cd ~
git clone http://git.code.sf.net/p/openocd/code openocd
cd openocd

# Configure, compile and install
echo "Installing"
./bootstrap
./configure --enable-maintainer-mode --enable-ti-icdi
make
sudo make install

# Delete temporary files
echo "Removing temporary files"
cd ..
rm -rf openocd

echo "Installation complete"