# Fazenda — Reação em Cadeia: layout físico v1

Este documento congela a autoria física do nível `farm_reaction`. O manifesto
`game/data/levels/earth/farm_reaction.level.json` é a autoridade de runtime; o
fixture `native/simulation/tests/fixtures/product_v2/farm_reaction_layout_v1.json`
é a projeção auditável e independente de apresentação. Nenhum mesh, node Godot
ou prop de diorama cria body, shape, junta, trigger ou objective.

## Identidade congelada

- `layout_hash`: `fnv1a64:33f8bb0627bc2cf0`.
- 72 bodies, dos quais 54 dynamic e 18 static.
- 52 joints em oito assemblies conexas.
- 12 bodies livres: chão, quatro porcos e sete módulos de cerca static.
- quatro objectives de porco e dois triggers `damage_threshold`.
- oito fragmentos físicos autorados em quatro padrões; cap simultâneo de
  runtime permanece 80.
- bounds AABB `(-24,-12,-12)` a `(48,32,12)` e gravidade uniforme
  `(0,-9.81,0)` em todo body dynamic.

## Inventário por assembly

| Assembly | Bodies | Joints | Conteúdo físico e envelope nominal |
|---|---:|---:|---|
| `ASM_NearShelter` | 6–12 | 1–6 | base, postes e travessas de pinho, parede e teto de palha; origem `(2.6,0,-3.4)`, envelope `(4.2,3.8,3.0)` |
| `ASM_ChainGate` | 13–19 | 7–12 | painel de vidro `(6.7,2.2,0)`, travessa `(4.4,4.2,0)` e contrapeso de chapa `(5.8,4.0,0)` |
| `ASM_HayRamp` | 20–26 | 13–18 | rampa a `-12°`, três fardos em `(7.8,3.4,-0.75/0/0.75)` e suportes |
| `BLD_Farm_Barn` | 27–35 | 19–26 | fundação, estrutura de pinho, dois painéis de vidro e cobertura de palha; envelope `(5.2,5.5,4.0)` |
| `ASM_FarmSilo` | 36–45 | 27–35 | fundação/segmentos de tijolo, alvo lateral `(10.9,1.4,0)`, plataforma e cobertura de chapa |
| `BLD_Farm_Windmill` | 46–51 | 36–40 | base/torre static, hub e três pás com bodies declarados; não existe dano por pá meramente visual |
| `ASM_FarmEquipment` | 52–59 | 41–47 | piso, vagão, trator, estrutura, tanque de combustível e recipiente pressurizado |
| `KIT_Farm_Fences` | 60–65 | 48–52 | dois postes static e quatro travessas dynamic na borda norte |

Os bodies 66–72 são módulos static individuais na borda sul, entre
`x=1..19`, em `z=-6.5`. Eles não formam barreira invisível.

## Inimigos e dispositivos

| Conteúdo | Body / entity | Transform | Contrato |
|---|---|---|---|
| `PIG_FARM_01` | 2 / 2001 | `(2.6,0.55,-3.4)` | 65 kg, objective 1 |
| `PIG_FARM_02` | 3 / 2002 | `(7.4,1.05,1.8)` | 65 kg, objective 2 |
| `PIG_FARM_03` | 4 / 2003 | `(13.0,1.40,0)` | 65 kg, objective 3 |
| `PIG_FARM_04` | 5 / 2004 | `(17.0,0.55,-2.5)` | 65 kg, objective 4 |
| Fuel tank | 58 / 3058 | `(16.2,1.10,-1.6)` | threshold 40, fuse 12, cooldown 180; burst 5 m, 8.000 N·s, 80.000 J, LOS, max32 |
| Pressure vessel | 59 / 3059 | `(17.6,1.00,-3.3)` | threshold 30, fuse 0, cooldown 180; burst 3,5 m, 6.000 N·s, 60.000 J, LOS, max24 |

Os dois dispositivos são chapa dúctil sem padrão de fratura. Cada trigger tem
um único target entity/body, uma causa rastreável e detona no máximo uma vez. O
runtime aplica os caps globais de 12 m/s e 90 J/kg do PressureBurstSystem.

## Materiais, massas e juntas

Os IDs normativos seguem o contrato tipado já congelado na Task 15:
`pine=1`, `brick/masonry=5`, `glass/brittle=9`, `straw=13` e
`sheet_steel=17`. Isso resolve a inversão 5/9 que aparecia apenas no resumo do
brief da Task 17; não houve renumeração do schema existente.

Todo material dynamic usa a densidade do catálogo. Cada porco usa esfera
física de raio 0,45 m e densidade `170.28923952219253 kg/m³`, totalizando
65 kg dentro da tolerância de 0,1%. Chapa não recebe fracture pattern. Os
padrões de pinho, vidro, palha e tijolo dividem o volume em metades simétricas,
conservando massa e centro de massa.

Limites das juntas: `pine_fit` 5.500 N/900 N·m, `glass_clamp` 2.200/350,
`mortar` 1.400/160, `straw_bind` 800/90 e ruptura `steel_ductile`
7.500/900. Toda `steel_ductile` possui exatamente um endpoint de material 17;
o yield intermediário permanece 3.200 N/450 N·m no runtime.

## Algoritmo do layout hash

O hash não integra `LevelManifest` nem o estado canônico da sessão. A projeção
do fixture possui campos na ordem lógica `level_id`, world, slingshot,
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
objective e exigem hash diferente. Reordenar arrays e memberships
semanticamente equivalentes mantém o hash.

## Compatibilidade Orbital

`first_orbit_v2.level.json` copia semanticamente bodies 1–22, joints,
assemblies e objective de `first_orbit.level.json`, depois adiciona o planeta
explícito como body 23. O v2 usa gravidade radial em `(0,0,0)`, `R=10`,
`g=9`, bounds 60 m e estilingue `k=2200`. A posição de repouso do novo
estilingue é autoral; não finge equivalência byte a byte com a origem variável
do launch ring v1. A equivalência de mecanismo/direção/velocidade será
certificada nas rotas da Task 18.
