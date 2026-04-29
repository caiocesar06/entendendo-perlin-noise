#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <numeric>
#include <random>
#include <algorithm>

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
    double fade(double t) {
        return t * t * t * (t * (6 * t - 15) + 10);
        // return t * t * (3 - 2 * t);
    }

    // Interpolação linear
    double lerp(double t, double a, double b) {
        return a + t * (b - a);
    }

    // Seleção do gradiente e produto escalar
    double grad(int hash, double x, double y) {
        int h = hash & 7;
        double u = h < 4 ? x : y;
        double v = h < 4 ? y : x;

        // Simula o produto escalar com direções fixas
        return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
    }

    // Função principal do ruído de uma oitava
    double noise(double x, double y) {
        // 1. Encontrar coordenadas da célula (floor)
        int i = (int)floor(x) & 255;
        int j = (int)floor(y) & 255;

        // 2. Posição relativa dentro da célula [0, 1)
        x -= floor(x);
        y -= floor(y);

        // 3. Aplicar a função fade em u e v
        double u = fade(x);
        double v = fade(y);

        // 4. Pegar os hashes dos 4 cantos da célula
        int h00 = p[p[i] + j];
        int h01 = p[p[i] + j + 1];
        int h10 = p[p[i + 1] + j];
        int h11 = p[p[i + 1] + j + 1];

        // 5. Calcular o dot product de cada canto
        double x1 = lerp(u, grad(h00, x, y), grad(h10, x - 1, y));
        double x2 = lerp(u, grad(h01, x, y - 1), grad(h11, x - 1, y - 1));

        // 6. Interpolar tudo de forma bilinear
        return lerp(v, x1, x2);
    }

    // Função de múltiplas escalas (Fractal Brownian Motion)
    double fBm(double x, double y, int octaves, double persistence, double lacunarity) {
        double total = 0.0;
        double amplitude = 1.0;
        double frequency = 1.0;
        double maxValue = 0.0;

        for (int i = 0; i < octaves; ++i) {
            total += noise(x * frequency, y * frequency) * amplitude;
            maxValue += amplitude;
            amplitude *= persistence;
            frequency *= lacunarity;
        }

        return total / maxValue;
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
    double persistence = 0.5;
    double lacunarity = 2.0;
    double scale = 0.05;

    bool needsUpdate = true;

    while (window.isOpen()) {
        while (auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            // Pressione Espaço para gerar uma nova semente e forçar o redesenho
            if (event->is<sf::Event::KeyPressed>()) {
                auto* keyEvent = event->getIf<sf::Event::KeyPressed>();
                if (keyEvent && keyEvent->code == sf::Keyboard::Key::Space) {
                    pn = PerlinNoise(rand());
                    needsUpdate = true;
                }

                // Pressione ESC para fechar a janela
                if (keyEvent && keyEvent->code == sf::Keyboard::Key::Escape) {
                    window.close();
                }
            }
        }

        if (needsUpdate) {
            // Varre todos os pixels da tela
            for (int y = 0; y < HEIGHT; ++y) {
                for (int x = 0; x < WIDTH; ++x) {

                    double nx = x * scale;
                    double ny = y * scale;

                    double noiseValue = pn.fBm(nx, ny, octaves, persistence, lacunarity);
                    // double noiseValue = pn.noise(nx, ny);

                    // Converte o retorno (assumido [-1, 1]) para o espaço de cor [0, 255]
                    int colorValue = (int)(((noiseValue + 1.0) / 2.0) * 255.0);

                    if (colorValue < 0) colorValue = 0;
                    if (colorValue > 255) colorValue = 255;

                    image.setPixel(sf::Vector2u(x, y), sf::Color(colorValue, colorValue, colorValue));
                }
            }

            if (texture.loadFromImage(image)) {
                sprite.setTexture(texture);
            }
            needsUpdate = false;
        }

        window.clear();
        window.draw(sprite);
        window.display();
    }

    return 0;
}
