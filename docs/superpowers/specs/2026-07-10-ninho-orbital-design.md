# Ninho Orbital: Mundos Partidos — Especificação de Design

**Data:** 10 de julho de 2026

**Plataforma principal:** Windows desktop x86_64

**Idioma inicial:** português do Brasil, com textos externalizados para futura localização

**Status:** aprovado por delegação autônoma do usuário

## 1. Visão do produto

`Ninho Orbital: Mundos Partidos` é um puzzle de demolição 3D baseado em física. O jogador inspeciona miniplanetas em 360 graus, posiciona um anel de impulso orbital e lança uma pequena equipe de aves contra fortificações construídas por javalis mineradores. Gravidade radial, impactos, juntas quebráveis, propriedades materiais e destroços em órbita formam o espaço de soluções.

O jogo usa a clareza imediata do gênero de lançamento e destruição, mas terá identidade própria. Não haverá estilingue, porcos verdes, ovos roubados, visão lateral fixa, sistema de três estrelas, poderes equivalentes aos personagens conhecidos nem cópia de nomes, silhuetas, sons ou interface de Angry Birds.

O diferencial central é que o espaço 3D interfere na solução: o jogador pode lançar em diferentes planos orbitais, atacar o lado oculto do planeta, usar detritos que circundam o mundo e explorar materiais com comportamento físico distinto.

## 2. Escopo de entrega

O produto inicial completo contém:

- um executável Windows autocontido e um build de desenvolvimento;
- menu principal, configurações, seleção de fase, pausa, reinício e resultados;
- seis fases curtas e rejogáveis em dois miniplanetas visuais;
- um laboratório livre para testar todos os personagens, inimigos e materiais;
- três aves jogáveis;
- três classes de inimigos suínos;
- vinte materiais de construção jogáveis;
- câmera orbital, previsão de trajetória até o primeiro impacto e replay cinematográfico curto;
- pontuação por eficiência, dano estrutural, combinações e restauração do núcleo;
- áudio, VFX, tutorial contextual e suporte completo a mouse e teclado;
- fontes do jogo, fontes dos modelos Blender, scripts de geração de assets, testes e licenças de terceiros.

O primeiro marco é um vertical slice, não uma demo descartável. Ele valida o lançamento, a gravidade radial, o Box3D, uma fortificação, três materiais, uma ave, uma classe de javali, câmera, áudio/VFX e fluxo completo de vitória ou derrota. O conteúdo restante é construído sobre essa base.

Ficam fora da primeira versão: multijogador, editor público de fases, campanha narrativa longa, loja, anúncios, cosméticos, geração procedural de fases, física de fluidos, terreno deformável contínuo e destruição volumétrica arbitrária.

## 3. Direções avaliadas

### 3.1 Ninho orbital — escolhida

Miniplanetas com gravidade radial, lançamento em qualquer plano e detritos orbitais. É a opção mais autoral, usa Box3D de forma indispensável e permite variedade visual com poucos kits modulares. O risco é a legibilidade de profundidade; trajetória, sombras, câmera e feedback de seleção recebem prioridade de produção.

### 3.2 Arquipélago de vapor

Ilhas, reservatórios, flutuabilidade e máquinas de pressão. Oferece espetáculo, mas água, eletricidade e destroços ampliam demais o risco técnico para uma primeira produção autônoma.

### 3.3 Teatro descosido

Cenários artesanais com painéis, cordas, cortes e costuras. É econômico e visualmente distinto, mas reduz a sensação visceral de demolição e exigiria um sistema de edição de conexões mais complexo que o lançamento orbital.

## 4. Loop de jogo

1. A câmera apresenta o planeta e o núcleo drenado.
2. No estado `Inspeção`, o jogador gira e aproxima a câmera para ler estrutura, inimigos e materiais.
3. Ao selecionar o anel, entra em `Mira`: escolhe uma posição permitida na casca de lançamento, orienta o plano tangente e ajusta direção e potência.
4. Uma linha segmentada mostra a trajetória prevista até o primeiro impacto provável.
5. O lançamento inicia; um segundo comando ativa a habilidade da ave.
6. O Box3D resolve colisões, rotação, juntas, quedas e destroços sob gravidade radial.
7. A câmera acompanha a ação e destaca reações em cadeia sem retirar controle por tempo excessivo.
8. Quando os corpos relevantes repousam por 1 segundo, a fase entra em `Avaliação`. Um watchdog de 30 segundos após o lançamento só encerra a resolução quando a habilidade terminou; até lá, projétil ativo ou impacto previsto impede timeout por silêncio.
9. O manifesto avalia objetivos tipados. A configuração padrão exige neutralizar todos os javalis-alvo e desligar todos os extratores obrigatórios; o jogador perde quando não há aves utilizáveis e algum objetivo obrigatório permanece incompleto.
10. O resultado mostra eficiência, dano, maior combinação e opção de replay, reinício ou próxima fase.

Um lançamento deve levar de 10 a 25 segundos. Uma fase deve durar de 2 a 6 minutos na primeira conclusão.

