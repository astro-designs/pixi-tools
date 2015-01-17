#!/bin/sh

set -x # print each command
set -e # exit immediately on error

mkdir -p ~/pixi-setup
cd ~/pixi-setup

# download archive gpg public key
rm -f archive.key
wget http://zaphod.astro-designs.net/raspbian/archive.key
# Verify the contents
echo "43409bee157164bb9c2d221797aad573c5362d1f *archive.key" | sha1sum -c

# add key to apt, so that debs can be verified
apt-key add archive.key

# Add the astro-designs repository by downloading the list file into the right directory
rm -f astro-designs.list
wget http://zaphod.astro-designs.net/raspbian/astro-designs.list
# Verify the contents
echo "d425554a30625a354dc46305eae0a8faa9e98741 *astro-designs.list" | sha1sum -c
cp astro-designs.list /etc/apt/sources.list.d/

# Update the pi's package list
apt-get update

# Install pixi-tools
apt-get install pixi-tools

# Configure a default FPGA image... First make a directory:
mkdir -p /etc/pixi-tools
# Make a symbolic link to the default FPGA image (use pixi_1vx if you have a version one pixi).
ln -s /usr/share/pixi-tools/fpga/pixi_2vx/pixi.bin /etc/pixi-tools
