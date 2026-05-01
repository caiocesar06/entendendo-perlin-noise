# Perlin Noise: Fundamentos Matemáticos

> **Como ler este documento.** O texto foi escrito para ser lido linearmente. Cada seção depende das anteriores. Os termos técnicos são introduzidos com definição no momento em que aparecem pela primeira vez. As fontes estão listadas ao final com indicação do que cada uma cobre.

---

## 1. O Problema: Por Que Ruído Branco Não Funciona

Antes de entender o Perlin Noise, é necessário entender o que ele resolve.

Suponha que você queira gerar uma textura que pareça natural — uma nuvem, um terreno montanhoso, uma superfície de água. A primeira tentativa ingênua seria atribuir um valor aleatório independente a cada pixel. Esse processo é chamado de **ruído branco** (*white noise*).

O ruído branco tem uma propriedade indesejável: **ausência de coerência espacial**. Isso significa que o valor em qualquer ponto não tem nenhuma relação com o valor nos pontos vizinhos. O resultado visual é estática de televisão — nenhuma estrutura, nenhuma continuidade.

Formalmente, o ruído branco é um processo estocástico onde, para quaisquer dois pontos $\mathbf{p}_1$ e $\mathbf{p}_2$:

$$\text{Cov}[f(\mathbf{p}_1),\ f(\mathbf{p}_2)] = 0 \quad \text{para } \mathbf{p}_1 \neq \mathbf{p}_2$$

Ou seja, os valores são completamente descorrelacionados. Fenômenos naturais não se comportam assim — em uma nuvem real, regiões próximas tendem a ter densidades parecidas. Existe uma estrutura de longa escala.

**O que se quer:** uma função $f: \mathbb{R}^n \to \mathbb{R}$ que seja:

1. **Contínua** — sem saltos abruptos entre pontos vizinhos
2. **Determinística com parâmetro de semente** — mesma semente produz o mesmo resultado
3. **Aparentemente aleatória** — sem padrão visualmente repetitivo
4. **Computacionalmente barata** — avaliável em tempo $O(1)$ por ponto

A solução de Ken Perlin, publicada em 1985 no SIGGRAPH, foi o *gradient noise* — ruído baseado em gradientes interpolados sobre uma grade. O que hoje chamamos de **Perlin Noise**.

---

## 2. A Ideia Central: Gradientes em uma Grade

A intuição fundamental do Perlin Noise é a seguinte:

> Em vez de atribuir valores aleatórios a cada ponto contínuo, atribuímos **vetores aleatórios** a uma grade de inteiros, e depois **interpolamos** suavemente entre eles.

Isso garante continuidade por construção: a interpolação suave entre vetores de grade vizinhos sempre produz transições suaves no espaço contínuo.

### 2.1 A Grade (*Lattice*)

Considere o plano $\mathbb{R}^2$ dividido em uma grade de células unitárias. Os vértices dessa grade são os pontos inteiros $\mathbb{Z}^2 = \{(i, j) \mid i, j \in \mathbb{Z}\}$.

Para um ponto contínuo $\mathbf{p} = (x, y) \in \mathbb{R}^2$, identificamos:

- **A célula que contém $\mathbf{p}$:** os índices inteiros do canto inferior-esquerdo são
$$i = \lfloor x \rfloor, \quad j = \lfloor y \rfloor$$
onde $\lfloor \cdot \rfloor$ denota o piso (*floor*), ou seja, o maior inteiro menor ou igual ao argumento.

- **A posição relativa dentro da célula:**
$$u = x - \lfloor x \rfloor, \quad v = y - \lfloor y \rfloor$$
com $u, v \in [0, 1)$.

**Analogia:** imagine que o plano é um mapa quadriculado como papel milimetrado. O índice $(i, j)$ diz em qual quadrado você está. Os valores $(u, v)$ dizem exatamente onde dentro daquele quadrado você se encontra — $(0.5, 0.5)$ é o centro, $(0.0, 0.0)$ é o canto inferior-esquerdo.

Os quatro cantos da célula que contém $\mathbf{p}$ são:

