# Fazenda — Reação em Cadeia: layout físico v1

Este documento congela a autoria física do nível `farm_reaction`. O manifesto
`game/data/levels/earth/farm_reaction.level.json` é a autoridade de runtime; o
fixture `native/simulation/tests/fixtures/product_v2/farm_reaction_layout_v1.json`
é sua projeção tipada e auditável. Meshes e nodes de apresentação não criam
bodies, shapes, juntas, triggers ou objetivos.

## Identidade congelada

- `layout_hash`: `fnv1a64:8324ce8a44754dad`.
- 72 bodies, dos quais 54 dynamic e 18 static.
- 52 joints em oito assemblies conexas.
- quatro objetivos de porco e dois triggers `damage_threshold`.
- bounds AABB `(-24,-12,-12)` a `(48,32,12)` e gravidade uniforme
  `(0,-9.81,0)` em todo body dynamic.
- aceitação de repouso: 120 ticks sem dano, yield, ruptura, fratura, trigger ou
  neutralização; as 52 juntas permanecem ativas.
- proxies `box`, inclusive fragmentos, têm half-extent mínimo de 0,01 m.
- cada `visual.bounds_m` é exatamente o tamanho do AABB local recursivo de seu
  collider, por eixo e com tolerância de autoria de `1e-6 m`.

## Transforms normativos

Os transforms de body abaixo, e os demais transforms congelados no fixture,
permanecem idênticos ao contrato autoral. Apoios, folgas e assentamentos são
modelados exclusivamente por shapes compostos e `local_transform`.

| Grupo | Body | Posição de root `(x,y,z)` | Observação local |
|---|---:|---|---|
| Porco 1 | 2 | `(2.6,0.55,-3.4)` | cápsula `(-0.02,0.06,0)`; esfera `(0.26,0.08,0)` |
| Porco 2 | 3 | `(7.4,1.05,1.8)` | cápsula `(-0.02,-0.44,0)`; esfera `(0.26,-0.42,0)` |
| Porco 3 | 4 | `(13.0,1.4,0)` | cápsula `(-0.02,-0.13,0)`; esfera `(0.26,-0.11,0)` |
| Porco 4 | 5 | `(17.0,0.55,-2.5)` | cápsula `(-0.02,0.06,0)`; esfera `(0.26,0.08,0)` |
| Portão de vidro | 14 | `(6.7,2.2,0)` | moldura fraturável |
| Travessa do portão | 15 | `(4.4,4.2,0)` | pinho |
| Gaiola do portão | 16 | `(5.8,4.0,0)` | frame de chapa |
| Rampa | 20 | `(8.8,1.8,0)` | rotação `-12°`; plano principal em offset local `x=0.2` |
| Fardo A | 21 | `(7.8,3.4,-0.75)` | root e collider coincidentes; fraturável |
| Fardo B | 22 | `(7.8,3.4,0)` | root e collider coincidentes |
| Fardo C | 23 | `(7.8,3.4,0.75)` | root e collider coincidentes |
| Suporte da rampa | 24 | `(10.5,0.8,-1.0)` | pinho |
| Suporte da rampa | 25 | `(10.5,0.8,1.0)` | collider local assentado |
| Travessa da rampa | 26 | `(6.8,3.2,0)` | pinho |
| Tanque de combustível | 58 | `(16.2,1.1,-1.6)` | gaiola de chapa, seção 0,01 m |
| Vaso pressurizado | 59 | `(17.6,1.0,-3.3)` | gaiola de chapa, seção 0,01 m |

Cada porco conserva massa total de 65 kg, densidade
`293.222103729329 kg/m³`, COM assimétrico positivo em X e inércia positiva. A
gravidade atua nos porcos e em todos os demais bodies dynamic.

## Inventário por assembly

| Assembly | Bodies | Joints | Conteúdo físico |
|---|---:|---:|---|
| `ASM_NearShelter` | 6–12 | 1–6 | baldrame, plintos, postes/travessas de pinho e molduras de palha |
| `ASM_ChainGate` | 13–19 | 7–12 | fundação, apoios reais, vidro, pinho e gaiola de chapa |
| `ASM_HayRamp` | 20–26 | 13–18 | rampa inclinada, três fardos e suportes |
| `BLD_Farm_Barn` | 27–35 | 19–26 | baldrame, estrutura, vidro e cobertura de palha |
| `ASM_FarmSilo` | 36–45 | 27–35 | baldrame de alvenaria, plintos, grelha e cobertura metálica |
| `BLD_Farm_Windmill` | 46–51 | 36–40 | base/torre, mastro frontal, hub e três pás |
| `ASM_FarmEquipment` | 52–59 | 41–47 | baldrame, vagão, trator, pórtico e dois dispositivos em gaiolas |
| `KIT_Farm_Fences` | 60–65 | 48–52 | dois postes e quatro travessas com montante central |

Os bodies 66–72 continuam módulos static individuais da cerca sul. O grafo de
cada assembly é explicitamente percorrido pelo teste; contagens sem conexão não
são aceitas.

## Caminhos de carga e contatos

O solver cria welds no ponto médio entre os roots e usa
`collide_connected=false`. Portanto uma peça não pode se apoiar fisicamente no
mesmo body ao qual está diretamente soldada. O layout resolve isso com caminhos
de carga explícitos:

- as fundações 6, 27, 36 e 52 são baldrames perimetrais; plintos do terreno
  atravessam seus vãos e apoiam postes, painéis, vagão e porcos;
- a base 13 sustenta os fardos, a moldura de vidro e as vigas do portão por
  prateleiras, pedestais e montantes tangentes; sua laje central ocupa somente
  `z=-0,6..0,6`, e os montantes voltados à rampa ficam em `x=2,1` local;
- a rampa 20 inclui um console local que apoia o painel 37 sem mover nenhum
  root;
- a fundação 27 possui colunas sob a cobertura 34 e pedestais sob os painéis
  31/32;
- a base 46 possui mastro frontal até o hub 48;
- cada travessa 62–65 possui um montante central até o terreno;
- as pás 49–51 mantêm o alcance externo, mas seus colliders começam fora do
  miolo; elas encaixam no hub sem se interpenetrar mutuamente.
- os baldrames 36 e 52 abrem apenas os trechos voltados às fundações vizinhas;
  suas faces externas e caminhos de carga permanecem, sem duplicar superfícies
  estáticas com 27/36/52 nem com os plintos do terreno 1.

As únicas permissões de interpenetração deliberada são os encaixes estruturais
`15–16`, `20–24` e `20–25`, medidos por SAT OBB, e os encaixes de `0,05 m` do
hub esférico com as pás `48–49`, `48–50` e `48–51`, medidos por esfera/OBB. O
teste exige que cada entrada dessa allowlist ainda corresponda a contato real;
as antigas permissões `6–8`, `6–9`, `49–50`, `49–51` e `50–51` foram removidas
depois que deixaram de sobrepor. O mesmo gate faz broadphase AABB, SAT OBB e
teste esfera/OBB em todos os pares, inclusive `static–static`, para rejeitar
qualquer overlap externo não autorizado. Os pares estáticos `1–13`, `13–20`,
`13–27`, `27–36` e `36–52` são tangentes ou separados, sem allowlist.
Contatos tangentes usam tolerância de `1e-5 m`.

Os envelopes visuais não são caixas decorativas independentes: eles são as
dimensões do AABB local da união recursiva do collider, compondo todos os
`local_transform`. Os assets finais das Tasks 25–27 devem ser produzidos dentro
desses envelopes físicos, com tolerância de `1e-6 m` no gate de autoria.

## Materiais, shapes e juntas

Os IDs normativos são `pine=1`, `brick/masonry=5`, `glass/brittle=9`,
`straw=13` e `sheet_steel=17`. Densidades vêm do catálogo fechado. Chapa não
recebe fracture pattern; padrões de pinho, vidro, palha e tijolo conservam massa
e centro de massa.

Painéis finos usam molduras, grelhas ou gaiolas. Os telhados de palha 10 e 34
são molduras perimetrais; as grelhas 42 e as gaiolas 16, 53, 58 e 59 preservam
vazios físicos. O pórtico 57 usa duas colunas locais tangentes à viga 54. Esses
proxies evitam massas de sólidos ocultos sem transformar vãos em paredes.

Os 52 limites são uniformes por tipo e não possuem exceções:

| Kind | Força | Torque |
|---|---:|---:|
| `pine_fit` | 5.500 N | 900 N·m |
| `glass_clamp` | 2.200 N | 350 N·m |
| `mortar` | 1.400 N | 160 N·m |
| `straw_bind` | 800 N | 90 N·m |
| `steel_ductile` | 7.500 N | 900 N·m |

O yield intermediário de chapa permanece 3.200 N/450 N·m no runtime. Cada
`steel_ductile` tem exatamente um endpoint de material 17.

## Dispositivos

| Conteúdo | Body / entity | Contrato |
|---|---|---|
| Fuel tank | 58 / 3058 | threshold 40, fuse 12, cooldown 180; burst 5 m, 8.000 N·s, 80.000 J, LOS, max32 |
| Pressure vessel | 59 / 3059 | threshold 30, fuse 0, cooldown 180; burst 3,5 m, 6.000 N·s, 60.000 J, LOS, max24 |

Os dispositivos são chapa dúctil sem padrão de fratura. Cada trigger tem um
único target e detona no máximo uma vez.

## Algoritmo do layout hash

O JSON é parseado como `LevelManifest` v2 e resserializado por
`to_canonical_json` antes da projeção. A projeção ordena bodies, joints,
assemblies, triggers e objectives por ID, e memberships numericamente. Reais
finitos são normalizados para `round(value*100000)`; zero não preserva sinal.
O JSON compacto usa chaves lexicográficas e bytes UTF-8. FNV-1a 64 começa em
`14695981039346656037` e multiplica cada byte por `1099511628211` com overflow
unsigned de 64 bits.

Mutation probes válidos cobrem world, launcher, transform, shape, joint,
trigger e objective. Reordenações semanticamente equivalentes e grafias
`1`/`1.0` ou `-0`/`0` preservam o hash.

## Compatibilidade orbital

`first_orbit_v2.level.json` preserva semanticamente os bodies 1–22, joints,
assemblies e objective de `first_orbit.level.json`, adicionando apenas o planeta
explícito como body 23. Os três arquivos legados de materiais, archetypes e fase
orbital continuam byte-idênticos aos hashes congelados pelo teste.
