# PerlinNoise

Visualizador simples de Perlin Noise usando SFML (C++17).

Objetivo: permitir que qualquer pessoa (Windows / Linux) clone o repositório e consiga compilar executando apenas comandos CMake.

Build (recomendado — usa SFML do sistema quando disponível):

Linux (Debian/Ubuntu):

```bash
sudo apt update
sudo apt install -y build-essential cmake libsfml-dev ninja-build
cmake -S . -B build -G Ninja -DUSE_SYSTEM_SFML=ON
cmake --build build --config Release
./build/PerlinNoise
```

Windows (MSYS2):

Abra o MSYS2 MinGW UCRT64 shell e instale SFML:

```bash
# no MSYS2 UCRT64
pacman -Sy
pacman -S --noconfirm mingw-w64-ucrt-x86_64-toolchain mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-sfml mingw-w64-ucrt-x86_64-ninja
cmake -S . -B build -G Ninja -DCMAKE_C_COMPILER=/c/msys64/ucrt64/bin/gcc.exe -DCMAKE_CXX_COMPILER=/c/msys64/ucrt64/bin/g++.exe -DUSE_SYSTEM_SFML=ON
cmake --build build
./build/PerlinNoise.exe
```

Alternativa: deixar o CMake baixar e compilar o SFML automaticamente (pode levar mais tempo):

```bash
cmake -S . -B build -G Ninja -DUSE_SYSTEM_SFML=OFF
cmake --build build
```

Compilar sem CMake (não recomendado — necessário apontar includes e libs do SFML manualmente):

```bash
#g++ (exemplo Linux)
g++ -std=c++17 main.cpp -o PerlinNoise -lsfml-graphics -lsfml-window -lsfml-system
```