| Canto | Coordenadas |
|-------|-------------|
| $C_{00}$ | $(i,\; j)$ |
| $C_{10}$ | $(i+1,\; j)$ |
| $C_{01}$ | $(i,\; j+1)$ |
| $C_{11}$ | $(i+1,\; j+1)$ |

---

## 3. Tabela de Permutação — Pseudoaleatoriedade Determinística

O algoritmo precisa atribuir um vetor gradiente a cada vértice da grade de forma que:

1. O mesmo vértice sempre receba o mesmo vetor (determinismo)
2. Vetores em vértices próximos pareçam independentes (aparência aleatória)
3. O cálculo seja $O(1)$, sem acesso a memória dinâmica

A solução de Perlin é uma **tabela de permutação** $P$ — um array de 256 inteiros, cada valor de 0 a 255 aparecendo exatamente uma vez, em ordem embaralhada.

### 3.1 Construção da Tabela

A tabela é construída uma única vez, com base em uma **semente** (*seed*) $s \in \mathbb{Z}$:

```
P[0..255] = embaralhamento de [0, 1, 2, ..., 255] com semente s
```

O embaralhamento padrão usa o algoritmo **Fisher-Yates** (também chamado de Knuth shuffle), que garante distribuição uniforme sobre todas as $256!$ permutações possíveis:

```
para i de 255 até 1:
    j = inteiro_aleatório_com_semente(0, i)
    troca P[i] com P[j]
```

A tabela é então **duplicada** para 512 elementos:

$$P[k] = P[k \bmod 256] \quad \text{para } k \in [0, 511]$$

**Por que duplicar?** O índice composto que usaremos tem a forma $P[i] + j$. Como $P[i] \in [0, 255]$ e $j \in [0, 255]$, o índice resultante pode chegar a $255 + 255 = 510$. Com a duplicação, nunca há *overflow* de índice sem precisar de operações de módulo adicionais.

### 3.2 A Função Hash

Para cada canto da célula, calculamos um hash inteiro usando dois acessos à tabela:

$$h_{00} = P\bigl[P[i \mathbin{\&} 255] + (j \mathbin{\&} 255)\bigr]$$
$$h_{10} = P\bigl[P[(i+1) \mathbin{\&} 255] + (j \mathbin{\&} 255)\bigr]$$
$$h_{01} = P\bigl[P[i \mathbin{\&} 255] + (j+1 \mathbin{\&} 255)\bigr]$$
$$h_{11} = P\bigl[P[(i+1) \mathbin{\&} 255] + (j+1 \mathbin{\&} 255)\bigr]$$

onde $\mathbin{\&} 255$ é equivalente a $\bmod 256$ para inteiros não-negativos (operação de bitmask, mais eficiente em hardware).

**Propriedade importante:** esta função é **determinística** — dados os mesmos $(i, j)$ e a mesma tabela $P$, o hash é sempre o mesmo. A aparência aleatória vem do embaralhamento inicial de $P$, não de chamadas a um gerador de números aleatórios em tempo de execução.

**Analogia:** imagine que antes de começar você sorteia 256 bolinhas numeradas de 0 a 255 e as coloca em uma fileira. Essa fileira é a tabela $P$. Para qualquer vértice $(i, j)$, você sempre consulta a mesma posição na fileira — o resultado é sempre o mesmo, mas parece aleatório porque a fileira foi sorteada.

---

## 4. Vetores Gradiente

O hash $h$ de cada canto é usado para selecionar um **vetor gradiente** $\mathbf{g}$ de um conjunto predefinido.

### 4.1 O Conjunto de Gradientes em 2D

Em duas dimensões, Perlin usa 8 vetores unitários uniformemente distribuídos:

$$G_{2D} = \left\{ (\pm 1, 0),\ (0, \pm 1),\ \left(\pm \frac{1}{\sqrt{2}}, \pm \frac{1}{\sqrt{2}}\right) \right\}$$

A seleção é feita por:

$$\mathbf{g}(h) = G_{2D}[h \bmod 8]$$

**Por que vetores fixos em vez de vetores contínuos?** Vetores contínuos exigiriam funções trigonométricas (seno/cosseno), que são computacionalmente caras. Os 8 vetores discretos são suficientes para produzir ruído visualmente convincente e permitem que a seleção seja feita com uma simples operação de índice.

