# Schema de conteúdo do produto 2.0

Este documento é o contrato normativo do JSON `schema_version = 2`. O parser faz o
dispatch pela versão antes de validar as demais chaves da raiz. Ele é atômico e
fail-closed: documento inválido não produz valor parcial; chaves duplicadas ou
desconhecidas, campos ausentes, tipos incorretos, referências órfãs, números não
finitos e valores fora dos limites abaixo são erros.

`source_schema_version` existe apenas nos tipos C++ normalizados e sempre preserva
a versão de origem. Ele não é um campo JSON. As APIs `parse_*_v1` e `parse_*_v2`
rejeitam a outra versão; as APIs históricas `parse_material_catalog`,
`parse_archetype_catalog` e `parse_level_manifest` apenas fazem dispatch explícito.
Não há conversão silenciosa entre versões. `canonical_state_v2` aceita somente
conteúdo v1.

## Limites globais

| Recurso | Limite fechado |
|---|---:|
| Catálogo ou campanha | 256 KiB |
| Nível | 1 MiB |
| Profundidade JSON | 16 |
| Materiais / surfaces | 64 / 64 |
| Abilities / birds / enemies | 32 / 32 / 32 |
| Weakpoints | 32 |
| Bodies / joints / assemblies | 500 / 250 / 128 |
| Memberships totais de assemblies | 1.000 |
| Vértices por convex hull | 64 |
| Children por compound | 16 |
| Profundidade de compound | 4 |
| Bird queue | 32 |
| Triggers / objectives | 64 / 64 |
| Worlds / levels por world | 16 / 64 |

Strings têm 1–128 bytes. IDs numéricos pertencem a `[1, 2^32-1]`, salvo campos
que declaram zero como válido. Entity IDs de conteúdo não podem usar o high bit,
reservado ao runtime. Todo número real deve ser finito e representável como
`float`, sem overflow nem colapso de um valor não zero para zero.

## MaterialCatalog v2

A raiz contém exatamente `schema_version`, `materials` e `surfaces`.

Cada material contém:

| Campo | Tipo / unidade | Faixa |
|---|---|---|
| `id` | ID | positivo, único |
| `key` | string | única no catálogo |
| `response` | enum | `fibrous`, `masonry`, `brittle`, `compressible` ou `ductile` |
| `density_kg_m3` | kg/m³ | `(0, 30000]` |
| `friction` | adimensional | `[0, 1]` |
| `restitution` | adimensional | `[0, 1]` |
| `toughness` | adimensional | `(0, 1]` |

Cada surface contém `id` (único e `>= 1000`), `key` (única),
`density_kg_m3` `(0,30000]`, `friction` `[0,1]` e `restitution` `[0,1]`.

## ArchetypeCatalog v2

A raiz contém exatamente `schema_version`, `presentation_ids`, `score_ids`,
`abilities`, `birds`, `weakpoints` e `enemies`. Os dois registries são arrays
não vazios de strings únicas.

### AbilityArchetype

Cada item contém exatamente `id`, `key`, `kind` e `payload`. O payload é fechado
pelo `kind`:

| Kind | Payload obrigatório | Faixa / unidade |
|---|---|---|
| `gravity_field` | `arm_ticks`, `duration_ticks` | `[1,3600]` ticks |
|  | `radius_m` | `(0,100]` m |
|  | `max_body_mass_kg` | `(0,100000]` kg |
|  | `max_bodies` | `[1,500]` |
|  | `max_acceleration_m_s2` | `(0,1000]` m/s² |
|  | `pulse_speed_m_s` | `(0,1000]` m/s |
| `mass_boost` | `duration_ticks` | `[1,3600]` ticks |
|  | `mass_multiplier` | `(1,20]` |
| `speed_boost` | `impulse_m_s` | `(0,1000]` m/s |
| `explosion` | `radius_m` | `(0,100]` m |
|  | `impulse_n_s` | `(0,10^9]` N·s |
|  | `energy_j` | `(0,10^12]` J |
|  | `max_bodies` | `[1,500]` |
| `split` | `child_count` | `[2,16]` |
|  | `spread_angle_deg` | `(0,180]` graus |
|  | `child_speed_multiplier` | `(0,2]` |

