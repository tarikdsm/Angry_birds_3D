# Fazenda — Reação em Cadeia: layout físico v1

Este documento congela a autoria física final de `farm_reaction`. O manifesto
`game/data/levels/earth/farm_reaction.level.json` é a autoridade de runtime; o
fixture `native/simulation/tests/fixtures/product_v2/farm_reaction_layout_v1.json`
é sua projeção tipada e auditável. Meshes e nodes de apresentação não criam
bodies, shapes, juntas, triggers ou objetivos.

## Identidade congelada

- `layout_hash`: `fnv1a64:ee2ad0b8c2912aa`.
- 106 bodies: 86 `dynamic` e 20 `static`.
- 49 joints em oito assemblies conexas e 49 free bodies.
- 256 primitivas autoradas expandidas e 68 fragmentos físicos autorados.
- Quatro objetivos de porco e dois triggers `damage_threshold`.
- Bounds AABB `(-24,-12,-12)` a `(48,32,12)` e gravidade uniforme
  `(0,-9.81,0)` em todo body `dynamic`.
- Aceitação de repouso: 120 ticks sem dano, yield, ruptura, fratura, trigger ou
  neutralização; as 49 juntas permanecem ativas.
- Proxies `box`, inclusive fragmentos, têm half-extent mínimo de 0,01 m.
- Cada `visual.bounds_m` é exatamente o tamanho do AABB local recursivo de seu
  collider, por eixo e com tolerância de autoria de `1e-6 m`.

## Inventário por assembly

O grafo de cada assembly é explicitamente percorrido pelo teste; contagens sem
conexão não são aceitas. A composição final da rampa de feno é normativa:

| Assembly | Bodies | Joints |
|---|---|---|
| `ASM_NearShelter` | conforme fixture | conforme fixture |
| `ASM_ChainGate` | conforme fixture | conforme fixture |
| `ASM_HayRamp` | `[20,24,25,73]` | `[16,17,18]` |
| `BLD_Farm_Barn` | conforme fixture | conforme fixture |
| `ASM_FarmSilo` | conforme fixture | conforme fixture |
| `BLD_Farm_Windmill` | conforme fixture | conforme fixture |
| `ASM_FarmEquipment` | conforme fixture | conforme fixture |
| `KIT_Farm_Fences` | conforme fixture | conforme fixture |

Os memberships completos, a lista de free bodies e a projeção ordenada ficam no
fixture; eles são a fonte auditável para qualquer alteração de layout.

## Caminhos de carga e contatos

O solver cria welds no ponto médio entre os roots e usa
`collide_connected=false`. Apoios, folgas e assentamentos são modelados por
shapes compostos e `local_transform`; transforms de body não mascaram
instabilidade.

As únicas permissões de interpenetração deliberada são `15–16` e os encaixes do
hub 48 com as pás `49`, `50` e `51`: `48–49`, `48–50` e `48–51`. O teste exige
que cada entrada da allowlist corresponda a contato real e rejeita qualquer
overlap externo não autorizado. O gate faz broadphase AABB, SAT OBB e teste
esfera/OBB em todos os pares, inclusive `static–static`; contatos tangentes
usam tolerância de `1e-5 m`.

Os envelopes visuais são as dimensões do AABB local da união recursiva do
collider, compondo todos os `local_transform`. Os assets de apresentação devem
permanecer dentro desses envelopes físicos, com tolerância de `1e-6 m` no gate
de autoria.

## Materiais, shapes e juntas

Os IDs normativos são `pine=1`, `brick/masonry=5`, `glass/brittle=9`,
`straw=13` e `sheet_steel=17`. Em geral, densidades vêm do catálogo fechado;
duas exceções são autoradas no body porque a geometria representa um composto
ou volume aparente: o fardo lateral 23 usa `109.77468250716919 kg/m³` para
preservar exatamente massa e centro de massa após o detalhe compound, e o
martelo esférico oco 74 usa densidade efetiva `520 kg/m³`, em vez da densidade
de chapa maciça. Chapa não recebe fracture pattern; padrões de pinho, vidro,
palha e tijolo conservam massa e centro de massa.

Os limites são uniformes por tipo:

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

## Rota determinística congelada

A rota `farm_chain` aceita dois releases, obtém score `50054`, três estrelas e
610 eventos ordenados. Os comandos e a evidência terminal completos pertencem
ao fixture de playthrough; este documento não prescreve detalhes adicionais da
sequência causal.

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
