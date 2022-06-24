#!/usr/bin/env python3

import os
import subprocess

os.chdir(os.path.dirname(os.path.realpath(__file__)) + "/../vcpkg")


def run(cmd):
    subprocess.check_call(cmd, shell=True)


if os.name == 'nt':
    run("bootstrap-vcpkg.bat")
    run("vcpkg.exe install --triplet x64-windows sdl2 freetype openssl")
else:
    run("./bootstrap-vcpkg.sh")
    if os.environ.get('DISPLAY'): # X11
        run("./vcpkg install sdl2 sdl2[x11] freetype openssl")
    else:
        run("./vcpkg install sdl2 freetype openssl")
