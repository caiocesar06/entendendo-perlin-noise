# Visualizador Perlin Noise

Ferramenta interativa de pesquisa e visualização matemática para algoritmos de ruído procedural (Perlin Noise e Fractal Brownian Motion) desenvolvida em C++17 e SFML.

O projeto permite a exploração da malha vetorial com câmera dinâmica, ajuste paramétrico em tempo real e ferramentas de *debug* analítico avançadas (como vetores gradientes, seções transversais e alternância de curvas de interpolação).

## 🎮 Controles e Navegação

| Tecla(s) | Ação |
| :--- | :--- |
| **W, A, S, D** | Navega a câmera bidimensional pelo plano infinito |
| **Scroll do Mouse** | Aplica Zoom In/Out ancorado na posição atual do ponteiro |
| **Espaço** | Injeta uma nova semente aleatória via `std::random_device`, gerando um novo mapa |
| **Esc** | Encerra a aplicação |

### 🔬 Ferramentas de Debug e Ajuste Paramétrico (Runtime)

| Tecla(s) | Parâmetro | Descrição |
| :--- | :--- | :--- |
| **TAB** | Modo Analítico | Alterna a exibição da grade de células, vetores gradientes (com setas direcionais) e o corte transversal interativo |
| **F** | Modo fBm | Liga/desliga o *Fractal Brownian Motion*, alternando entre a oitava base isolada e a composição fractal complexa |
| **V** | Modo de Fade | Alterna a curva matemática de suavização entre Linear (Grade visível), Cúbica (Perlin 1985) e Quíntica (Perlin 2002) |
| **O / P** | Oitavas | Aumenta/diminui as frequências sobrepostas no algoritmo fBm (Limites: 1 a 16) |
| **1 / 2** | Persistência | Modifica o multiplicador de atenuação de amplitude a cada oitava |
| **3 / 4** | Lacunaridade | Modifica o multiplicador de crescimento de frequência a cada oitava |
| **Z / X** | Posterização | Aumenta/diminui a resolução das "fatias" de cores para visualização de isolinhas |

> **Telemetria e Inspeção:** A ferramenta rastreia a posição do cursor a cada *frame*, imprimindo o tempo exato de geração (em *ms*) e o valor bruto do ruído (entre -1 e 1) diretamente no console para validação matemática.

## 🛠️ Compilação e Build

Objetivo: permitir que qualquer pessoa (Windows / Linux) clone o repositório e consiga compilar executando apenas comandos CMake.

Build (recomendado — usa SFML do sistema quando disponível):

### Linux (Debian/Ubuntu):

```bash
sudo apt update
sudo apt install -y build-essential cmake libsfml-dev ninja-build
cmake -S . -B build -G Ninja -DUSE_SYSTEM_SFML=ON
cmake --build build --config Release
./build/PerlinNoise
```

### Windows (MSYS2):

Abra o MSYS2 MinGW UCRT64 shell e instale SFML:

```bash
# no MSYS2 UCRT64
pacman -Sy
pacman -S --noconfirm mingw-w64-ucrt-x86_64-toolchain mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-sfml mingw-w64-ucrt-x86_64-ninja
cmake -S . -B build -G Ninja -DCMAKE_C_COMPILER=/c/msys64/ucrt64/bin/gcc.exe -DCMAKE_CXX_COMPILER=/c/msys64/ucrt64/bin/g++.exe -DUSE_SYSTEM_SFML=ON
cmake --build build
./build/PerlinNoise.exe
```

### Alternativas de Build
Deixar o CMake baixar e compilar o SFML automaticamente (pode levar mais tempo):

```bash
cmake -S . -B build -G Ninja -DUSE_SYSTEM_SFML=OFF
cmake --build build
```

Compilar sem CMake (não recomendado — necessário apontar includes e libs do SFML manualmente):

```bash
#g++ (exemplo Linux)
g++ -std=c++17 main.cpp -o PerlinNoise -lsfml-graphics -lsfml-window -lsfml-system
```