> **Nota técnica:** a versão de 2002 de Perlin usa 12 vetores em 3D para eliminar um viés direcional que aparece com 8. Em 2D, os 8 vetores são matematicamente adequados para a maioria das aplicações.

### 4.2 O Vetor de Distância

Para cada canto $C_{ij}$, calculamos o **vetor de distância** $\boldsymbol{\delta}_{ij}$ do canto até o ponto $\mathbf{p}$:

$$\boldsymbol{\delta}_{00} = (u,\; v)$$
$$\boldsymbol{\delta}_{10} = (u - 1,\; v)$$
$$\boldsymbol{\delta}_{01} = (u,\; v - 1)$$
$$\boldsymbol{\delta}_{11} = (u - 1,\; v - 1)$$

---

## 5. O Produto Escalar — Combinando Gradiente e Distância

Para cada canto, calculamos o **produto escalar** (*dot product*) entre o vetor gradiente e o vetor de distância:

$$d_{ij} = \mathbf{g}_{ij} \cdot \boldsymbol{\delta}_{ij}$$

Em coordenadas, se $\mathbf{g} = (g_x, g_y)$ e $\boldsymbol{\delta} = (\delta_x, \delta_y)$:

$$d = g_x \cdot \delta_x + g_y \cdot \delta_y$$

### 5.1 Por Que Produto Escalar?

O produto escalar $\mathbf{a} \cdot \mathbf{b} = \|\mathbf{a}\| \|\mathbf{b}\| \cos\theta$, onde $\theta$ é o ângulo entre os vetores. Como os gradientes são unitários ($\|\mathbf{g}\| = 1$), temos:

$$d_{ij} = \|\boldsymbol{\delta}_{ij}\| \cos\theta$$

O valor de $d_{ij}$ é:
- **Positivo** quando o gradiente aponta em direção ao ponto ($\cos\theta > 0$, ângulo agudo)
- **Zero** quando o gradiente é perpendicular ao vetor de distância
- **Negativo** quando o gradiente aponta para longe do ponto ($\cos\theta < 0$, ângulo obtuso)

Isso cria a estrutura de colinas e vales do ruído: regiões onde os gradientes "apontam para você" têm valores altos; regiões onde apontam "para longe" têm valores baixos.

**Analogia:** imagine que cada canto da grade é uma fonte de vento com direção fixa (o gradiente). O produto escalar mede quanto desse vento você "sente" dependendo de onde você está em relação à fonte. Próximo e na direção do vento: valor alto. Atrás da fonte: valor baixo.

O resultado são quatro valores escalares, um por canto:

$$d_{00},\quad d_{10},\quad d_{01},\quad d_{11} \in \mathbb{R}$$

---

## 6. A Função Fade — Garantindo Suavidade $C^2$

Antes de interpolar os quatro valores, é necessário suavizar os parâmetros de interpolação $u$ e $v$. Interpolar linearmente produziria descontinuidades na **derivada** nas bordas das células — o padrão de grade ficaria visível.

### 6.1 O Problema da Interpolação Linear

Se usássemos $u$ e $v$ diretamente como parâmetros de interpolação, a derivada da função de ruído seria descontínua nas bordas das células. Isso ocorre porque a derivada de $\text{lerp}(a, b, t) = a + t(b-a)$ em relação a $t$ é constante: $b - a$. Na borda de uma célula, os valores $a$ e $b$ mudam abruptamente, criando uma "quina" na superfície — um artefato visual perceptível.

### 6.2 A Função Quintica de Perlin (2002)

A solução é aplicar uma **função de suavização** $f: [0,1] \to [0,1]$ aos parâmetros antes da interpolação. Perlin, em 1985, usou a função cúbica de Hermite (também chamada de *smoothstep*):

$$f_{1985}(t) = 3t^2 - 2t^3$$

Em 2002, Perlin propôs uma função quintica superior:

$$\boxed{f(t) = 6t^5 - 15t^4 + 10t^3}$$

### 6.3 Por Que a Quintica é Superior

A escolha da quintica não é arbitrária. Ela satisfaz seis condições de contorno:

| Condição | Verificação |
|----------|-------------|
| $f(0) = 0$ | $6(0)^5 - 15(0)^4 + 10(0)^3 = 0$ ✓ |
| $f(1) = 1$ | $6 - 15 + 10 = 1$ ✓ |
| $f'(0) = 0$ | $f'(t) = 30t^4 - 60t^3 + 30t^2$; em $t=0$: $0$ ✓ |
| $f'(1) = 0$ | $30 - 60 + 30 = 0$ ✓ |
| $f''(0) = 0$ | $f''(t) = 120t^3 - 180t^2 + 60t$; em $t=0$: $0$ ✓ |
| $f''(1) = 0$ | $120 - 180 + 60 = 0$ ✓ |

As condições $f'(0) = f'(1) = 0$ garantem que a função de ruído seja de classe $C^1$ (derivada primeira contínua) nas bordas das células. As condições $f''(0) = f''(1) = 0$ garantem $C^2$ (derivada segunda contínua), o que elimina artefatos de curvatura que a cúbica deixava.

**Em linguagem técnica:** a quintica produz uma interpolação $C^2$, enquanto a cúbica produzia apenas $C^1$. A diferença é perceptível em frequências altas do ruído — superfícies com muitos detalhes.

**Analogia:** imagine uma estrada que passa suavemente de plana para uma curva. Uma estrada $C^1$ tem curvas suaves, mas se você fosse um carro, sentiria um "arranção" no volante na transição. Uma estrada $C^2$ tem transições tão graduais que nem percebe a mudança.

Definimos:

$$\tilde{u} = f(u), \qquad \tilde{v} = f(v)$$

---

## 7. Interpolação Bilinear — Combinando os Quatro Cantos

Com os quatro dot products $d_{00}, d_{10}, d_{01}, d_{11}$ e os parâmetros suavizados $\tilde{u}, \tilde{v}$, realizamos uma **interpolação bilinear** — interpolação linear em dois eixos.

### 7.1 A Função de Interpolação Linear

A interpolação linear entre dois valores $a$ e $b$ com parâmetro $t \in [0,1]$ é:

$$\text{lerp}(a, b, t) = a + t(b - a) = (1-t)a + tb$$

Quando $t = 0$, o resultado é $a$. Quando $t = 1$, o resultado é $b$. Para $t$ intermediário, o resultado é uma média ponderada.

### 7.2 A Interpolação em Duas Etapas

**Etapa 1 — Interpolação horizontal** (ao longo do eixo $x$):

$$x_1 = \text{lerp}(d_{00},\; d_{10},\; \tilde{u})$$
$$x_2 = \text{lerp}(d_{01},\; d_{11},\; \tilde{u})$$

$x_1$ é o valor interpolado na linha superior da célula. $x_2$ é o valor interpolado na linha inferior.

**Etapa 2 — Interpolação vertical** (ao longo do eixo $y$):

$$\text{noise}(x, y) = \text{lerp}(x_1,\; x_2,\; \tilde{v})$$

O resultado final é um único valor escalar.

### 7.3 Expandindo a Expressão

Substituindo as definições de lerp:

$$\text{noise}(x,y) = (1-\tilde{v})\bigl[(1-\tilde{u})\,d_{00} + \tilde{u}\,d_{10}\bigr] + \tilde{v}\bigl[(1-\tilde{u})\,d_{01} + \tilde{u}\,d_{11}\bigr]$$

Expandindo:

$$\text{noise}(x,y) = (1-\tilde{u})(1-\tilde{v})\,d_{00} + \tilde{u}(1-\tilde{v})\,d_{10} + (1-\tilde{u})\tilde{v}\,d_{01} + \tilde{u}\tilde{v}\,d_{11}$$

Esta é a forma bilinear canônica: cada $d_{ij}$ é ponderado pelo produto das distâncias complementares ao canto oposto.

**Por que o resultado está em $[-1, 1]$?** Os vetores gradiente são unitários e os vetores de distância têm norma máxima $\sqrt{2}$ (diagonal de uma célula unitária). O produto escalar máximo é $\|\mathbf{g}\| \cdot \|\boldsymbol{\delta}\| = 1 \cdot \sqrt{2}/2 \approx 0.707$ para os cantos diagonais. Na prática, a interpolação e a distribuição dos gradientes fazem o valor ficar aproximadamente em $[-0.7, 0.7]$, não exatamente $[-1, 1]$.