## 5. Espaço físico e controles

Cada fase usa um miniplaneta aproximadamente esférico de 10 a 18 metros de raio. O centro físico permanece próximo da origem para preservar precisão de ponto flutuante.

O mundo Box3D usa gravidade global zero. Antes de cada passo fixo, o jogo aplica aos corpos dinâmicos acordados e elegíveis uma aceleração radial em direção ao centro:

`a(r) = min(g_surface * R² / max(r², (0.6R)²), 18.0) m/s²`

`R` é o raio de referência do planeta e `g_surface` começa em `9.0 m/s²`. Não existe limite inferior: a atração decai com a distância e permite ejeção. Para `r < 1 mm`, a direção é definida como vetor zero. Corpos adormecidos não recebem força até serem acordados por contato/comando, evitando wake permanente; corpos marcados sem gravidade também são excluídos.

A casca segura termina em `3R`. Um alvo é ejetado quando permanece em `r ≥ 4R`, com velocidade radial para fora de pelo menos `2 m/s`, por 0,5 segundo contínuo. Corpos não essenciais são removidos deterministicamente em `6R`; alvos já ejetados permanecem apenas como representação visual. Detritos comuns têm TTL de 20 segundos, limite de 250 fragmentos ativos e culling por prioridade estável fora de `2,5R`. A fase limita 500 corpos dinâmicos, 800 shapes e 250 juntas.

A máquina de estados de input é `Inspeção → Mira → Voo/Habilidade → Resolução → Avaliação → Resultado`:

- `Inspeção`: botão direito orbita, roda aplica zoom e clique no anel entra em `Mira`;
- `Mira`: `Alt` + arraste esquerdo reposiciona o anel por raycast contra a casca de raio `R + 3 m`, limitado por uma máscara de arco do manifesto; arraste esquerdo no gizmo define vetor tangente e potência; `Q`/`E` aplicam roll fino; `Esc` cancela para `Inspeção`;
- `Espaço` é o único comando de lançamento, habilitado quando origem e vetor são válidos; soltar o mouse nunca lança;
- `Voo/Habilidade`: após 150 ms de armamento, clique esquerdo ou `Espaço` ativa a habilidade uma única vez; antes disso o comando é ignorado com feedback visual;
- `Resolução` aceita apenas câmera, pausa e reinício; `R` mantido por 0,5 segundo reinicia, sem undo ou snapshot implícito.

O mapeamento do mouse usa raycast da câmera. Movimentação do anel resolve a interseção raio–esfera; mira resolve a interseção com o plano tangente local. Empates e snaps usam ordem por ID estável, e todas as escolhas viram comandos quantizados no próximo tick.

Controles padrão:

- botão esquerdo: selecionar, manipular o gizmo e ativar habilidade durante o estado válido;
- botão direito: orbitar câmera;
- roda: zoom;
- `Q`/`E`: rotação fina do plano de lançamento;
- `A`/`D`: potência fina;
- `Espaço`: lançar na `Mira` ou ativar habilidade no `Voo` após o armamento;
- `R`: manter por 0,5 segundo para reiniciar;
- `Esc`: pausa;
- `F1`: alternar ajuda contextual no build de desenvolvimento.

A mira usa cores, espessura, marca de primeiro impacto e sombra projetada no planeta. Não depende apenas de cor. A câmera limita inclinação e distância para evitar desorientação, oferece recentralização e reduz movimento em uma opção de acessibilidade.

## 6. Aves jogáveis

### 6.1 Luma, a Beija-flor Vetorial

Função: controle de destroços e reações em cadeia.

Ao ativar, Luma cria por 1,25 segundo um poço gravitacional temporário no ponto atual. Corpos dinâmicos de até 150 kg num raio de 4 metros recebem aceleração suavizada de no máximo `12 m/s²` em direção ao campo; depois, um pulso de no máximo `4 m/s` os libera radialmente. Corpos são processados por handle crescente. O campo não exige linha de visão, mas ignora planeta, cenário permanente, alvos neutralizados e o próprio projétil.

Leitura visual: trilhas turquesa, folhas orbitais e anéis concêntricos. Áudio: subida harmônica seguida de pulso grave.

### 6.2 Nox, a Coruja Fásica

Função: penetração precisa e ataque ao interior.

Ao ativar, Nox recebe um impulso de perfuração na direção de voo. Um shape cast identifica até 3 metros de materiais perfuráveis. A habilidade aplica dano concentrado na entrada e procura a primeira posição com folga esférica de `raio_da_ave + 5 cm`, fora do planeta e sem overlap. Se existir, move a ave no limite do tick e conserva 70% do momento linear. Se não existir, não teleporta: aplica o dano de entrada, rebate com 35% da velocidade e consome a habilidade. Basalto, aço e liga magnética bloqueiam a travessia; vidro, cerâmica, madeira, tijolo e materiais leves podem ser atravessados.

Leitura visual: silhueta violeta, rastro de refração e rachadura luminosa nos pontos de entrada e saída.

### 6.3 Talo, o Albatroz-Raiz

