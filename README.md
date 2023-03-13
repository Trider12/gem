# gem (WIP)

**gem** is a cross-platform browser for the [Gemini protocol](https://en.wikipedia.org/wiki/Gemini_(protocol)).

Platforms: Windows, Linux (X11), MacOS, Android (WIP)

Example URLs (***don't work*** in a "normal" web browser):
* gemini://gemini.circumlunar.space/
* gemini://geminispace.info/
* gemini://warmedal.se/~antenna/

## Compilation
### Requirements:
* [`CMake`](https://cmake.org/)
* [`Ninja`](https://ninja-build.org/)
* [`Python 3.x`](https://www.python.org/downloads/)
* Compiler supporting `C++17` or higher

### Commands:
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
