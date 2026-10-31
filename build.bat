@echo off

if "%1"=="release" (
    set flags=/O2 /Zi /DNDEBUG
) else (
    set flags=/Zi /Od
)

if not exist build mkdir build
pushd build
cl %flags% /W4 /DUNICODE /D_UNICODE ..\main.c /link user32.lib gdi32.lib opengl32.lib
popd
