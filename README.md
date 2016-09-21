# Building insight under Ubuntu 16.04 for ARM targets

## Install build dependencies
`sudo apt-get install texinfo bison flex tcl8.6 tcl8.6-dev tk8.6 tk8.5-dev`

## Clone repository
`git clone --recursive https://github.com/yaminov/insight`

This clones insigth GUI source files and binutils-gdb submodule. The GUI part of insight is now maintained outside the GDB tree. 
GDB sources are published at git://sourceware.org/git/binutils-gdb.git

## Configure for building
`cd insight`

`autoconf`

`./configure --prefix=path-to-installation --target=arm-none-eabi`

## Build and install
`make`

`make install`
