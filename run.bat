@echo off
g++ -std=c++17 main.cpp -lGL -lsfml-graphics -lsfml-window -lsfml-system -lsfml-network -lsfml-audio
if %errorlevel% equ 0 (
    a.exe
)
pause