Campos pertencentes a outro payload são chaves desconhecidas e invalidam o
documento.

### BirdArchetype

| Campo | Tipo / unidade | Regra |
|---|---|---|
| `id`, `key` | ID, string | ID único |
| `projectile_visual_id` | string | deve existir em `presentation_ids` |
| `ability_id` | ID | deve existir em `abilities` |
| `surface_id` | ID | validado contra MaterialCatalog no bundle |
| `mass_kg` | kg | `(0,100000]` |
| `radius_m` | m | `(0,100]` |
| `friction`, `restitution` | adimensional | `[0,1]` |
| `bullet` | bool | obrigatório |
| `launch_speed_cap_m_s` | m/s | `(0,1000]` |
| `score_id` | string | deve existir em `score_ids` |
| `icon_id`, `animation_id` | string | devem existir em `presentation_ids` |

### WeakpointProfile

`weakpoints` contém de 0 a 32 itens. Cada item é um objeto fechado:

| Campo | Tipo / unidade | Regra |
|---|---|---|
| `id` | ID | positivo e único entre weakpoints |
| `key` | string | 1–128 bytes |
| `protected_direction` | vec3 | componentes em `[-1,1]`, norma 1 ± 10⁻⁶ |
| `protected_cone_deg` | graus | `(0,180]` |
| `protected_multiplier` | razão | `[0,10]` |
| `exposed_multiplier` | razão | `[0,10]` |

### EnemyArchetype

`enemies` contém de 0 a 32 itens. Cada item é um objeto fechado:

| Campo | Tipo / unidade | Regra |
|---|---|---|
| `id` | ID | positivo e único entre enemies |
| `key` | string | 1–128 bytes |
| `weakpoint_id` | ID | deve existir em `weakpoints` |
| `surface_id` | ID | deve existir em MaterialCatalog quando o bundle é criado |
| `mass_kg` | kg | `(0,100000]` |
| `integrity` | pontos | `(0,100000]` |
| `damage_energy_j_per_kg` | J/kg | `(0,100000]` |
| `max_damage` | pontos | `(0,integrity]` |

No bundle, cada body marcado como enemy deve ser dynamic, usar somente
`surface_id`, usar a mesma surface do archetype e pertencer a uma entidade sem
partes não inimigas nem mistura de archetypes.

## CampaignManifest v2

`game/data/worlds/world_catalog.v2.json` é a única representação normativa do
CampaignManifest. A raiz contém exatamente `schema_version`, `default_world_id`,
`world_order`, `scene_ids`, `diorama_ids`, `text_ids` e `worlds`.

- `default_world_id` é `earth`.
- `world_order` contém exatamente `earth`, `orbital`, nessa ordem, e enumera
  `worlds` sem omissões ou duplicatas.
- `scene_ids`, `diorama_ids` e `text_ids` são registries não vazios e únicos.
- Cada world contém exatamente `id`, `diorama_id`, `text_id`,
  `default_level_id`, `level_order` e `levels`.
- Diorama/texto devem existir nos registries; o default deve existir no world.
- `level_order` deve reproduzir a ordem de `levels` sem omissões.
- Cada level contém exatamente `id`, `region_id`, `camera_profile_id`,
  `presentation_profile_id`, `scene_id` e `unlock_after_level_id`.
- `scene_id` deve estar registrado. O primeiro level tem unlock `null`; cada
  level seguinte referencia exatamente o level anterior. Assim os dois mundos
  começam disponíveis e o unlock dentro de cada world é linear.

## LevelManifest v2

A raiz contém exatamente:

`schema_version`, `id`, `world_id`, `region_id`, `camera_profile_id`,
`presentation_profile_id`, `world`, `slingshot`, `bird_queue`, `scoring`,
`free_body_ids`, `bodies`, `joints`, `assemblies`, `triggers`, `objectives`,
`settle_policy` e `watchdog_ticks`.