---

## 8. O Algoritmo Completo

Reunindo todas as etapas:

**Entrada:** ponto $(x, y) \in \mathbb{R}^2$ e tabela de permutação $P$

**Saída:** valor escalar $\text{noise}(x, y) \approx [-1, 1]$

```
1. LOCALIZAR na grade:
   i ← ⌊x⌋ & 255
   j ← ⌊y⌋ & 255
   u ← x − ⌊x⌋
   v ← y − ⌊y⌋

2. HASH dos quatro cantos:
   aa ← P[P[i  ] + j  ]
   ba ← P[P[i+1] + j  ]
   ab ← P[P[i  ] + j+1]
   bb ← P[P[i+1] + j+1]

3. GRADIENTES e DOT PRODUCTS:
   d₀₀ ← grad(aa) · (u,   v  )
   d₁₀ ← grad(ba) · (u−1, v  )
   d₀₁ ← grad(ab) · (u,   v−1)
   d₁₁ ← grad(bb) · (u−1, v−1)

4. SUAVIZAÇÃO (fade quintica):
   ũ ← 6u⁵ − 15u⁴ + 10u³
   ṽ ← 6v⁵ − 15v⁴ + 10v³

5. INTERPOLAÇÃO bilinear:
   x₁ ← lerp(d₀₀, d₁₀, ũ)
   x₂ ← lerp(d₀₁, d₁₁, ũ)
   retorna lerp(x₁, x₂, ṽ)
```

---

## 9. Fractal Brownian Motion — Ruído em Múltiplas Escalas

O Perlin Noise básico gera uma superfície com uma única escala de variação. Terrenos reais têm estrutura em múltiplas escalas: grandes montanhas, colinas menores, saliências pequenas, detalhes microscópicos. Isso é modelado pela **Fractal Brownian Motion** (fBm).

### 9.1 Conceito de Oitavas

Uma **oitava** (*octave*) é uma camada de ruído com frequência e amplitude específicas. A fBm soma $n$ oitavas, cada uma com o dobro da frequência e metade da amplitude da anterior:

$$\text{fBm}(x, y) = \sum_{k=0}^{n-1} a^k \cdot \text{noise}\bigl(x \cdot l^k,\; y \cdot l^k\bigr)$$

onde:
- $a \in (0, 1)$ é a **persistência** (*persistence*) — controla quanto cada oitava contribui (tipicamente $a = 0.5$)
- $l > 1$ é a **lacunaridade** (*lacunarity*) — controla o fator de aumento de frequência (tipicamente $l = 2.0$)
- $n$ é o número de oitavas

Para normalizar o resultado em $[-1, 1]$, divide-se pela soma geométrica das amplitudes:

$$\text{fBm}(x, y) = \frac{\displaystyle\sum_{k=0}^{n-1} a^k \cdot \text{noise}\bigl(x \cdot l^k,\; y \cdot l^k\bigr)}{\displaystyle\sum_{k=0}^{n-1} a^k} = \frac{\displaystyle\sum_{k=0}^{n-1} a^k \cdot \text{noise}\bigl(x \cdot l^k,\; y \cdot l^k\bigr)}{\dfrac{1 - a^n}{1 - a}}$$

### 9.2 Interpretação dos Parâmetros

| Parâmetro | Efeito de aumentar | Efeito de diminuir |
|-----------|-------------------|-------------------|
| $n$ (oitavas) | Mais detalhes finos | Superfície mais suave |
| $a$ (persistência) | Detalhes finos mais proeminentes | Superfície mais suave e arredondada |
| $l$ (lacunaridade) | Saltos de frequência maiores | Transição gradual entre escalas |

---

## 10. Propriedades Matemáticas do Ruído

### 10.1 Isotropia Estatística

O Perlin Noise é **estatisticamente isotrópico**: sua distribuição de valores não tem direção preferencial. Isso significa que o espectro de potência do ruído é aproximadamente circular (em 2D) ou esférico (em 3D).