Função: tração estrutural e colapso controlado.

No primeiro impacto após ativação, Talo ancora uma semente. Ela seleciona até três corpos estruturais dinâmicos num raio de 4,5 metros, visíveis por raycast. Candidatos são ordenados por distância quantizada e depois por handle. Planeta, extrator, inimigos, fragmentos menores que 10 cm e corpos já neutralizados são excluídos. A semente cria juntas de distância e contrai o comprimento-alvo durante 1,5 segundo. O motor de contração aplica no máximo `8 kN`; a força de reação total da junta, que também inclui impactos e carga externa, causa ruptura acima de `10 kN` por dois ticks. Um teste aplica impacto externo para provar que esse estado é alcançável. A raiz nunca atravessa o volume do planeta.

Leitura visual: raízes douradas que engrossam conforme a tensão e soltam folhas ao romper.

## 7. Inimigos suínos

Os inimigos são javalis mineiros de tons ferrugem, creme e roxo, com silhuetas alongadas e equipamentos industriais. Eles não usam o visual arredondado verde associado a Angry Birds.

### 7.1 Javali-Âncora

É pesado, possui capacete mineral e recebe apenas 25% do dano quando a normal do impacto fica a até 45° da direção frontal do capacete. Impactos laterais ou traseiros usam 100%. Neutraliza-se ao zerar integridade ou cumprir a regra numérica de ejeção. Sua massa também pode estabilizar a construção e servir como contrapeso.

### 7.2 Javali-Engenheiro

É fisicamente frágil, mas aumenta em 35% o limiar de ruptura das juntas estruturais num raio de 5 metros, limitado às 24 juntas mais próximas e desempate por ID. O reforço desaparece no tick seguinte à neutralização. Um feixe visual conecta o engenheiro às juntas afetadas para tornar a regra legível.

### 7.3 Javali-Sifonador

Flutua em um casulo que atrai fragmentos de até 40 kg e alimenta um extrator explicitamente ligado no manifesto. O campo pode proteger aliados ou ser sobrecarregado: manter ao menos 120 kg de massa elegível dentro de 1,5 metro por 0,75 segundo desliga a sustentação. O casulo então se torna plenamente dinâmico e pode colidir com a estrutura.

Um inimigo é neutralizado por integridade igual a zero ou pela regra de ejeção da seção 5. Desligar o casulo apenas remove a proteção do Sifonador; ele ainda precisa ser danificado ou ejetado. Cada condição gera feedback visual e sonoro distinto.

## 8. Materiais

Todos os materiais são recursos de dados. Valores abaixo são pontos iniciais de calibração, não promessas de equivalência laboratorial. `Densidade` usa kg/m³; `Atrito` e `Restituição` seguem o modelo do Box3D; `Tenacidade` é um multiplicador adimensional aplicado ao limiar de energia de fratura por massa. A arte pode ter variantes sem mudar a identidade física.

| # | Material | Densidade | Atrito | Restituição | Tenacidade | Comportamento distintivo |
|---:|---|---:|---:|---:|---:|---|
| 1 | Madeira de pinho | 520 | 0,55 | 0,18 | 0,55 | quebra progressiva e produz lascas leves |
| 2 | Madeira dura | 760 | 0,62 | 0,12 | 0,82 | resiste à compressão, rompe em juntas |
| 3 | Bambu | 420 | 0,48 | 0,30 | 0,70 | flexiona em segmentos e devolve energia |
| 4 | Cortiça | 240 | 0,50 | 0,38 | 0,28 | muito leve, excelente para campos gravitacionais |
| 5 | Tijolo | 1.800 | 0,72 | 0,08 | 0,52 | blocos resistem; argamassa/juntas cedem |
| 6 | Concreto | 2.300 | 0,78 | 0,05 | 0,76 | pesado, forte à compressão e frágil a impacto pontual |
| 7 | Arenito | 2.100 | 0,70 | 0,07 | 0,38 | acumula dano e esfarela |
| 8 | Basalto | 3.000 | 0,80 | 0,04 | 1,00 | âncora quase indestrutível no escopo normal |
| 9 | Vidro | 2.450 | 0,42 | 0,10 | 0,16 | rompe com pico de impacto e gera fragmentos predefinidos |
| 10 | Cristal harmônico | 2.600 | 0,38 | 0,16 | 0,24 | ruptura transmite pulso a cristais da mesma frequência |
| 11 | Cerâmica | 2.200 | 0,60 | 0,09 | 0,22 | suporta carga estática, falha sob impacto concentrado |
| 12 | Aço | 3.200 | 0,52 | 0,08 | 1,00 | densidade de jogo limitada; deforma apenas por juntas |
| 13 | Ferro meteórico | 3.400 | 0,58 | 0,06 | 0,95 | pesado e afetado por liga magnética |
| 14 | Cobre solar | 2.900 | 0,50 | 0,07 | 0,72 | conduz pulsos entre dispositivos conectados |
| 15 | Liga magnética | 3.100 | 0,46 | 0,09 | 0,88 | atrai ferro dentro de raio curto e limitado |
| 16 | Gelo cometário | 900 | 0,05 | 0,12 | 0,25 | desliza; impactos energéticos reduzem sua tenacidade |
| 17 | Borracha lunar | 1.050 | 0,84 | 0,78 | 0,66 | ricochete alto com limite de velocidade |
| 18 | Espuma-nuvem | 85 | 0,76 | 0,15 | 0,20 | amortece e dissipa energia de colisão |
| 19 | Resina âmbar | 1.100 | 0,92 | 0,03 | 0,46 | cria junta adesiva temporária no primeiro contato forte |
| 20 | Cápsula de pólen | 320 | 0,44 | 0,22 | 0,18 | ao romper, aplica impulso radial sem fogo ou dano térmico |

