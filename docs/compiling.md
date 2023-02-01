# Compiling

### Linux

##### Ubuntu

```
sudo apt-get install -y build-essential cmake git pkg-config \
    libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev \
    mesa-common-dev libgtk-3-dev libfreetype-dev libmpv-dev
git clone https://github.com/tsl0922/ImPlay.git
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=RELEASE ..
cmake --build . --target package
```

This will build a deb package.

### macOS

Install [Homebrew](https://brew.sh), and run:

```
brew install cmake git mpv freetype2 pkg-config
git clone https://github.com/tsl0922/ImPlay.git
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=RELEASE ..
cmake --build . --target package
```

This will build a DMG file.

### Windows

Install [msys2](https://www.msys2.org), and open the `MSYS2 MINGW64` shell:

```
pacman -S base-devel git p7zip mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-freetype
git clone https://github.com/tsl0922/ImPlay.git
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=RELEASE -DUSE_OPENGL_ES2=ON ..
cmake --build . --target package
```

This will build a MSI installer and a portable ZIP.