O bundle atômico resolve `world_id` + `id` no CampaignManifest e exige igualdade
exata de `region_id`, `camera_profile_id` e `presentation_profile_id` com o level
registrado. Também resolve queue, surfaces, materiais e enemy archetypes.

### WorldDefinition

Variante `uniform`:

- `kind = "uniform"`;
- `acceleration_m_s2`: vetor de três componentes em `[-1000,1000]` m/s² e não zero;
- `bounds`: objeto fechado com `min_m` e `max_m`, vetores em
  `[-100000,100000]` m, com `min < max` em cada eixo.

Variante `radial`:

- `kind = "radial"`;
- `center_m`: vetor em `[-100000,100000]` m;
- `reference_radius_m`: `(0,100000]` m;
- `reference_acceleration_m_s2`: `(0,1000]` m/s²;
- `bounds`: objeto fechado com `radius_m` em
  `(reference_radius_m,1000000]` m.

Campos uniform em radial, radiais em uniform ou qualquer kind adicional são
rejeitados.

### SlingshotDefinition

| Campo | Tipo / unidade | Faixa / valor |
|---|---|---|
| `asset_id` | string | obrigatório |
| `rest_position_m` | vec3 m | `[-100000,100000]` por eixo |
| `rest_rotation_xyzw` | quaternion | componentes `[-1,1]`, norma 1 ± 10⁻⁶ |
| `spring_constant_n_m` | N/m | `(0,10^9]` |
| `energy_efficiency` | razão | `(0,1]` |
| `minimum_extension_m` | m | `[0,100]` |
| `maximum_extension_m` | m | `(minimum,100]` |
| `plane_policy` | enum | `gravity_vertical_camera_yaw` |
| `projectile_clearance_m` | m | `[0,100]` |
| `speed_ceiling_m_s` | m/s | `(0,1000]` |

### Shapes e bodies

Shapes são variantes fechadas:

- `box`: `half_extents_m`, vec3 `[0.0001,1000]` m;
- `sphere`: `radius_m` `(0,1000]` m;
- `capsule`: `radius_m` e `half_height_m`, ambos `(0,1000]` m;
- `convex_hull`: `vertices_m`, 4–64 vec3 em `[-1000,1000]` m, com volume
  não zero;
- `compound`: `children`, 1–16 shapes, profundidade máxima 4.

Cada body é um objeto fechado com os campos abaixo. `bodies` contém de 0 a
500 itens; `body_id` é único e o par `(entity_id, part_id)` também é único.

| Campo | Tipo / unidade | Regra |
|---|---|---|
| `body_id` | ID | positivo e único no nível |
| `entity_id` | ID | positivo; high bit reservado ao runtime |
| `part_id` | ID | positivo; único dentro da entidade |
| `body_type` | enum | `static` ou `dynamic` |
| `affected_by_world_gravity` | bool | obrigatório; deve ser `true` para dynamic |
| `material_id` | ID ou `null` | referência resolvida no bundle |
| `surface_id` | ID ou `null` | referência resolvida no bundle |
| `enemy_archetype_id` | ID ou `null` | referência resolvida no bundle |
| `density_kg_m3` | kg/m³ | `[0,30000]` para static; `(0,30000]` para dynamic |
| `transform` | objeto | exatamente `position_m` e `rotation_xyzw` normalizado |
| `shape` | objeto | uma das variantes fechadas acima |
| `visual` | objeto | exatamente `asset_id` e `bounds_m` vec3 em `[0.0001,2000]` |

Exatamente um entre `material_id` e `surface_id` é não nulo. Um enemy body é
dynamic, não usa material, usa a surface de seu EnemyArchetype e sua entidade
contém exatamente um archetype inimigo, sem partes não inimigas.

### Ownership, joints e assemblies

| Array | Cardinalidade | Regra dos itens |
|---|---:|---|
| `free_body_ids` | 0–500 | IDs positivos, existentes e sem duplicatas |
| `joints` | 0–250 | IDs únicos; todos pertencem exatamente a uma assembly |
| `assemblies` | 0–128 | IDs únicos; listas não vazias; até 1.000 memberships totais |
| `objectives` | 1–64 | IDs únicos |

