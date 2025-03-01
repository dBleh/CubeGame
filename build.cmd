@echo off
cd C:\Users\dunca\Desktop\Projects\CubeGame
rmdir /S /Q build 2>nul
mkdir build
mkdir build\Release
copy steam_appid.txt build\Release\steam_appid.txt
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
echo Build complete!
pause