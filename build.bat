@echo off

echo ==========================================
echo      MAXIWINDOWSXP CLEAN + DEBUG BUILD
echo ==========================================
echo.

set "ROOT=%~dp0"
set "PRJ=%~dp0MAXIWINDOWSXP.vcxproj"
set "SOL=%~dp0"
set "VSROOT=C:\Program Files\Microsoft Visual Studio\18\Community"
set "VCVARS=%VSROOT%\VC\Auxiliary\Build\vcvars64.bat"

echo [INFO] Project:
echo %PRJ%
echo.

if not exist "%PRJ%" (
    echo [ERROR] MAXIWINDOWSXP.vcxproj not found!
    echo.
    pause
    exit /b 1
)

if not exist "%VCVARS%" (
    echo [ERROR] vcvars64.bat not found!
    echo.
    echo Searched:
    echo %VCVARS%
    echo.
    pause
    exit /b 1
)

echo [INFO] Setting up the Visual Studio environment...
call "%VCVARS%"

if errorlevel 1 (
    echo.
    echo [ERROR] Failed to initialize the Visual Studio environment!
    echo.
    pause
    exit /b 1
)

echo.
echo [INFO] Checking MSBuild...

where msbuild.exe

if errorlevel 1 (
    echo.
    echo [ERROR] MSBuild not found!
    echo.
    pause
    exit /b 1
)

echo.
echo ==========================================
echo [1/2] CLEAN
echo ==========================================
echo.

msbuild.exe "%PRJ%" /t:Clean /p:Configuration=Debug /p:Platform=x64 /p:SolutionDir="%SOL%"

if errorlevel 1 (
    echo.
    echo [ERROR] Clean failed!
    echo.
    pause
    exit /b 1
)

echo.
echo ==========================================
echo [2/2] BUILD DEBUG x64
echo ==========================================
echo.

msbuild.exe "%PRJ%" /t:Build /p:Configuration=Debug /p:Platform=x64 /p:SolutionDir="%SOL%" /nologo /v:minimal

if errorlevel 1 (
    echo.
    echo [ERROR] Build failed!
    echo.
    pause
    exit /b 1
)

echo.
echo ==========================================
echo SUCCESS
echo ==========================================
echo.
echo Build completed successfully.
echo.

if exist "%ROOT%x64\Debug\MAXIWINDOWSXP.exe" (
    echo EXE found:
    echo %ROOT%x64\Debug\MAXIWINDOWSXP.exe
) else (
    echo [WARNING] EXE not found at the expected path.
)

echo.
pause