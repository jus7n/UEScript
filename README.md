﻿## Building
1. Recursively clone the repository `git clone --recursive https://github.com/jus7n/UEScript` 
2. Build LuaJIT statically
   1. Open "x64 Native Tools Command Prompt" (comes with Visual Studio)
   2. Run `cd uescript/vendor/luajit/src`
   3. Run `msvcbuild.bat static`
3. Install MinHook dependency
   1. Download [vcpkg](https://github.com/microsoft/vcpkg)
   2. Install the `minhook:x64-windows-static` triplet