Na prática, a isotropia é aproximada, não exata, porque os gradientes são escolhidos de um conjunto discreto e a grade tem simetria quadrada, não circular.

### 10.2 Espectro de Frequências

O Perlin Noise básico é um **ruído de banda limitada**: sua energia está concentrada em torno de uma frequência característica determinada pela escala da grade. A função de autocorrelação decai com a distância — pontos próximos são mais similares que pontos distantes.

A fBm, ao somar oitavas com razão $l$, produz um espectro de potência que decai como lei de potência:

$$S(f) \propto f^{-\beta}$$

onde $\beta = -2\log_l(a)$. Para $a = 0.5$ e $l = 2$, temos $\beta = 2$, o que corresponde ao espectro de **ruído rosa** (*pink noise* ou ruído $1/f^2$) — característico de muitos fenômenos naturais como topografia terrestre.

---

## 11. Limitações e Artefatos

### 11.1 Artefatos de Grade

Como a função é calculada sobre uma grade regular, o Perlin Noise tem um viés visual em ângulos de $0°$, $45°$ e $90°$ em relação à grade. Em escala ampliada, é possível perceber um leve padrão quadriculado. O Simplex Noise (2001) resolve este problema usando uma grade simplex (triangular em 2D), que tem simetria mais uniforme.

### 11.2 Range Não Uniforme

O valor teórico do ruído está em $[-1, 1]$, mas a distribuição não é uniforme — ela tende a se concentrar em torno de zero (distribuição aproximadamente gaussiana com média zero). Isso ocorre porque a interpolação suaviza os extremos. Para mapear o ruído para $[0, 1]$, a conversão $c = (\text{noise} + 1) / 2$ é uma aproximação, não uma garantia de cobertura uniforme do intervalo.

### 11.3 Escalabilidade com Dimensão

O custo computacional do Perlin Noise em $n$ dimensões é $O(2^n)$ — o número de cantos da hipercélula cresce exponencialmente. Em 3D são 8 cantos, em 4D são 16. O Simplex Noise resolve isso com custo $O(n^2)$ por ponto.

---

## 12. Resumo das Equações

Para consulta rápida, as equações centrais do algoritmo:

**Localização na grade:**
$$i = \lfloor x \rfloor,\quad j = \lfloor y \rfloor,\quad u = x - i,\quad v = y - j$$

**Hash dos cantos:**
$$h_{ij} = P\bigl[P[i + \Delta i] + j + \Delta j\bigr], \quad (\Delta i, \Delta j) \in \{0,1\}^2$$

**Produto escalar:**
$$d_{ij} = \mathbf{g}(h_{ij}) \cdot \boldsymbol{\delta}_{ij}$$

**Função fade (quintica, Perlin 2002):**
$$f(t) = 6t^5 - 15t^4 + 10t^3$$

**Interpolação bilinear:**
$$\text{noise}(x,y) = \text{lerp}\bigl(\text{lerp}(d_{00}, d_{10}, f(u)),\; \text{lerp}(d_{01}, d_{11}, f(u)),\; f(v)\bigr)$$

**fBm com $n$ oitavas:**
$$\text{fBm}(x,y) = \frac{\sum_{k=0}^{n-1} a^k \cdot \text{noise}(l^k x,\; l^k y)}{\sum_{k=0}^{n-1} a^k}$$

---

## Fontes e Referências

As afirmações deste documento podem ser verificadas nas fontes abaixo, organizadas por tópico.

---

### Fontes Primárias — Papers Originais

