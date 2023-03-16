@echo off

start calc.exe

timeout 2 > NUL

cd build
injector CalculatorApp.exe library.dll

timeout 2 > NUL

type %USERPROFILE%\AppData\Local\Packages\Microsoft.WindowsCalculator_8wekyb3d8bbwe\AC\log.txt

cd ..
