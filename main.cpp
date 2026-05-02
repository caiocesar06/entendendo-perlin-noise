#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <numeric>
#include <random>
#include <algorithm>
#include <chrono>
#include <cstdint>

inline int fastFloor(float x) {
    int xi = (int)x;
    return x < xi ? xi - 1 : xi;
}

struct PerlinNoise {
    std::vector<int> p; // Tabela de permutação

    // Construtor: Inicializa e embaralha a tabela de permutação com uma semente
    PerlinNoise(unsigned int seed) {
        p.resize(256);
        std::iota(p.begin(), p.end(), 0); // Preenche de 0 a 255

        std::default_random_engine engine(seed);
        std::shuffle(p.begin(), p.end(), engine); // Embaralha (Fisher-Yates)

        // Duplica para 512 para evitar overflow
        p.insert(p.end(), p.begin(), p.end());
    }

    // Função de suavização (Fade)
    float fade(float t) {
        return t * t * t * (t * (6 * t - 15) + 10);
        // return t * t * (3 - 2 * t);
    }

    // float fade(float t) {
    //     const float PI = 3.14159265358979323846;
    //     return (1.0 - cos(t * PI)) * 0.5;
    // }

    // Interpolação linear
    float lerp(float t, float a, float b) {
        return a + t * (b - a);
    }

    // Seleção do gradiente e produto escalar
    // float grad(int hash, float x, float y) {
    //     int h = hash & 7;
    //     float u = h < 4 ? x : y;
    //     float v = h < 4 ? y : x;

    //     // Simula o produto escalar com direções fixas
    //     return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
    // }

    float grad(int hash, float x, float y) {
        // Conjunto G_2D: 8 vetores unitários uniformes (cardeais e diagonais)
        const float G2D[8][2] = {
            {1.0, 0.0}, {-1.0, 0.0},
            {0.0, 1.0}, {0.0, -1.0},
            {0.707106, 0.707106}, {-0.707106, 0.707106},
            {0.707106, -0.707106}, {-0.707106, -0.707106}
        };

        int h = hash & 7; // Escolhe um índice de 0 a 7

        // Calcula o produto escalar explicitamente
        return G2D[h][0] * x + G2D[h][1] * y;
    }

    // Função principal do ruído de uma oitava
    float noise(float x, float y) {
        // 1. Encontrar coordenadas da célula (floor)
        int xi = fastFloor(x);
        int yi = fastFloor(y);

        int i = xi & 255;
        int j = yi & 255;

        // 2. Posição relativa dentro da célula [0, 1)
        float xf = x - xi;
        float yf = y - yi;

        // 3. Aplicar a função fade em u e v
        float u = fade(xf);
        float v = fade(yf);
        // float u = x;
        // float v = y;

        // 4. Pegar os hashes dos 4 cantos da célula
        int h00 = p[p[i] + j];
        int h01 = p[p[i] + j + 1];
        int h10 = p[p[i + 1] + j];
        int h11 = p[p[i + 1] + j + 1];

        // 5. Calcular o dot product de cada canto
        float x1 = lerp(u, grad(h00, xf, yf), grad(h10, xf - 1, yf));
        float x2 = lerp(u, grad(h01, xf, yf - 1), grad(h11, xf - 1, yf - 1));

        // 6. Interpolar tudo de forma bilinear
        return lerp(v, x1, x2);
    }

    // Função de múltiplas escalas (Fractal Brownian Motion)
    float fBm(float x, float y, int octaves, float persistence, float lacunarity) {
        float total = 0.0;
        float amplitude = 1.0;
        float frequency = 1.0;
        float maxValue = 0.0;

        for (int i = 0; i < octaves; ++i) {
            total += noise(x * frequency, y * frequency) * amplitude;
            maxValue += amplitude;
            amplitude *= persistence;
            frequency *= lacunarity;
        }

        return total / maxValue;
    }

    int getHash(int i, int j) const {
        return p[p[i & 255] + (j & 255)];
    }
};