**[1] PERLIN, Ken. An image synthesizer.**
*Proceedings of SIGGRAPH 1985*, v. 19, n. 3, p. 287–296, 1985.
DOI: [10.1145/325165.325247](https://doi.org/10.1145/325165.325247)
> Artigo fundador. Apresenta o algoritmo original, a motivação visual e a primeira descrição da tabela de permutação. A função fade usada aqui é a cúbica, não a quintica.

**[2] PERLIN, Ken. Improving noise.**
*Proceedings of SIGGRAPH 2002*, p. 681–682, 2002.
DOI: [10.1145/566570.566636](https://doi.org/10.1145/566570.566636)
> Versão revisada. Introduz a função fade quintica $f(t) = 6t^5 - 15t^4 + 10t^3$, justifica a mudança de $C^1$ para $C^2$, e apresenta novos vetores gradiente para 3D. São apenas 2 páginas — leitura obrigatória.

---

### Fontes Secundárias — Explicações e Análises

**[3] GUSTAVSON, Stefan. Simplex noise demystified.**
Linköping University, 2005.
Disponível em: [researchgate.net/publication/216813608](https://www.researchgate.net/publication/216813608_Simplex_noise_demystified)
> A explicação mais didática do algoritmo do Perlin Noise passo a passo, com a construção matemática de cada etapa. Seções 2 e 3 cobrem tudo descrito neste documento. É mais acessível que o paper original de Perlin.

**[4] EBERT, David S. et al. Texturing and Modeling: A Procedural Approach.** 3. ed. Morgan Kaufmann, 2002.
> Referência enciclopédica. Capítulos 2 e 5 cobrem ruído coerente, fBm e as propriedades espectrais do Perlin Noise. A fórmula da fBm e a discussão de espectro de potência em lei de potência estão no Capítulo 5.

**[5] ARNERIN, Reynald. A journey in a procedural volume: optimization and filtering of Perlin noise.**
INRIA, 2009.
Disponível em: [inria.hal.science/inria-00598443](https://inria.hal.science/inria-00598443)
> Análise aprofundada das propriedades espectrais e de isotropia do Perlin Noise, com foco em otimizações para GPU. Relevante para a seção 10 (propriedades matemáticas).

---

### Fontes de Implementação — Não Acadêmicas

**[6] BEVINS, Adrian. Understanding Perlin Noise.**
*adrianb.io*, 2014.
Disponível em: [adrianb.io/2014/08/09/perlinnoise.html](https://adrianb.io/2014/08/09/perlinnoise.html)
> Melhor guia prático de implementação passo a passo. Não é fonte acadêmica — não citar no relatório LaTeX como referência técnica. Usar como ferramenta de estudo para implementação.

**[7] QUILEZ, Inigo. Gradient noise derivatives.**
*iquilezles.org*, 2019.
Disponível em: [iquilezles.org/articles/gradientnoise](https://iquilezles.org/articles/gradientnoise/)
> Análise técnica rigorosa da diferença entre value noise e gradient noise (Perlin), com derivação das expressões analíticas. Matematicamente correto, mas sem peer review formal.

**[8] QUILEZ, Inigo. Smooth Hermite interpolation.**
*iquilezles.org*.
Disponível em: [iquilezles.org/articles/smoothsteps](https://iquilezles.org/articles/smoothsteps/)
> Análise comparativa entre smoothstep (cúbica), quintica e outras funções de suavização. Inclui gráficos de $f$, $f'$ e $f''$. Útil para verificar a seção 6 deste documento.

---

### Referências Gerais — Contexto e PRNGs

**[9] KNUTH, Donald E. The Art of Computer Programming, Vol. 2: Seminumerical Algorithms.** 3. ed. Addison-Wesley, 1997.
> Seção 3.2 cobre o algoritmo Fisher-Yates e a teoria de permutações uniformes. Seção 3.1 define formalmente o que é um PRNG. Referência canônica para a seção 3.1 deste documento.

**[10] MARSAGLIA, George. Xorshift RNGs.**
*Journal of Statistical Software*, v. 8, n. 14, 2003.
DOI: [10.18637/jss.v008.i14](https://doi.org/10.18637/jss.v008.i14)
> Define o Xorshift — um PRNG simples e eficiente. Útil para entender o que é um PRNG antes de estudar a tabela de permutação de Perlin (que é uma alternativa determinística a PRNGs clássicos).

**[11] PATEL, Amit. Making maps with noise functions.**
*Red Blob Games*, 2022.
Disponível em: [redblobgames.com/maps/terrain-from-noise](https://www.redblobgames.com/maps/terrain-from-noise)
> Visualizações interativas do efeito dos parâmetros de frequência, amplitude e número de oitavas. Não é fonte acadêmica — usar como ferramenta visual de estudo.
