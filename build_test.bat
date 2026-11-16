@echo off
set "ROOT=%~dp0"
set "PRJ=%ROOT%MAXIWINDOWSXP.vcxproj"

call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
msbuild "%PRJ%" /p:Configuration=Debug /p:Platform=x64 /p:SolutionDir="%ROOT%" /p:TargetName=MAXITEST /t:Build /nologo /v:minimal 2>&1
