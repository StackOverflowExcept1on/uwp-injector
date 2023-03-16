@echo off

set CMAKE_BUILD_TYPE="Release"

if not exist build mkdir build
cd build

set vswhere="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set vcvarsLookup=call %vswhere% -latest -property installationPath

for /f "tokens=*" %%i in ('%vcvarsLookup%') do set vcvars="%%i\VC\Auxiliary\Build\vcvars64.bat"

call %vcvars% uwp

cmake -G Ninja -DCMAKE_BUILD_TYPE=%CMAKE_BUILD_TYPE% -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON -DCMAKE_SYSTEM_NAME=WindowsStore -DCMAKE_SYSTEM_VERSION=10 ..
cmake --build .
cmake --install .

cd ..
