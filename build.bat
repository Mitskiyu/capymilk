@echo off

mkdir build
pushd build
cl /Zi /Od ..\main.c /link user32.lib

popd
