#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <numeric>
#include <random>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>

// ─────────────────────────────────────────────────────────────────────────────
// Utilitários
// ─────────────────────────────────────────────────────────────────────────────

inline int fastFloor(float x) {
    int xi = (int)x;
    return x < xi ? xi - 1 : xi;
}

// ─────────────────────────────────────────────────────────────────────────────
// Modos de suavização (Fade)
// ─────────────────────────────────────────────────────────────────────────────

enum class FadeMode { NONE, CUBIC, QUINTIC };

const char* fadeName(FadeMode m) {
    switch (m) {
    case FadeMode::NONE:    return "NONE    (linear)";
    case FadeMode::CUBIC:   return "CUBIC   (3t²-2t³)  [Perlin 1985]";
    case FadeMode::QUINTIC: return "QUINTIC (6t⁵-15t⁴+10t³) [Perlin 2002]";
    }
    return "?";
}

// ─────────────────────────────────────────────────────────────────────────────
// Perlin Noise
// ─────────────────────────────────────────────────────────────────────────────

struct PerlinNoise {

    // Vetores gradiente 2D: 4 cardeais + 4 diagonais unitárias
    static constexpr float G2D[8][2] = {
        { 1.0f,       0.0f      }, {-1.0f,       0.0f      },
        { 0.0f,       1.0f      }, { 0.0f,      -1.0f      },
        { 0.707106f,  0.707106f }, {-0.707106f,  0.707106f },
        { 0.707106f, -0.707106f }, {-0.707106f, -0.707106f }
    };

    std::vector<int> p; // Tabela de permutação (512 = 256 duplicado)
    FadeMode fadeMode;

    // Construtor: embaralha a tabela com uma semente via Fisher-Yates
    PerlinNoise(unsigned int seed, FadeMode mode = FadeMode::QUINTIC) : fadeMode(mode)
    {
        p.resize(256);
        std::iota(p.begin(), p.end(), 0);
        std::default_random_engine engine(seed);
        std::shuffle(p.begin(), p.end(), engine);
        p.insert(p.end(), p.begin(), p.end()); // duplica → 512
    }

    // ── Funções de suavização ────────────────────────────────────────────────

    // Sem suavização: interpolação linear pura — grade visível
    float fadeNone(float t) { return t; }

    // Cúbica (Perlin 1985) — garante C¹, mas não C²
    float fadeCubic(float t) { return t * t * (3.0f - 2.0f * t); }

    // Quíntica (Perlin 2002) — garante C²: f'(0)=f'(1)=f''(0)=f''(1)=0
    float fadeQuintic(float t) { return t * t * t * (t * (6.0f * t - 15.0f) + 10.0f); }

    float fade(float t) {
        switch (fadeMode) {
        case FadeMode::NONE:    return fadeNone(t);
        case FadeMode::CUBIC:   return fadeCubic(t);
        case FadeMode::QUINTIC: return fadeQuintic(t);
        }
        return t;
    }

    // ── Interpolação linear ──────────────────────────────────────────────────

    float lerp(float t, float a, float b) { return a + t * (b - a); }

    // ── Gradiente: produto escalar com vetor de distância ───────────────────

    float grad(int hash, float x, float y) {
        int h = hash & 7;
        return G2D[h][0] * x + G2D[h][1] * y;
    }

    // ── Ruído de uma oitava ──────────────────────────────────────────────────

    float noise(float x, float y) {
        // 1. Célula da grade
        int xi = fastFloor(x);
        int yi = fastFloor(y);
        int i = xi & 255;
        int j = yi & 255;

        // 2. Posição relativa dentro da célula [0, 1)
        float xf = x - xi;
        float yf = y - yi;

        // 3. Fade
        float u = fade(xf);
        float v = fade(yf);

        // 4. Hash dos 4 cantos via tabela de permutação
        int h00 = p[p[i] + j];
        int h01 = p[p[i] + j + 1];
        int h10 = p[p[i + 1] + j];
        int h11 = p[p[i + 1] + j + 1];

        // 5. Dot products + interpolação bilinear
        float x1 = lerp(u, grad(h00, xf, yf),
            grad(h10, xf - 1.0f, yf));
        float x2 = lerp(u, grad(h01, xf, yf - 1.0f),
            grad(h11, xf - 1.0f, yf - 1.0f));

        return lerp(v, x1, x2); // resultado em [-1, 1] (aproximado)
    }

