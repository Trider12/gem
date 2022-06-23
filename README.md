# gem (WIP)

## Description
TODO

## Compilation
### Requirements:
* [`CMake`](https://cmake.org/)
* [`Ninja`](https://ninja-build.org/)
* [`Python 3.x`](https://www.python.org/downloads/)
* Compiler supporting `C++17` or higher

### Commands
Clone and update submodules:
```
git clone https://github.com/Trider12/gem
cd gem
git submodule update --init
```
Build third party libraries:
```
python3 ./tools/scripts/vcpkg_install_third_parties.py
```
Build sources:
```
cmake -Bbuild -DCMAKE_BUILD_TYPE=Release -GNinja
cmake --build build --config Release --target all
```
