@echo off
setlocal enabledelayedexpansion

set "ROOT=%~dp0"
set "PRJ=%ROOT%MAXIWINDOWSXP.vcxproj"

call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
msbuild "%PRJ%" /p:Configuration=Debug /p:Platform=x64 /p:SolutionDir="%ROOT%" /t:Build /nologo /v:minimal 2>&1

if errorlevel 1 (
    echo [BUILD FAILED]
    exit /b 1
)

set "SRC=%ROOT%x64\Debug\MAXIWINDOWSXP.exe"
set "DST=%ROOT%x64\Debug"

if not exist "%SRC%" (
    echo [WARN] EXE not found: %SRC%
    exit /b 0
)

set "ALPH=ABCDEFGHJKLMNPQRSTUVWXYZ23456789"
set "NAME="
for /L %%i in (1,1,10) do (
    set /a "r=!random! %% 32"
    for /f %%c in ("!r!") do set "NAME=!NAME!!ALPH:~%%c,1!"
)

copy /Y "%SRC%" "%DST%\!NAME!.exe" >nul
if errorlevel 1 (
    echo [WARN] Copy failed, keeping original name
    exit /b 0
)

del /F /Q "%SRC%" >nul 2>&1

echo.
echo ==========================================
echo   BUILD OK  -  Obfuscated EXE: !NAME!.exe
echo ==========================================
