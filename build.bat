@echo off

mkdir build
pushd build
cl /Zi /Od ..\main.c

popd
