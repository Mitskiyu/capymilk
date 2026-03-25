@echo off

mkdir build
pushd build
cl /Zi /Od ..\main.c /link user32.lib gdi32.lib opengl32.lib

popd
