@echo off
cd /d "%~dp0.."
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
msbuild BitWork.slnx /p:Configuration=Debug /p:Platform=x64 /v:minimal /nologo
if errorlevel 1 exit /b 1
msbuild BitWork.slnx /p:Configuration=Release /p:Platform=x64 /v:minimal /nologo