    // ── fBm: soma de oitavas (Fractal Brownian Motion) ───────────────────────

    float fBm(float x, float y, int octaves, float persistence, float lacunarity)
    {
        float total = 0.0f;
        float amplitude = 1.0f;
        float frequency = 1.0f;
        float maxValue = 0.0f;

        for (int i = 0; i < octaves; ++i) {
            total += noise(x * frequency, y * frequency) * amplitude;
            maxValue += amplitude;
            amplitude *= persistence;
            frequency *= lacunarity;
        }

        return total / maxValue; // normalizado em [-1, 1]
    }

    // ── Acesso público ao hash (usado pelo debug visual) ─────────────────────

    int getHash(int i, int j) const {
        return p[p[i & 255] + (j & 255)];
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────

int main() {
    const unsigned int WIDTH = 800;
    const unsigned int HEIGHT = 600;

    sf::RenderWindow window(
        sf::VideoMode(sf::Vector2u(WIDTH, HEIGHT)),
        "Visualizador Perlin Noise"
    );

    sf::Texture texture;
    {
        sf::Image blank(sf::Vector2u(WIDTH, HEIGHT), sf::Color::Black);
        texture.loadFromImage(blank);
    }
    sf::Sprite sprite(texture);

    // ── Estado ───────────────────────────────────────────────────────────────

    PerlinNoise pn(42);

    int   octaves = 6;
    float persistence = 0.5f;
    float lacunarity = 2.0f;
    float scale = 0.01f;
    float degrees = 12.0f; // níveis de posterização
    float offsetX = 0.0f;
    float offsetY = 0.0f;

    bool needsUpdate = true;
    bool showDebug = false;
    bool fBmOn = true;

    // Posição do mouse — atualizada todo frame para o display do valor
    sf::Vector2i lastMousePos(-1, -1);

    // ── Helpers de log ───────────────────────────────────────────────────────

    auto printStatus = [&]() {
        printf("scale=%.4f | octaves=%d | persistence=%.2f | lacunarity=%.2f"
            " | fBm=%s | fade=%s\n\n",
            scale, octaves, persistence, lacunarity,
            fBmOn ? "ON" : "OFF",
            fadeName(pn.fadeMode));
        };

    // ── Loop principal ────────────────────────────────────────────────────────

    while (window.isOpen()) {

        // ── Eventos ──────────────────────────────────────────────────────────

        while (auto event = window.pollEvent()) {

            if (event->is<sf::Event::Closed>())
                window.close();

            // Zoom centrado no cursor
            if (auto* scroll = event->getIf<sf::Event::MouseWheelScrolled>()) {
                float factor = (scroll->delta > 0) ? 0.9f : 1.1f;
                float oldScale = scale;
                scale *= factor;

                float targetX = WIDTH / 2.0f;
                float targetY = HEIGHT / 2.0f;

                sf::Vector2i mp = sf::Mouse::getPosition(window);
                if (mp.x >= 0 && mp.x < (int)WIDTH &&
                    mp.y >= 0 && mp.y < (int)HEIGHT) {
                    targetX = (float)mp.x;
                    targetY = (float)mp.y;
                }

                float ratio = oldScale / scale;
                offsetX = (targetX + offsetX) * ratio - targetX;
                offsetY = (targetY + offsetY) * ratio - targetY;

                needsUpdate = true;
            }

            if (auto* key = event->getIf<sf::Event::KeyPressed>()) {

                switch (key->code) {

                        // Fechar
                    case sf::Keyboard::Key::Escape:
                        window.close();
                        break;

                        // Nova semente aleatória
                    case sf::Keyboard::Key::Space:
                        pn = PerlinNoise(std::random_device{}(), pn.fadeMode);
                        offsetX = offsetY = 0.0f;
                        needsUpdate = true;
                        break;

                        // Debug visual (grade + gradientes + corte)
                    case sf::Keyboard::Key::Tab:
                        showDebug = !showDebug;
                        break;

                        // Toggle fBm
                    case sf::Keyboard::Key::F:
                        fBmOn = !fBmOn;
                        needsUpdate = true;
                        printStatus();
                        break;

                        // Cicla modo de fade: NONE → CUBIC → QUINTIC → NONE ...
                    case sf::Keyboard::Key::V:
                        pn.fadeMode = (FadeMode)(((int)pn.fadeMode + 1) % 3);
                        needsUpdate = true;
                        printStatus();
                        break;

                        // Oitavas
                    case sf::Keyboard::Key::O:
                        octaves = std::max(1, octaves - 1);
                        needsUpdate = true; printStatus(); break;
                    case sf::Keyboard::Key::P:
                        octaves = std::min(16, octaves + 1);
                        needsUpdate = true; printStatus(); break;

                        // Persistência
                    case sf::Keyboard::Key::Num1:
                    case sf::Keyboard::Key::Numpad1:
                        persistence = std::max(0.0f, persistence - 0.05f);
                        needsUpdate = true; printStatus(); break;
                    case sf::Keyboard::Key::Num2:
                    case sf::Keyboard::Key::Numpad2:
                        persistence = std::min(2.0f, persistence + 0.05f);
                        needsUpdate = true; printStatus(); break;

                        // Lacunaridade
                    case sf::Keyboard::Key::Num3:
                    case sf::Keyboard::Key::Numpad3:
                        lacunarity = std::max(1.0f, lacunarity - 0.1f);
                        needsUpdate = true; printStatus(); break;
                    case sf::Keyboard::Key::Num4:
                    case sf::Keyboard::Key::Numpad4:
                        lacunarity += 0.1f;
                        needsUpdate = true; printStatus(); break;

                        // Posterização
                    case sf::Keyboard::Key::Z:
                        degrees = std::max(2.0f, degrees - 1.0f);
                        needsUpdate = true; break;
                    case sf::Keyboard::Key::X:
                        degrees += 1.0f;
                        needsUpdate = true; break;

                        // Navegação (câmera)
                    case sf::Keyboard::Key::W:
                        offsetY -= 20.0f / scale * 0.01f;
                        needsUpdate = true; break;
                    case sf::Keyboard::Key::S:
                        offsetY += 20.0f / scale * 0.01f;
                        needsUpdate = true; break;
                    case sf::Keyboard::Key::A:
                        offsetX -= 20.0f / scale * 0.01f;
                        needsUpdate = true; break;
                    case sf::Keyboard::Key::D:
                        offsetX += 20.0f / scale * 0.01f;
                        needsUpdate = true; break;

                    default: break;
                }
            }
        }

        // ── Renderização do mapa de ruído ─────────────────────────────────────

        if (needsUpdate) {
            std::vector<std::uint8_t> pixelBuffer(WIDTH * HEIGHT * 4);

            auto t0 = std::chrono::high_resolution_clock::now();

            for (int y = 0; y < (int)HEIGHT; ++y) {
                for (int x = 0; x < (int)WIDTH; ++x) {
                    float nx = (x + offsetX) * scale;
                    float ny = (y + offsetY) * scale;

                    float val = fBmOn
                        ? pn.fBm(nx, ny, octaves, persistence, lacunarity)
                        : pn.noise(nx, ny);

                    float normalized = (val + 1.0f) / 2.0f;
                    float posterized = fastFloor(normalized * degrees) / degrees;
                    int   c = (int)(posterized * 255.0f);
                    if (c < 0)   c = 0;
                    if (c > 255) c = 255;

                    int idx = (y * WIDTH + x) * 4;
                    pixelBuffer[idx + 0] = (uint8_t)c;
                    pixelBuffer[idx + 1] = (uint8_t)c;
                    pixelBuffer[idx + 2] = (uint8_t)c;
                    pixelBuffer[idx + 3] = 255;
                }
            }

            auto t1 = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> dt = t1 - t0;
            printf("Tempo de geracao: %.2f ms\n", dt.count());
            printStatus();

            texture.update(pixelBuffer.data());
            needsUpdate = false;
        }

        // ── Valor do ruído sob o cursor (atualizado todo frame) ───────────────

        sf::Vector2i mp = sf::Mouse::getPosition(window);

        if (mp.x >= 0 && mp.x < (int)WIDTH &&
            mp.y >= 0 && mp.y < (int)HEIGHT &&
            mp != lastMousePos)
        {
            lastMousePos = mp;

            float nx = (mp.x + offsetX) * scale;
            float ny = (mp.y + offsetY) * scale;
            float val = fBmOn
                ? pn.fBm(nx, ny, octaves, persistence, lacunarity)
                : pn.noise(nx, ny);

            // \r sobrescreve a linha — não polui o terminal
            printf("\r[px %4d, %4d] | ruido = %+.5f   ", mp.x, mp.y, val);
            fflush(stdout);
        }

        // ── Desenho ───────────────────────────────────────────────────────────

        window.clear();
        window.draw(sprite);

        if (showDebug) {
            float cellSize = 1.0f / scale;

            // Guard: só desenha se a grade couber razoavelmente na tela
            if (cellSize > 20.0f && cellSize < (float)WIDTH) {

                // Grade
                sf::VertexArray gridLines(sf::PrimitiveType::Lines);
                sf::Color gridColor(255, 255, 255, 50);

                for (float x = 0; x <= WIDTH; x += cellSize) {
                    gridLines.append({ sf::Vector2f(x, 0.f),          gridColor });
                    gridLines.append({ sf::Vector2f(x, (float)HEIGHT), gridColor });
                }
                for (float y = 0; y <= HEIGHT; y += cellSize) {
                    gridLines.append({ sf::Vector2f(0.f,         y), gridColor });
                    gridLines.append({ sf::Vector2f((float)WIDTH, y), gridColor });
                }
                window.draw(gridLines);

                // Vetores gradiente com seta
                sf::VertexArray gradients(sf::PrimitiveType::Lines);
                const float PI = 3.14159265f;

                for (float y = 0; y <= HEIGHT; y += cellSize) {
                    for (float x = 0; x <= WIDTH; x += cellSize) {
                        int i = (int)((x + offsetX) * scale) & 255;
                        int j = (int)((y + offsetY) * scale) & 255;

                        int h = pn.getHash(i, j) & 7;
                        float gx = PerlinNoise::G2D[h][0];
                        float gy = PerlinNoise::G2D[h][1];

                        sf::Vector2f orig(x, y);
                        sf::Vector2f tip(x + gx * cellSize * 0.4f,
                            y + gy * cellSize * 0.4f);

                        gradients.append({ orig, sf::Color(255, 0, 0) });
                        gradients.append({ tip,  sf::Color(255, 0, 0) });

                        // Ponta da seta
                        float arrowLen = cellSize * 0.1f;
                        float angle = std::atan2(gy, gx);
                        float arrowAngle = PI / 6.0f;

                        for (int side : {-1, 1}) {
                            float a = angle + PI + side * arrowAngle;
                            sf::Vector2f barb(tip.x + std::cos(a) * arrowLen,
                                tip.y + std::sin(a) * arrowLen);
                            gradients.append({ tip,  sf::Color(255, 0, 0) });
                            gradients.append({ barb, sf::Color(255, 0, 0) });
                        }
                    }
                }
                window.draw(gradients);

                // Corte transversal interativo
                sf::Vector2i mousePos = sf::Mouse::getPosition(window);
                float cutY = (float)(HEIGHT / 2);
                if (mousePos.x >= 0 && mousePos.x < (int)WIDTH &&
                    mousePos.y >= 0 && mousePos.y < (int)HEIGHT)
                    cutY = (float)mousePos.y;

                sf::VertexArray cutLine(sf::PrimitiveType::Lines);
                cutLine.append({ sf::Vector2f(0.f,         cutY), sf::Color(0, 255, 0, 200) });
                cutLine.append({ sf::Vector2f((float)WIDTH, cutY), sf::Color(0, 255, 0, 200) });
                window.draw(cutLine);

                // Perfil do terreno na linha de corte
                sf::VertexArray profile(sf::PrimitiveType::LineStrip);
                float graphBaseY = HEIGHT - 80.0f;
                float amplitude = 60.0f;

                for (float x = 0; x <= WIDTH; x += 1.0f) {
                    float nx = (x + offsetX) * scale;
                    float ny = (cutY + offsetY) * scale;
                    float v = fBmOn
                        ? pn.fBm(nx, ny, octaves, persistence, lacunarity)
                        : pn.noise(nx, ny);
                    profile.append({ sf::Vector2f(x, graphBaseY - v * amplitude),
                                     sf::Color(255, 255, 0) });
                }

                sf::VertexArray zeroLine(sf::PrimitiveType::Lines);
                zeroLine.append({ sf::Vector2f(0.f,          graphBaseY), sf::Color(255, 165, 0, 100) });
                zeroLine.append({ sf::Vector2f((float)WIDTH, graphBaseY), sf::Color(255, 165, 0, 100) });

                window.draw(zeroLine);
                window.draw(profile);
            }
        }

        window.display();
    }

    return 0;
}
