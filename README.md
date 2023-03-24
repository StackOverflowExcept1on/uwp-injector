### uwp-injector

[![Build Status](https://github.com/StackOverflowExcept1on/uwp-injector/actions/workflows/ci.yml/badge.svg)](https://github.com/StackOverflowExcept1on/uwp-injector/actions/workflows/ci.yml)

This project allows to perform [DLL-injection](https://en.wikipedia.org/wiki/DLL_injection)
into [UWP applications](https://en.wikipedia.org/wiki/Universal_Windows_Platform) like Windows Calculator.

Also, it can be used to inject DLL into Windows Store games.

### Requirements

- Visual Studio 2022 https://visualstudio.microsoft.com/en/vs/ with enabled:
    - Desktop development with C++
    - Universal Windows Platform development

### Usage

```
injector process.exe library.dll
```

### Building

```
_build.bat
```

### Running demo

```
_inject.bat
```

```
successfully loaded into process memory
pid: 3504

> frida-ps
  PID  Name
-----  ---------------------------
 3504  CalculatorApp.exe
```
