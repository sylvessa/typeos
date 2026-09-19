#!/bin/bash
set -e

cyan="\033[0;36m"
green="\033[0;32m"
yellow="\033[1;33m"
red="\033[0;31m"
reset="\033[0m"

binv=2.47
gccv=16.2.0
target=i386-elf
prefix="$(cd "$(dirname "$0")" && pwd)/bin"
prefix=$(realpath "$prefix")
stage="$(cd "$(dirname "$0")" && pwd)/toolchain"

export PATH="$prefix:$PATH"

if command -v $target-gcc >/dev/null 2>&1 && command -v $target-ld >/dev/null 2>&1; then
	echo -e "${green}toolchain already built${reset}"
	echo -e "${yellow}run make${reset}"
	exit 0
fi

echo -e "${cyan}make dirs${reset}"
mkdir -p "$stage"
mkdir -p "$prefix"

echo -e "${cyan}install deps${reset}"
if command -v apt >/dev/null 2>&1; then
	sudo apt update
	sudo apt install wget build-essential bison flex libgmp-dev libmpc-dev libmpfr-dev texinfo nasm qemu-system -y
elif command -v pacman >/dev/null 2>&1; then
	sudo pacman -S --needed wget base-devel bison flex gmp libmpc mpfr texinfo nasm qemu-full --noconfirm
else
	echo -e "${yellow}unsupported distro, install deps manually${reset}"
fi

echo -e "${cyan}download src${reset}"
cd "$stage"
wget -q https://ftp.wayne.edu/gnu/binutils/binutils-$binv.tar.gz
wget -q https://ftp.wayne.edu/gnu/gcc/gcc-$gccv/gcc-$gccv.tar.gz

echo -e "${cyan}extract src${reset}"
tar -xzf binutils-$binv.tar.gz
tar -xzf gcc-$gccv.tar.gz

echo -e "${cyan}build binutils${reset}"
mkdir -p build-binutils
cd build-binutils
../binutils-$binv/configure --target=$target --prefix=$prefix --with-sysroot --disable-nls --disable-werror
make -j"$(nproc)" V=1
make install
cd ..

echo -e "${cyan}fetch gcc deps${reset}"
cd gcc-$gccv
./contrib/download_prerequisites
cd ..

echo -e "${cyan}build gcc (c only)${reset}"
mkdir -p build-gcc
cd build-gcc
../gcc-$gccv/configure --target=$target --prefix=$prefix --disable-nls --enable-languages=c --without-headers
make all-gcc -j"$(nproc)" V=1
make install-gcc
cd ..

if [ ! -f "$prefix/bin/i386-elf-gcc" ]; then
	echo -e "${red}i386-elf-gcc not found in $prefix/bin${reset}"
	ls -l "$prefix/bin"
	exit 1
fi

echo -e "${cyan}cleanup${reset}"
cd "$(dirname "$0")"
rm -rf "$stage"

echo
echo -e "${green}toolchain built${reset}"
echo -e "${yellow}run make${reset}"