As densidades dos metais são reduzidas em relação aos valores reais para limitar razões de massa, manter estabilidade de pilhas e produzir ritmo de jogo. Essa adaptação é explícita: a meta é comportamento fisicamente plausível e consistente, não simulação de engenharia.

Os vinte materiais aparecem no laboratório. As seis fases usam subconjuntos cuidadosamente ensinados; nenhuma fase exige memorizar todos ao mesmo tempo.

## 9. Destruição e dano

A geometria não fratura arbitrariamente em tempo de execução. Peças quebráveis possuem fragmentos pré-segmentados e juntas estruturais preparadas no Blender ou por ferramentas de geração. Isso garante silhuetas bonitas, colisores convexos, orçamento previsível e replays estáveis.

Eventos de impacto do Box3D alimentam um sistema de dano por energia. Para cada par de corpos, apenas o evento de maior velocidade normal no tick é contabilizado. A energia incidente é `E = 0,5 * m_eff * max(0, v_n - 1 m/s)²`, onde `m_eff = mA*mB/(mA+mB)` para dois corpos dinâmicos e a massa do corpo dinâmico contra cenário estático. O dano normalizado no alvo é `D = E / (massa_alvo * 250 J/kg * tenacidade)`, multiplicado pela vulnerabilidade direcional. Materiais frágeis rompem quando um único `D ≥ 1`; fibrosos e alvenaria acumulam `D` sem recuperação durante o lançamento e rompem ao total `≥ 1`. O dano acumulado persiste até o fim da fase.

Juntas possuem limites de força e torque. Elas rompem se `max(força/limite_força, torque/limite_torque) ≥ 1` por dois ticks consecutivos ou `≥ 1,5` num único tick. Leituras e rupturas são enfileiradas e aplicadas somente no início do próximo tick, evitando mutação durante callbacks.

Ao fraturar, o corpo-pai é substituído por fragmentos predefinidos. A soma de massa deve ficar a ±1% da massa original. Cada fragmento recebe `v = v_pai + ω_pai × deslocamento` e a mesma velocidade angular; um impulso de separação total limitado a 2% do impulso causador evita sobreposição. Colisões entre irmãos ficam desabilitadas por dois ticks. O momento linear após a troca deve ficar a ±5% do anterior. Se hulls, massa ou overlaps violarem invariantes, a fratura física é cancelada, o erro é registrado e apenas o feedback visual não destrutivo é emitido no build de desenvolvimento.

Cada quebra emite um evento de domínio com material, energia, posição, normal e cadeia causal. Renderização, áudio, partículas, pontuação e câmera consomem o evento sem conhecer handles do Box3D.

Comportamentos de material usam uma interface comum de eventos `on_contact`, `on_break` e `on_fixed_tick`. As seis respostas estruturais são fibrosa, flexível, alvenaria, frágil, dúctil e elástica/amortecedora. Um material pode adicionar no máximo um modificador especial dentre ressonância, condução, magnetismo, adesão, amolecimento ou pulso. Cada modificador tem orçamento próprio, ordenação por handle e teste isolado; a ausência do modificador preserva a resposta estrutural básica.

## 10. Arquitetura técnica

### 10.1 Pilha escolhida

- Godot Engine `4.5.1-stable`, commit `f62fdbde15035c5576dad93e586201f4d41ef0cb`, como host de renderização, cenas, UI, input, áudio, shaders, partículas e exportação Windows;
- Box3D `v0.1.0`, commit `8441b4a06d6d09dcfb0b0f704df4d847d1437b92`, compilado estaticamente;
- GDExtension C++ para conectar o núcleo físico ao Godot, usando `godot-cpp` `godot-4.5-stable`, commit `e83fd0904c13356ed1d4c3d09f8bb9132bdc6b77`;
- C++20 para o código de integração e C17 para Box3D;
- Visual Studio 2026 `18.7.3` build `11925.98`/MSVC `14.44` (toolset v143), Windows SDK target `10.0.26100.0`, CMake `4.3.3` e Ninja `1.13.2`;
- generator `Ninja` com diretórios `build/debug` e `build/release` separados para selecionar `GODOTCPP_TARGET=template_debug|template_release`, arquitetura x64, CRT estático `/MTd` em Debug e `/MT` em Release, sem `/fp:fast`;
- Blender 5.1 para modelagem, geração procedural, UVs, colisores e exportação glTF/GLB;
- Git para versionamento; dependências externas sempre fixadas por tag e commit.

