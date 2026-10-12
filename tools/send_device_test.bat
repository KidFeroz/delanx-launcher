@echo off
setlocal
set "NXLINK=D:\devkitPro\msys2\msys64\opt\devkitpro\tools\bin\nxlink.exe"
if exist "%NXLINK%" (
    "%NXLINK%" -s tests\device\DelanxSelfTest.nro %*
) else (
    echo Error: nxlink.exe no encontrado en D:\devkitPro\msys2\msys64\opt\devkitpro\tools\bin
    exit /b 1
)
