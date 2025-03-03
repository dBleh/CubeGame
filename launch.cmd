@echo off
cd C:\Users\dunca\Desktop\Projects\CubeGame\build\Release

if not exist CubeShooter.exe (
    echo Error: CubeShooter.exe not found in C:\Users\dunca\Desktop\Projects\CubeGame\build\Release
    pause
    exit /b 1
)

echo Launching Host instance in debug mode...
start "Host" cmd /k "CubeShooter.exe --host & echo Host running... Press Ctrl+C to exit."

timeout /t 2 /nobreak >nul

echo Launching Client instance in debug mode...
start "Client" cmd /k "CubeShooter.exe & echo Client running... Press Ctrl+C to exit."

echo Two players launched in debug mode! Game should start automatically.
pause