Fontes técnicas: [Box3D oficial](https://github.com/erincatto/box3d), [release v0.1.0 declarada alfa](https://github.com/erincatto/box3d/releases/tag/v0.1.0), [Hello Box3D](https://box2d.org/documentation3d/hello.html), [Godot 4.5.1](https://github.com/godotengine/godot/releases/tag/4.5.1-stable), [godot-cpp 4.5 estável](https://github.com/godotengine/godot-cpp/releases/tag/godot-4.5-stable), [Visual Studio 2026](https://learn.microsoft.com/en-us/visualstudio/releases/2026/release-history), [CMake 4.3.3](https://github.com/Kitware/CMake/releases/tag/v4.3.3), [Ninja 1.13.2](https://github.com/ninja-build/ninja/releases/tag/v1.13.2) e [pipeline glTF/Blender do Godot](https://docs.godotengine.org/en/latest/tutorials/assets_pipeline/importing_3d_scenes/available_formats.html).

### 10.2 Limites de módulos

```text
Godot: cenas, UI, câmera, VFX, áudio e conteúdo
                    |
Box3D GDExtension: API em lote e conversão de tipos
                    |
PhysicsKernel: handles próprios, tick, gravidade, dano e replay
                    |
Box3D v0.1.0: colisão, solver, corpos, shapes, queries e joints
```

`PhysicsKernel` não depende do Godot e roda em testes headless. Nenhum tipo `b3*`, ponteiro ou ID interno atravessa sua API pública. O jogo usa handles geracionais próprios, comandos imutáveis e eventos copiados. Essa camada anticorrupção contém futuras mudanças do solver alfa.

Godot não cria `RigidBody3D`, `CharacterBody3D` ou colisores próprios para objetos de gameplay. `Node3D` representa somente a visão. O estado autoritativo vem do kernel e é sincronizado em lotes de transformações após cada tick.

### 10.3 Fluxo por quadro

1. Godot coleta input e cria comandos de alto nível.
2. O adaptador enfileira comandos sem alterar o mundo imediatamente.
3. Um acumulador avança passos fixos de `1/60 s`, inicialmente com quatro substeps do Box3D.
4. No início do tick, o kernel aplica comandos, gravidade radial e habilidades ativas.
5. Box3D avança o mundo.
6. O kernel copia contatos, eventos de corpo e forças de juntas.
7. Sistemas de dano e regras produzem eventos e enfileiram alterações para o tick seguinte.
8. Transformações e eventos são publicados em arrays contíguos.
9. Godot interpola apenas a apresentação entre o estado anterior e o atual.

O acumulador processa no máximo quatro ticks de recuperação por quadro; hitches maiores descartam atraso visual e registram telemetria. Simulação nunca recebe `delta` variável.

Corpos adormecidos são omitidos da aplicação de gravidade e só acordam por contato, comando ou habilidade. A avaliação de repouso considera corpos relevantes com velocidade linear `< 0,15 m/s` e angular `< 0,20 rad/s` por 1 segundo; projétil ativo, alvos, peças estruturais ligadas e extratores são relevantes, enquanto fragmentos em órbita fora de `2,5R` não bloqueiam a avaliação.

O replay cinematográfico não re-simula Box3D. Um ring buffer guarda snapshots autoritativos a 30 Hz dos últimos 20 segundos, lifecycle de corpos, eventos de domínio e sugestões de câmera. Posições são quantizadas em milímetros e quaternions em quatro componentes de 16 bits; IDs e eventos têm ordenação canônica. O orçamento é 24 MB. O resultado reproduz até 8 segundos ao redor da maior cadeia do último lançamento e usa somente nodes visuais fantasmas, áudio/VFX referenciados pelos eventos e câmera reaplicada. Se o buffer estiver incompleto, o replay começa no snapshot completo mais recente. Hashes de determinismo são exatos sobre estado canônico quantizado; comparações com tolerância são métricas separadas, nunca “hash com tolerância”.

### 10.4 Dados e autoria de fases

Materiais, aves, inimigos e parâmetros de fase usam recursos versionáveis e validados na carga. Modelos visuais usam glTF/GLB. Proxies de colisão são convex hulls ou compounds separados dos meshes renderizados. Convenções: metros, eixo Y para cima, pivôs no centro de massa planejado e nomes estáveis.

Um manifesto de fase declara planeta, casca/arcos permitidos do anel, peças, material, transform, juntas, aves disponíveis, inimigos, extratores, ligações e objetivos tipados. Os objetivos iniciais são `neutralize_all_targets`, `disable_all_extractors` e `preserve_core_integrity(min_percent)`. Um extrator desliga quando sua integridade zera ou quando todos os Sifonadores ligados são neutralizados. Vitória exige todos os objetivos obrigatórios; alvos e dispositivos opcionais afetam apenas pontuação. O build falha se houver material inexistente, ID duplicado, hull inválido, corpo dinâmico sem densidade, objetivo sem alvo ou junta/ligação referenciando entidade ausente.

O save usa schema JSON versionado `1`, separado entre progresso e configurações. Ele guarda fases desbloqueadas, melhor pontuação, tutoriais vistos e opções. Escrita ocorre em arquivo temporário, flush e rename atômico; a versão anterior permanece como backup. Se principal e backup forem inválidos, ambos são preservados com sufixo `.corrupt`, padrões seguros são carregados e a UI informa a recuperação. Vencer uma fase desbloqueia apenas a próxima.

Pontuação usa inteiros e arredondamento half-up: `1.000` por javali-alvo, `250` adicionais se neutralizado por ejeção, `750` por extrator, `1.500 * aves_não_usadas / aves_iniciais` de eficiência e `10 * integridade_percentual_do_núcleo`. Pontos de reação: junta rompida `30`, peça estrutural fraturada `50`, modificador especial acionado `75`; a fratura recebe bônus energético `min(150, round(50 * E/E_fratura))`. Eventos de reação únicos separados por no máximo 1,2 segundo formam combo: primeiro `×1`, segundo `×2`, até `×5`. O multiplicador afeta apenas pontos de reação; a soma pós-multiplicador é limitada a `2.000` por lançamento. Alvos/extratores podem manter a cadeia visual, mas seus prêmios fixos não são multiplicados. Cada ID de entidade/evento pontua uma vez, impedindo farming por recriação.

O runtime canônico é Y-up, metros, mão direita e frente em `-Z`, compartilhado por Godot e Box3D. Blender trabalha em metros e Z-up; o exportador GLB e o importador Godot fazem a conversão. Proxies de colisão são meshes convexos nomeados no mesmo GLB e extraídos já no espaço Godot, sem conversão manual paralela. Uma fixture dourada assimétrica valida posição, quaternion, escala, volume, centro de massa e hull de Blender → GLB → Godot → Box3D. Pivô visual e centro de massa físico são armazenados separadamente.

## 11. Mitigação do Box3D alfa

1. Fixar `v0.1.0` e o SHA exato; nunca compilar `main` automaticamente.
2. Compilar Box3D estaticamente dentro da extensão para evitar incompatibilidade de DLL/CRT.
3. Preservar o código externo sem modificações; patches locais ficam isolados e documentados.
4. Começar single-threaded. Multithreading só entra após profiling e replays equivalentes.
5. Não usar `/fp:fast`; manter ordem estável de criação e comandos somente em limites de tick.
6. Executar testes upstream e cenários próprios em Debug e Release x64.
7. Registrar entrada de cenário e seed em formato próprio. Replays nativos do Box3D complementam, mas não substituem, os goldens do jogo.
8. Qualquer atualização do solver exige suíte completa, comparação de métricas e aprovação explícita no changelog técnico.
9. Crash, corrupção, tunneling sem mitigação ou instabilidade de pilhas bloqueiam expansão de conteúdo e produzem reprodução mínima.

## 12. Direção visual e pipeline Blender

O estilo é um diorama cósmico estilizado: formas arredondadas, silhuetas grandes, materiais PBR simplificados, gradientes pintados, bordas suavizadas e partículas gráficas. A iluminação combina sol direcional, preenchimento do planeta, glow seletivo e sombras de contato. Planetas usam paletas curtas para preservar leitura dos materiais.

Blender gera:

- planeta e módulos de terreno;
- anel de impulso;
- três aves e três javalis com rigs simples;
- kits estruturais dos vinte materiais;
- fragmentos predefinidos e proxies convexos;
- elementos decorativos de fundo.

Scripts Blender repetíveis constroem primitivas, LODs, nomes, pivôs e exportação. Os `.blend` de origem permanecem no repositório; o jogo consome GLB versionado para builds reproduzíveis. Luzes e efeitos finais são configurados no Godot.

Princípios de leitura:

- materiais diferem por silhueta, roughness, som e partículas, não apenas cor;
- objetos interativos recebem rim light discreta;
- trajetória e alvos permanecem legíveis em fundos claros ou escuros;
- VFX nunca encobrem o primeiro impacto por mais de 150 ms;
- opções reduzem tremor de câmera, flash e partículas.

Uma art bible versionada fixa paletas, escala, shapes, roughness, iluminação, linguagem de UI e exemplos proibidos por semelhança de IP. Cada fase possui capturas douradas em vista geral, mira e impacto, a 1080p no preset médio. Revisão visual exige silhuetas reconhecíveis em miniatura de 160 px, contraste WCAG AA nos textos, ausência de clipping/z-fighting evidente e desvio de histograma/luminância investigado contra as capturas. Orçamento por vista: até 1,5 milhão de triângulos, 1.000 draw calls, 512 MB de texturas residentes e 200 mil partículas simultâneas no preset alto.

## 13. Áudio e interface

Áudio usa paisagem cósmica leve, percussão de madeira/metal e motivos próprios. Cada família material possui camadas de impacto, tensão e ruptura. A intensidade musical reage a cadeias de destruição sem alterar a simulação.

A interface é mínima e diegética: o anel exibe potência e plano; ícones das aves mostram disponibilidade e habilidade; inimigos reforçados exibem ligação visual. Textos usam frases curtas e pictogramas. O tutorial interrompe apenas na primeira ação indispensável e pode ser reaberto.

Configurações iniciais: resolução, modo de janela, VSync, escala de render, qualidade de sombras, volume por canal, sensibilidade da câmera, redução de movimento, intensidade de tremor, limite de partículas, escala de UI de 100% a 200%, tamanho de legendas, contraste de trajetória e remapeamento completo.

Toda a UI é navegável por teclado, com foco visível e sem exigir mouse. Ações de arrastar têm alternativa por teclas e ajustes incrementais; ações cronometradas aceitam uma janela ampliada de habilidade. Legendas identificam fonte e categoria de som; impactos importantes também têm pulso visual. Nenhuma regra depende somente de cor ou áudio. Testes cobrem escala 100%, 150% e 200%, remapeamento persistente, navegação sem mouse, redução de movimento/flash e contraste dos elementos de mira.

## 14. Fases

| Fase | Planeta | Aves e quantidade | Inimigos | Materiais ensinados | Objetivo obrigatório | Par de referência |
|---|---|---|---|---|---|---:|
| 1. Primeira Órbita | Jardim Aster | Luma ×3 | Âncora ×1 | pinho, vidro, tijolo | neutralizar alvo | 3 |
| 2. Coração Oco | Jardim Aster | Nox ×3 | Âncora ×2 | cerâmica, arenito, concreto, cristal | neutralizar alvos + extrator | 3 |
| 3. Raízes no Vazio | Jardim Aster | Talo ×3 | Engenheiro ×1, Âncora ×1 | madeira dura, bambu, aço, resina | neutralizar alvos com núcleo ≥50% | 3 |
| 4. Oficina do Engenheiro | Forja Umbra | Luma ×2, Nox ×1 | Engenheiro ×2, Âncora ×1 | cobre, ferro, borracha, gelo, liga magnética | neutralizar alvos + 2 extratores | 3 |
| 5. Peso do Mundo | Forja Umbra | Luma ×1, Talo ×2 | Sifonador ×1, Âncora ×2 | basalto, cortiça, espuma, cápsula de pólen | neutralizar alvos + extrator | 3 |
| 6. Sifão do Eclipse | Forja Umbra | Luma ×1, Nox ×1, Talo ×1 | as três classes ×2 | combinação sem material inédito | neutralizar alvos + 3 extratores com núcleo ≥35% | 3 |

Todos os vinte materiais são ensinados antes da fase final. O “par de referência” é a quantidade de lançamentos usada no balanceamento, não um limite rígido; soluções com menos aves aumentam eficiência. Cada fase deve possuir duas rotas verificadas: ataque estrutural direto e uso de habilidade/material.

O laboratório contém presets por família, controle de câmera, lançamento infinito e painel de métricas no build de desenvolvimento.

## 15. Testes e observabilidade

### 15.1 Testes nativos

- criação/destruição e validade de handles;
- passo fixo, fila de comandos e ordem de eventos;
- aceleração radial em raios representativos;
- trajetória de lançamento e CCD do projétil `isBullet`;
- repouso de pilha, limites de penetração e ausência de tunneling no cenário hostil;
- cálculo de dano, ruptura de juntas e fragmentação predefinida;
- cada comportamento especial de material;
- cada habilidade e condição de neutralização;
- carga e validação de manifestos;
- replay do mesmo build com hash exato do estado quantizado e métricas tolerantes separadas;
- captura/playback do ring buffer visual e limite de 24 MB;
- objetivos, pontuação, prevenção de duplicidade, unlock e recuperação de save corrompido.

### 15.2 Testes Godot

- extensão carrega e descarrega sem handles vazando;
- transformações são sincronizadas em lote;
- reinício repetido não acumula nós, memória ou áudio;
- menu, pausa, resultado e seleção de fase funcionam em modo headless quando aplicável;
- assets e recursos obrigatórios existem no build exportado;
- UI funciona sem mouse, em escalas de 100%/150%/200%, com remapeamento e opções de redução visual persistentes.

### 15.3 Playtests automáticos e manuais

- lançar, ativar habilidade, vencer, perder, reiniciar e sair;
- completar cada fase pelas duas rotas registradas na matriz de conteúdo;
- verificar legibilidade em 1080p e escalas de UI de 100% a 150%;
- capturar screenshot e log de erros em cada build candidato;
- registrar métricas de tempo de step, corpos acordados, contatos, juntas e fragmentos.

Meta obrigatória de desempenho: 60 FPS em 1920×1080 no preset médio, VSync desligado, num Ryzen 5 3600 ou Core i5-10400 com GTX 1060 6 GB ou RX 580 8 GB, 16 GB de RAM e driver estável vigente no início do marco de polimento. O protocolo usa build Release, 30 segundos de aquecimento, três execuções de 5 minutos da fase 6 e cenário de stress com 500 corpos, 800 shapes e 250 juntas. O p95 do frame completo deve ser `≤16,67 ms`, o p95 do step físico `≤8 ms`, e nenhuma execução pode apresentar crash ou estado inválido. Presets reduzem sombras, partículas e resolução, nunca correção física. Não cumprir bloqueia aceite; exceção exige waiver escrito do usuário com hardware, medição e impacto.

### 15.4 Licenças, compatibilidade e pacote

Dependências e assets externos aceitos automaticamente usam MIT, BSD-2/3-Clause, Apache-2.0, Zlib ou CC0. CC-BY exige atribuição individual. GPL, AGPL, LGPL dinâmica, licenças `NC`/`ND`, conteúdo sem proveniência e assets extraídos de outros jogos exigem revisão e não entram por padrão. O repositório mantém inventário SPDX JSON, `THIRD_PARTY_NOTICES.md`, origem, versão, hash e licença de cada item; assets originais e gerados registram ferramenta, script e autor da geração.

A entrega é um ZIP portátil x86_64 com executável, PCK, GDExtension, licenças, notas e SHA-256. A DLL usa CRT estático, portanto não exige instalador do Visual C++ Redistributable. O alvo é Windows 10 22H2 x64 e Windows 11 23H2 ou posterior, validado em VMs limpas com usuário sem privilégios administrativos. O primeiro pacote é não assinado e declara isso nas notas; assinatura Authenticode só entra quando houver certificado fornecido pelo proprietário.

## 16. Critérios de aceite

O jogo inicial é aceito quando:

- o executável abre numa instalação Windows limpa compatível e chega ao menu sem erro;
- Box3D v0.1.0 é verificável no build e é o único solver dos objetos de gameplay;
- as seis fases passam pelas duas rotas registradas e o laboratório instancia os 20 presets;
- as três aves possuem habilidades distintas e testadas;
- as três classes de javalis aparecem com regras legíveis e testadas;
- os vinte materiais aparecem no laboratório e seus dados/efeitos passam pela validação;
- lançamento, câmera, impacto, destruição, objetivos, vitória, derrota, replay e reinício funcionam sem intervenção do editor;
- pontuação não duplica eventos, progresso persiste e save principal corrompido recupera o backup;
- modelos, rigs, fragmentos e proxies são reproduzíveis a partir dos `.blend`/scripts versionados;
- tutorial, áudio, VFX, legendas, remapeamento e navegação sem mouse passam pelos casos da seção 15;
- art bible e três capturas douradas por fase passam pela revisão visual e pelos orçamentos da seção 12;
- testes nativos, testes de integração, importação de assets e exportação Windows terminam sem falhas;
- não existem erros no console durante um playthrough automatizado das rotas principais;
- desempenho atende obrigatoriamente ao protocolo da seção 15.3, salvo waiver explícito do usuário;
- ZIP roda nas VMs limpas definidas, possui SHA-256 e não exige runtime/instalação adicional;
- SBOM, NOTICE, proveniência e atribuições cobrem todas as dependências e assets.

Matriz mínima de rastreabilidade:

| Requisito | Evidência principal | Artefato |
|---|---|---|
| Box3D autoritativo | testes nativos + inspeção da cena sem corpos Godot | DLL e relatório do spike |
| lançamento/câmera/input | playtest automatizado da máquina de estados | vídeo/log da fase 1 |
| 3 aves e 3 javalis | testes por habilidade/estado + fases 1–6 | recursos e manifests |
| 20 materiais | validação de dados + laboratório + testes dos modificadores | catálogo e cena laboratório |
| destruição | testes de energia, joints, massa e momento | replays golden |
| seis fases/dois planetas | duas rotas por linha da matriz | manifests e relatórios |
| replay/pontuação/save | testes de buffer, score único e corrupção | snapshots e saves fixture |
| visual/áudio/acessibilidade | capturas douradas, budgets e casos assistivos | art bible e checklist QA |
| performance | protocolo 1080p em três execuções | JSON de benchmark |
| entrega/licenças | VM limpa, hash e validação SPDX | ZIP, SHA-256, SBOM e NOTICE |

## 17. Marcos de produção

1. **Bootstrap reproduzível:** ferramentas, repositório, build, CI local e shell mínimo do Godot.
2. **Spike Box3D:** kernel headless, planeta, gravidade radial, projétil bullet, pilha e stress.
3. **Vertical slice:** lançamento, câmera, uma ave, um inimigo, três materiais e fluxo completo.
4. **Destruição sistêmica:** dano, joints, fragmentos, vinte materiais e laboratório.
5. **Elenco completo:** três aves, três javalis, UI de seleção e regras de fase.
6. **Conteúdo e arte:** seis fases, modelos Blender, shaders, áudio, VFX e tutorial.
7. **Polimento e entrega:** acessibilidade, profiling, regressão, export, licenças e pacote Windows.

Cada marco só avança após build, testes, playtest e revisão de código proporcionais ao risco.
