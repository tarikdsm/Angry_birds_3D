# Fazenda — Reação em Cadeia: layout físico v1

Este documento congela a autoria física do nível `farm_reaction`. O manifesto
`game/data/levels/earth/farm_reaction.level.json` é a autoridade de runtime; o
fixture `native/simulation/tests/fixtures/product_v2/farm_reaction_layout_v1.json`
é a projeção auditável e independente de apresentação. Nenhum mesh, node Godot
ou prop de diorama cria body, shape, junta, trigger ou objective.

## Identidade congelada

- `layout_hash`: `fnv1a64:303f48a4c1509fbd`.
- 72 bodies, dos quais 54 dynamic e 18 static.
- 52 joints em oito assemblies conexas.
- 12 bodies livres: chão, quatro porcos e sete módulos de cerca static.
- quatro objectives de porco e dois triggers `damage_threshold`.
- oito fragmentos físicos autorados em quatro padrões; cap simultâneo de
  runtime permanece 80.
- bounds AABB `(-24,-12,-12)` a `(48,32,12)` e gravidade uniforme
  `(0,-9.81,0)` em todo body dynamic.
- aceitação de repouso: 120 ticks sem dano, yield, ruptura, fratura, trigger ou
  neutralização; ao final, todas as 52 juntas permanecem ativas.

## Inventário por assembly

| Assembly | Bodies | Joints | Conteúdo físico e envelope nominal |
|---|---:|---:|---|
| `ASM_NearShelter` | 6–12 | 1–6 | base, postes e travessas de pinho, parede e teto de palha; origem `(2.6,0,-3.4)`, envelope `(4.2,3.8,3.0)` |
| `ASM_ChainGate` | 13–19 | 7–12 | moldura de vidro `(6.9,2.2,0)`, travessa `(4.4,4.45,0)` e gaiola de chapa `(5.2,4.0,0)` |
| `ASM_HayRamp` | 20–26 | 13–18 | rampa a `-12°` centrada em `(9,1.8,-4)`, três fardos assentados em `(8,2.72,-4.75/-4/-3.25)` e suportes |
| `BLD_Farm_Barn` | 27–35 | 19–26 | fundação, estrutura de pinho, dois painéis de vidro e cobertura de palha; envelope `(5.2,5.5,4.0)` |
| `ASM_FarmSilo` | 36–45 | 27–35 | fundação/segmentos de tijolo, grelha de chapa em `y=1.51`, quatro colunas e cobertura em moldura |
| `BLD_Farm_Windmill` | 46–51 | 36–40 | base/torre static, hub e três pás com bodies declarados; não existe dano por pá meramente visual |
| `ASM_FarmEquipment` | 52–59 | 41–47 | piso, vagão, trator em frame, pórtico, tanque de combustível e recipiente pressurizado protegidos por gaiolas |
| `KIT_Farm_Fences` | 60–65 | 48–52 | dois postes static e quatro travessas dynamic em cadeia conexa na borda norte |

Os bodies 66–72 são módulos static individuais na borda sul, entre
`x=1..19`, em `z=-6.5`. Eles não formam barreira invisível.

## Inimigos e dispositivos

| Conteúdo | Body / entity | Transform | Contrato |
|---|---|---|---|
| `PIG_FARM_01` | 2 / 2001 | `(3.2,0.61,-2.5)` | 65 kg, objective 1 |
| `PIG_FARM_02` | 3 / 2002 | `(7.4,0.61,1.8)` | 65 kg, objective 2 |
| `PIG_FARM_03` | 4 / 2003 | `(13.0,1.83,0)` | 65 kg, objective 3 |
| `PIG_FARM_04` | 5 / 2004 | `(18.8,0.61,-2.6)` | 65 kg, objective 4 |
| Fuel tank | 58 / 3058 | `(15.1,1.40,-1.0)` | threshold 40, fuse 12, cooldown 180; burst 5 m, 8.000 N·s, 80.000 J, LOS, max32 |
| Pressure vessel | 59 / 3059 | `(17.3,2.00,-2.7)` | threshold 30, fuse 0, cooldown 180; burst 3,5 m, 6.000 N·s, 60.000 J, LOS, max24 |

