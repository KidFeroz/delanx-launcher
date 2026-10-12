@echo off
setlocal
set "MSYS_BASH=D:\devkitPro\msys2\msys64\usr\bin\bash.exe"
if exist "%MSYS_BASH%" (
    "%MSYS_BASH%" -lc "export PATH=/opt/devkitpro/tools/bin:/opt/devkitpro/devkitA64/bin:$PATH; export DEVKITPRO=/opt/devkitpro; cd /e/Vibecoding/Delanx && make -C tests/device"
) else (
    echo Error: No se encontro el entorno devkitPro / MSYS2 en D:\devkitPro
    exit /b 1
)
