@echo off

if not exist build mkdir build

cd injector
call build.bat
copy build\bin\injector.exe ..\build
cd ..

cd library
call build.bat
copy build\bin\library.dll ..\build
cd ..