Todo body aparece exatamente uma vez: em `free_body_ids` ou em `body_ids` de
uma única assembly. Nenhum body livre pode ser endpoint de joint.

Cada joint é um objeto fechado:

| Campo | Tipo / unidade | Regra |
|---|---|---|
| `id` | ID | positivo e único |
| `assembly_id` | ID | deve existir e coincidir com a dona do joint |
| `kind` | enum | `pine_fit`, `glass_clamp` ou `mortar` |
| `body_a_id`, `body_b_id` | ID | existentes, distintos e ambos na mesma assembly do joint |
| `force_limit_n` | N | `(0,10^12]` |
| `torque_limit_nm` | N·m | `(0,10^12]` |

Cada assembly é um objeto fechado:

| Campo | Tipo | Regra |
|---|---|---|
| `id` | ID | positivo e único |
| `key` | string | 1–128 bytes |
| `body_ids` | array de IDs | 1–500, sem duplicatas locais; todos pertencem à assembly |
| `joint_ids` | array de IDs | 1–250, sem duplicatas locais; todos pertencem à assembly |

O total de memberships de todas as assemblies é no máximo 1.000. O grafo
formado pelos bodies como vértices e joints como arestas deve ser conexo.

Cada objective é um objeto fechado:

| Campo | Tipo | Regra |
|---|---|---|
| `id` | ID | positivo e único |
| `kind` | enum | somente `neutralize_entity` |
| `target_entity_id` | ID | entidade puramente inimiga com exatamente um EnemyArchetype |

Uma entidade alvo ausente, não inimiga ou mista é inválida.

### Bird queue e scoring

`bird_queue` é array não vazio de 1–32 BirdArchetype IDs. Duplicatas são válidas
e a ordem é semântica: canonical JSON nunca a classifica.

`scoring` contém exatamente:

| Campo | Unidade | Faixa |
|---|---|---|
| `pig_points`, `unused_bird_points` | pontos | `[0,1000000]` |
| `star_thresholds` | pontos | exatamente 3 inteiros positivos, estritamente crescentes |
| `chain_window_ticks` | ticks | `[1,3600]` |
| `chain_multiplier_step` | razão | `[0,10]` |
| `max_chain_multiplier` | razão | `[1,100]` |

### EnvironmentalTriggerDefinition

Cada trigger contém exatamente `id`, `target_entity_id`,
`kind = "damage_threshold"`, `damage_threshold` `(0,10^6]`, `fuse_ticks` e
`cooldown_ticks` `[0,36000]`, e `pressure_burst`. O alvo deve ser uma entidade
de body do mesmo nível e o ID do trigger é único. `temperature` e qualquer kind
desconhecido são rejeitados.

Pressure burst contém exatamente:

- `radius_m`: `(0,1000]` m;
- `impulse_n_s`: `(0,10^9]` N·s;
- `energy_j`: `(0,10^12]` J;
- `line_of_sight`: bool;
- `max_bodies`: `[1,500]`.

### Settle e watchdog

`settle_policy` contém `linear_speed_m_s` e `angular_speed_rad_s`, ambos
`(0,100]`, e `rest_ticks` `[1,3600]`. `watchdog_ticks` fica em `[1,36000]` e
deve ser estritamente maior que `rest_ticks`.

## Canonicalização e validação cruzada

`to_canonical_json` preserva o schema de origem. Em v2, ordem de arrays
semânticos (worlds, levels e bird queue) é preservada; objetos usam a ordem
canônica da biblioteca JSON. Um parse do resultado deve gerar exatamente os
mesmos bytes JSON no round-trip seguinte.

`make_product_v2_content_bundle` é o gate de referência cruzada e aceita apenas
quatro valores com `source_schema_version = 2`: MaterialCatalog,
ArchetypeCatalog, CampaignManifest e LevelManifest. O bundle legado aceita apenas
v1. Assim, nenhum caminho interpreta um documento de uma versão como a outra.
