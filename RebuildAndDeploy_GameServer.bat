@echo off
echo ============================================
echo Rebuilding GameServer with latest changes
echo ============================================

REM Find MSBuild (try multiple VS versions)
set MSBUILD=""

if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" (
    set MSBUILD="C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
)

if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe" (
    set MSBUILD="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe"
)

if %MSBUILD%=="" (
    echo ERROR: MSBuild not found! Please build from Visual Studio manually.
    pause
    exit /b 1
)

echo Found MSBuild: %MSBUILD%
echo.

REM Build GameServer
echo Building Solution (Release4Server x64)...
%MSBUILD% "DboServer\DboServer.sln" /p:Configuration=Release4Server /p:Platform=x64 /t:Build /m

if errorlevel 1 (
    echo.
    echo ERROR: Build failed!
    pause
    exit /b 1
)

echo.
echo ============================================
echo Build successful! Deploying to ExecutionEnv
echo ============================================

REM Copy the built exe to ExecutionEnv (in case OutDir didn't already place it there)
if exist "DboServer\Server\GameServer\x64\Release\GameServer.exe" (
    copy /Y "DboServer\Server\GameServer\x64\Release\GameServer.exe" "DboServer\ExecutionEnv\GameServer.exe"
) else (
    echo NOTE: GameServer.exe not found under x64\Release; assuming it was emitted directly to ExecutionEnv by project OutDir.
)

if errorlevel 1 (
    echo ERROR: Failed to copy GameServer.exe
    pause
    exit /b 1
)

echo.
echo ============================================
echo SUCCESS! GameServer.exe updated
echo You can now restart the server
echo ============================================
echo.
pause
