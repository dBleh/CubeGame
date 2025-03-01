@echo off
cd C:\Users\dunca\Desktop\Projects\CubeGame\build\Release

:: Launch Host (Steam instance)
echo Launching Host instance...
start "Host" cmd /k "CubeShooter.exe & echo Host running... & pause"

:: Wait briefly to ensure host starts
timeout /t 2 /nobreak >nul

:: Launch Client (Debug mode)
echo Launching Client instance in debug mode...
start "Client" cmd /k "CubeShooter.exe --debug & echo Client running... & pause"

echo Two players launched! Host is Steam-authenticated, Client is in debug mode.
pause