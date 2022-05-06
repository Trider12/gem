#!/usr/bin/env python3

import os
import subprocess

os.chdir(os.path.dirname(os.path.realpath(__file__)) + "/../vcpkg")


def run(cmd):
    subprocess.check_call(cmd, shell=True)


if os.name == 'nt':
    run("bootstrap-vcpkg.bat")
    run("vcpkg.exe install sdl2:x64-windows")
else:
    run("./bootstrap-vcpkg.sh")
    run("./vcpkg install sdl2")
