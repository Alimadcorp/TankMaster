@echo off
g++ -std=c++17 main.cpp -I"C:/SFML/include" -L"C:/SFML/lib" -static -lsfml-graphics -lsfml-window -lsfml-system -lsfml-network -lsfml-audio -lopengl32 -lglu32 -o main.exe
if %errorlevel% equ 0 (
    main.exe
)
pause
s