Os dois dispositivos são chapa dúctil sem padrão de fratura. Cada trigger tem
um único target entity/body, uma causa rastreável e detona no máximo uma vez. O
runtime aplica os caps globais de 12 m/s e 90 J/kg do PressureBurstSystem.

## Materiais, massas e juntas

Os IDs normativos seguem o contrato tipado já congelado na Task 15:
`pine=1`, `brick/masonry=5`, `glass/brittle=9`, `straw=13` e
`sheet_steel=17`. Isso resolve a inversão 5/9 que aparecia apenas no resumo do
brief da Task 17; não houve renumeração do schema existente.

Todo material dynamic usa a densidade do catálogo. Cada porco usa um compound
assimétrico de cápsula horizontal e esfera deslocada, com densidade
`293.222103729329 kg/m³`, COM positivo no eixo local X e massa total de 65 kg.
Isso preserva silhueta, inércia e orientação estável sem colisores esféricos
sobrepostos. Chapa não recebe fracture pattern. Os padrões de pinho, vidro,
palha e tijolo conservam massa e centro de massa.

Painéis visuais muito finos foram substituídos por proxies físicos esparsos de
moldura, grelha ou gaiola. Toda primitiva `box`, inclusive fragmentos, tem
half-extent mínimo de 0,01 m. Os proxies evitam sólidos maciços ocultos e
espessuras submilimétricas instáveis sem transformar vazios visuais em parede.

Os limites seguem a carga calculada de cada suporte: `pine_fit` usa em geral
5.500 N/3.500 N·m (4.500 N·m no pórtico do equipamento), `glass_clamp`
2.200/1.800, `mortar` 2.200/1.500 no silo (5.000/1.500 na ligação da
plataforma), `straw_bind` 800–1.800/700–1.800 e ruptura `steel_ductile`
7.500/900. A junta histórica da torre do moinho permanece em 1.400/160. Toda
`steel_ductile` possui exatamente um endpoint de material 17; o yield
intermediário permanece 3.200 N/450 N·m no runtime.

## Algoritmo do layout hash

O hash não integra o estado canônico da sessão. Antes da projeção, o JSON é
obrigatoriamente parseado como `LevelManifest` v2 e serializado pelo caminho
canônico tipado; um probe inválido falha em vez de produzir um hash enganoso.
A projeção do fixture possui campos na ordem lógica `level_id`, world, slingshot,
free bodies, bodies, joints, assemblies, triggers e objectives. Arrays de
entidades são ordenados por ID; memberships são ordenadas numericamente. Cada
body usa a tupla posicional documentada abaixo, o que reduz duplicação sem
omitir conteúdo:

```text
[body_id, entity_id, part_id, body_type, affected_by_world_gravity,
 material_id, surface_id, enemy_archetype_id, density_kg_m3,
 transform, shape, visual, fracture_pattern|null]
```

Joints usam `[id, assembly_id, kind, body_a_id, body_b_id, force, torque]`;
assemblies `[id,key,body_ids,joint_ids]`; triggers
`[id,target,kind,threshold,fuse,cooldown,pressure_burst]`; objectives
`[id,kind,target]`. Reais são normalizados para `round(value*100000)` e zero
não preserva sinal. O JSON compacto usa objetos com chaves lexicográficas e
bytes UTF-8 independentes de locale/filesystem. FNV-1a 64 começa em
`14695981039346656037` e multiplica cada byte por `1099511628211` com overflow
unsigned de 64 bits.

Os testes mudam world, launcher, transform, densidade, shape, joint, trigger e
objective por documentos ainda válidos e exigem hash diferente. Reordenar
arrays e memberships semanticamente equivalentes mantém o hash. Grafias JSON
equivalentes (`1`/`1.0` e `-0`/`0`) também produzem exatamente o mesmo hash.

## Compatibilidade Orbital

`first_orbit_v2.level.json` copia semanticamente bodies 1–22, joints,
assemblies e objective de `first_orbit.level.json`, depois adiciona o planeta
explícito como body 23. O v2 usa gravidade radial em `(0,0,0)`, `R=10`,
`g=9`, bounds 60 m e estilingue `k=2200`. A posição de repouso do novo
estilingue é autoral; não finge equivalência byte a byte com a origem variável
do launch ring v1. A equivalência de mecanismo/direção/velocidade será
certificada nas rotas da Task 18.