int main() {
    const unsigned int WIDTH = 800;
    const unsigned int HEIGHT = 600;

    sf::RenderWindow window(sf::VideoMode(sf::Vector2u(WIDTH, HEIGHT)), "Visualizador Perlin Noise");
    sf::Image image(sf::Vector2u(WIDTH, HEIGHT), sf::Color::Black);

    sf::Texture texture;
    sf::Sprite sprite(texture);

    if (texture.loadFromImage(image)) {
        sprite.setTexture(texture, true);
    }

    // Semente inicial
    PerlinNoise pn(42);

    // Parâmetros do fBm
    int octaves = 6;
    float persistence = 0.5;
    float lacunarity = 2.0;
    float scale = 0.01;

    bool needsUpdate = true;

    bool showDebug = false;
    bool fBmOn = true;

    float offsetX = 0.0f, offsetY = 0.0f;

    while (window.isOpen()) {
        while (auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            if (auto* scroll = event->getIf<sf::Event::MouseWheelScrolled>()) {
                float factor = (scroll->delta > 0) ? 0.9f : 1.1f;
                scale *= factor;
                needsUpdate = true;
            }

            if (event->is<sf::Event::KeyPressed>()) {
                auto* keyEvent = event->getIf<sf::Event::KeyPressed>();

                // Pressione Espaço para gerar uma nova semente e forçar o redesenho
                if (keyEvent && keyEvent->code == sf::Keyboard::Key::Space) {
                    pn = PerlinNoise(std::random_device{}());
                    offsetX = 0.0f; offsetY = 0.0f;
                    needsUpdate = true;
                }

                // Pressione ESC para fechar a janela
                if (keyEvent && keyEvent->code == sf::Keyboard::Key::Escape) {
                    window.close();
                }

                // Pressione TAB para mostrar o Debug
                if (keyEvent && keyEvent->code == sf::Keyboard::Key::Tab) {
                    showDebug = !showDebug; // Inverte o booleano
                }

                // Pressione F para ativar o fBm
                if (keyEvent && keyEvent->code == sf::Keyboard::Key::F) {
                    fBmOn = !fBmOn; // Inverte o booleano
                    needsUpdate = true;
                }

                // Câmera de Navegação
                float moveSpeed = 20.0f; // Move 20 pixels por clique
                if (keyEvent && keyEvent->code == sf::Keyboard::Key::W) {
                    offsetY -= moveSpeed; // W move a câmera para cima (Y diminui)
                    needsUpdate = true;
                }
                if (keyEvent && keyEvent->code == sf::Keyboard::Key::S) {
                    offsetY += moveSpeed; // S move a câmera para baixo
                    needsUpdate = true;
                }
                if (keyEvent && keyEvent->code == sf::Keyboard::Key::D) {
                    offsetX += moveSpeed; // D move para a direita
                    needsUpdate = true;
                }
                if (keyEvent && keyEvent->code == sf::Keyboard::Key::A) {
                    offsetX -= moveSpeed; // A move para a esquerda
                    needsUpdate = true;
                }
            }
        }

        if (needsUpdate) {
            // Buffer de pixels (Largura * Altura * 4 canais RGBA)
            std::vector<std::uint8_t> pixelBuffer(WIDTH * HEIGHT * 4);

            auto start = std::chrono::high_resolution_clock::now();
            // Varre todos os pixels da tela
            for (int y = 0; y < HEIGHT; ++y) {
                for (int x = 0; x < WIDTH; ++x) {

                    float nx = (x + offsetX) * scale;
                    float ny = (y + offsetY) * scale;

                    float noiseValue = (x < WIDTH / 2) ?
                        pn.noise(nx, ny) :
                        pn.fBm(nx, ny, octaves, persistence, lacunarity);

                    float normalized = (noiseValue + 1.0) / 2.0;

                    float degrees = 12.0;
                    float posterized = fastFloor(normalized * degrees) / degrees;

                    // Converte o retorno (assumido [-1, 1]) para o espaço de cor [0, 255]
                    int colorValue = (int)(posterized * 255.0);

                    if (colorValue < 0) colorValue = 0;
                    if (colorValue > 255) colorValue = 255;

                    // Mapeamento analítico do espaço bidimensional (x, y) para o índice do arranjo unidimensional
                    int index = (y * WIDTH + x) * 4;

                    // Escrita incisiva e direta nos bytes da memória RAM (operação de ínfima latência)
                    pixelBuffer[index + 0] = colorValue; // Canal Primário (Red)
                    pixelBuffer[index + 1] = colorValue; // Canal Secundário (Green)
                    pixelBuffer[index + 2] = colorValue; // Canal Terciário (Blue)
                    pixelBuffer[index + 3] = 255;        // Canal de Opacidade (Alpha estritamente opaco)
                }
            }

            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> ms_double = end - start;
            printf("Tempo de geracao: %f ms\n", ms_double.count());

            // Transferência em bloco da matriz de pixels da Memória Principal para a Memória de Vídeo (VRAM)
            texture.update(pixelBuffer.data());

            needsUpdate = false;

            // Após qualquer mudança de parâmetro:
            printf("scale=%.4f | octaves=%d | persistence=%.2f | lacunarity=%.2f | fBm=%s\n",
                scale, octaves, persistence, lacunarity, fBmOn ? "ON" : "OFF");
        }

        window.clear();
        window.draw(sprite);

        // SÓ CONSTROÍ E DESENHA SE O DEBUG ESTIVER ATIVADO
        if (showDebug) {
            // ==========================================
            // DEBUG VISUAL: DESENHANDO A GRADE E VETORES
            // ==========================================
            float cellSize = 1.0f / scale;

            sf::VertexArray gridLines(sf::PrimitiveType::Lines);
            sf::Color gridColor(255, 255, 255, 50);

            for (float x = 0; x <= WIDTH; x += cellSize) {
                gridLines.append(sf::Vertex{ sf::Vector2f(x, 0.f), gridColor });
                gridLines.append(sf::Vertex{ sf::Vector2f(x, (float)HEIGHT), gridColor });
            }
            for (float y = 0; y <= HEIGHT; y += cellSize) {
                gridLines.append(sf::Vertex{ sf::Vector2f(0.f, y), gridColor });
                gridLines.append(sf::Vertex{ sf::Vector2f((float)WIDTH, y), gridColor });
            }
            window.draw(gridLines);

            sf::VertexArray gradients(sf::PrimitiveType::Lines);

            const float G2D[8][2] = {
                {1.0, 0.0}, {-1.0, 0.0}, {0.0, 1.0}, {0.0, -1.0},
                {0.707106, 0.707106}, {-0.707106, 0.707106},
                {0.707106, -0.707106}, {-0.707106, -0.707106}
            };

            for (float y = 0; y <= HEIGHT; y += cellSize) {
                for (float x = 0; x <= WIDTH; x += cellSize) {
                    int i = (int)((x + offsetX) * scale) & 255;
                    int j = (int)((y + offsetY) * scale) & 255;

                    int hash = pn.getHash(i, j);
                    int h = hash & 7;

                    float gx = G2D[h][0];
                    float gy = G2D[h][1];

                    sf::Vector2f start(x, y);
                    sf::Vector2f end((float)(x + gx * (cellSize * 0.4)), (float)(y + gy * (cellSize * 0.4)));

                    gradients.append(sf::Vertex{ start, sf::Color(255, 0, 0) });
                    gradients.append(sf::Vertex{ end, sf::Color(255, 0, 0) });

                    const float PI = 3.14159265f;
                    float arrowLength = cellSize * 0.1f;
                    float arrowAngle = PI / 6.0f;
                    float angle = atan2((float)gy, (float)gx);

                    float leftAngle = angle + PI - arrowAngle;
                    sf::Vector2f leftBarb(
                        end.x + cos(leftAngle) * arrowLength,
                        end.y + sin(leftAngle) * arrowLength
                    );

                    float rightAngle = angle + PI + arrowAngle;
                    sf::Vector2f rightBarb(
                        end.x + cos(rightAngle) * arrowLength,
                        end.y + sin(rightAngle) * arrowLength
                    );

                    gradients.append(sf::Vertex{ end, sf::Color(255, 0, 0) });
                    gradients.append(sf::Vertex{ leftBarb, sf::Color(255, 0, 0) });

                    gradients.append(sf::Vertex{ end, sf::Color(255, 0, 0) });
                    gradients.append(sf::Vertex{ rightBarb, sf::Color(255, 0, 0) });
                }
            }

            window.draw(gradients);

            // ==========================================
            // DEBUG ANALÍTICO: CORTE TRANSVERSAL INTERATIVO
            // ==========================================

            // 1. O plano de corte acompanha o eixo Y do mouse
            float cutY = (float)(HEIGHT / 2); // Posição padrão caso o mouse esteja fora

            sf::Vector2i mousePos = sf::Mouse::getPosition(window);

            // Verifica se o ponteiro está dentro dos limites da janela
            if (mousePos.x >= 0 && mousePos.x < (int)WIDTH && mousePos.y >= 0 && mousePos.y < (int)HEIGHT) {
                cutY = (float)mousePos.y; // Atualiza a linha de corte para o mouse
            }

            // Desenha a "linha de corte" vermelha em cima do mapa 2D
            sf::VertexArray cutLine(sf::PrimitiveType::Lines);
            cutLine.append(sf::Vertex{ sf::Vector2f(0.f, cutY), sf::Color(0, 255, 0, 200) });
            cutLine.append(sf::Vertex{ sf::Vector2f((float)WIDTH, cutY), sf::Color(0, 255, 0, 200) });
            window.draw(cutLine);

            // 2. Prepara o gráfico do perfil do terreno
            sf::VertexArray profileGraph(sf::PrimitiveType::LineStrip);

            // Onde o gráfico será desenhado na tela? (Na parte inferior)
            float graphBaseY = HEIGHT - 80.0f;
            float amplitude = 60.0f;

            // 3. Caminha sobre o eixo X, extraindo a altura
            for (float x = 0; x <= WIDTH; x += 1.0f) {
                float nx = (x + offsetX) * scale;
                float ny = (cutY + offsetY) * scale; // ny agora varia conforme você move o mouse!

                float noiseValue = (x < WIDTH / 2) ?
                    pn.noise(nx, ny) :
                    pn.fBm(nx, ny, octaves, persistence, lacunarity);

                float plotY = graphBaseY - (float)(noiseValue * amplitude);
                profileGraph.append(sf::Vertex{ sf::Vector2f(x, plotY), sf::Color(255, 255, 0) });
            }

            // Desenha a linha de base (Zero)
            sf::VertexArray zeroLine(sf::PrimitiveType::Lines);
            zeroLine.append(sf::Vertex{ sf::Vector2f(0.f, graphBaseY), sf::Color(255, 165, 0, 100) });
            zeroLine.append(sf::Vertex{ sf::Vector2f((float)WIDTH, graphBaseY), sf::Color(255, 165, 0, 100) });

            window.draw(zeroLine);
            window.draw(profileGraph);
            // ==========================================
        }

        window.display();
    }

    return 0;
}
