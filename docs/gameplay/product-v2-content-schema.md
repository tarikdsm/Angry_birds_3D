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
| Primitivas expandidas por sessão | 256 |
| Bird queue | 32 |
| Triggers / objectives | 64 / 64 |
| Fragmentos físicos autorados ativos | 80 |
| Worlds / levels por world | 16 / 64 |

Strings têm 1–128 bytes. IDs numéricos pertencem a `[1, 2^32-1]`, salvo campos
que declaram zero como válido. Entity IDs de conteúdo não podem usar o high bit,
reservado ao runtime. Todo número real deve ser finito e representável como
`float`, sem overflow nem colapso de um valor não zero para zero.

## MaterialCatalog v2

A raiz contém exatamente `schema_version`, `materials` e `surfaces`.
`materials` e `surfaces` são arrays não vazios com 1–64 itens cada.

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

O preset terrestre recomendado preserva os IDs históricos e usa:

| Material | Response | Densidade | Atrito | Restituição | Falha específica |
|---|---|---:|---:|---:|---:|
| pinho (`id=1`) | `fibrous` | 520 | 0,55 | 0,15 | 65 J/kg acumulados (`toughness=0,26`) |
| tijolo (`id=5`) | `masonry` | 1.800 | 0,72 | 0,05 | 130 J/kg acumulados (`toughness=0,52`) |
| vidro (`id=9`) | `brittle` | 2.450 | 0,38 | 0,08 | pico individual de 18 J/kg (`toughness=0,072`) |
| palha (`id=13`) | `compressible` | 110 | 0,82 | 0,12 | 22 J/kg acumulados (`toughness=0,088`) |
| chapa (`id=17`) | `ductile` | 7.800 | 0,48 | 0,10 | não usa a fratura comum |

`brittle` conserva o maior pico individual; `fibrous`, `masonry` e
`compressible` acumulam energia. `ductile` somente cede e rompe por juntas
`steel_ductile`.

## ArchetypeCatalog v2

A raiz contém exatamente `schema_version`, `presentation_ids`, `score_ids`,
`abilities`, `birds`, `weakpoints` e `enemies`. Os dois registries são arrays
não vazios de strings únicas. `abilities` e `birds` contêm 1–32 itens cada;
`weakpoints` e `enemies`, 0–32 itens cada.

### AbilityArchetype

Cada item contém exatamente `id`, `key`, `kind` e `payload`. `id` é positivo
e único entre abilities. O payload é fechado pelo `kind`:

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
|  | `max_bodies` | `[1,32]` |
| `split` | `child_count` | `[2,16]` |
|  | `spread_angle_deg` | `(0,180]` graus |
|  | `child_speed_multiplier` | `(0,2]` |

Campos pertencentes a outro payload são chaves desconhecidas e invalidam o
documento.

O runtime atual implementa `split` de forma fail-closed somente para o preset
`child_count=3`, `spread_angle_deg=11` e
`child_speed_multiplier=3/(1+2*cos(11°))`, comparado na resolução canônica de
`10^-5`. O multiplicador é recalculado pela simulação, não aceito como fonte de
verdade física. Catálogos futuros com outros presets continuam parseáveis
dentro das faixas acima, mas não podem criar uma sessão nesta versão. Toda ave
que referencia `split` exige `CHR_BlueChild` em `presentation_ids`, e sua massa
e raio também devem produzir filhas Box3D-seguras com `m/3` e
`cbrt(1/3)*raio`.

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

`mass_kg` e `radius_m` também devem formar uma esfera numericamente segura no
runtime do Box3D. A densidade derivada após estreitamento para `float`, a massa
recalculada pela esfera e seu recíproco devem permanecer finitos, positivos e
não nulos. Essa regra é baseada nas operações do solver, sem um limite decimal
adicional de massa.

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
| `damage_model` | enum opcional | `legacy_directional_energy` (omissão) ou `terrestrial_pig` |

No bundle, cada body marcado como enemy deve ser dynamic, usar somente
`surface_id`, usar a mesma surface do archetype e pertencer a uma entidade sem
partes não inimigas nem mistura de archetypes.

`terrestrial_pig` é um contrato tipado, nunca inferido de `key` ou visual. Ele
exige massa física total de 65 kg ±0,065 kg por entidade (um compound ou a soma
de vários bodies/parts), surface com atrito 0,65 e restituição 0,05,
weakpoint uniforme (`protected_multiplier=exposed_multiplier=1`),
`damage_energy_j_per_kg=18` e `max_damage=70`. O contato usa as grandezas raw:
`E=0,5*m_effective*v_normal²` e
`damage=clamp((E/m_pig-18)*0,9,0,70)`. Burst/explosão passa a energia externa
pelo mesmo DamageSystem. `m_pig` é sempre a massa autoritativa da entidade, não
a massa do part atingido; o `m_effective` continua sendo o valor físico do par
em contato. Um mesmo `EventId` causal aplica dano no máximo uma vez por entidade
inimiga, mesmo se alcançar vários parts, e o receipt persistente integra o
canonical. Em mundo uniforme, sair do AABB neutraliza uma vez por `BoundsExit`;
em mundo radial permanece `Ejection`.

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

Toda shape aceita `local_transform` opcional, com `position_m` e
`rotation_xyzw` normalizado. A omissão equivale à identidade. Em compounds, o
transform do filho é composto deterministicamente com o de seus ancestrais; o
JSON canônico publica a identidade explicitamente.

- `box`: `half_extents_m`, vec3 `[0.0001,1000]` m;
- `sphere`: `radius_m` `(0,1000]` m;
- `capsule`: `radius_m` e `half_height_m`, ambos `(0,1000]` m;
- `convex_hull`: `vertices_m`, 4–64 vec3 em `[-1000,1000]` m. Existe ao
  menos uma combinação de três arestas partindo do primeiro vértice cujo produto
  triplo escalar satisfaz `abs(determinant) > 1e-9 m³`; esse determinante é seis
  vezes o volume assinado do tetraedro e é o critério exato de não degeneração;
- `compound`: `children`, 1–16 shapes, profundidade máxima 4.

O parser aceita a faixa geral acima. A autoria física da Fazenda adota um
contrato mais estrito: toda primitiva `box` usada por body ou fragmento possui
half-extent mínimo de 0,01 m. Superfícies visuais finas são representadas por
molduras, grelhas ou gaiolas esparsas, não por sólidos submilimétricos.

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
| `fracture_pattern` | objeto opcional | somente para body de material não inimigo |

Exatamente um entre `material_id` e `surface_id` é não nulo. Um enemy body é
dynamic, não usa material, usa a surface de seu EnemyArchetype e sua entidade
contém exatamente um archetype inimigo, sem partes não inimigas.

`fracture_pattern` contém exatamente `physical_fragments` e
`cosmetic_asset_ids`. Cada fragmento físico declara `ordinal` positivo e único,
`shape`, `local_transform`, `density_kg_m3` e `visual_id`. A soma das massas
geométricas dos fragmentos deve igualar a massa do pai em ±0,1%; convex hull
usa o volume e o centroide do hull, não o AABB. A soma `Σm*r` também deve
preservar o centro de massa do pai. O catálogo pode autorar mais de 80
fragmentos entre padrões distintos, mas o runtime admite no máximo 80
fragmentos físicos simultaneamente ativos e rejeita cada substituição excedente
de forma atômica. IDs cosméticos não criam body, DamageState, identidade física
nem alteram o canonical físico. Enemy bodies não podem declarar padrão de
fratura.

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
| `kind` | enum | `pine_fit`, `glass_clamp`, `mortar`, `straw_bind` ou `steel_ductile` |
| `body_a_id`, `body_b_id` | ID | existentes, distintos e ambos na mesma assembly do joint |
| `force_limit_n` | N | `(0,10^12]` |
| `torque_limit_nm` | N·m | `(0,10^12]` |

O preset terrestre usa argamassa em 1.400 N/160 N·m e `straw_bind` em
800 N/90 N·m. `steel_ductile` faz `Elastic→Yielded` em 3.200 N ou 450 N·m,
recria a constraint no tick seguinte sob o mesmo handle e somente um solver
posterior pode rompê-la em 7.500 N ou 900 N·m. A causa de `MaterialYielded`
considera apenas dano do tick atual em um dos dois endpoints; sem incidente
correspondente, a causa é zero e nunca reutiliza um evento global antigo.

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
- `max_bodies`: `[1,32]`, limitado por identidade de domÃ­nio.

### Esmagamento terrestre

Para cada entidade de porco terrestre, o runtime soma uma vez o impulso normal
agregado de todos os seus parts no tick, usa o COM ponderado da entidade e
compara `impulse/(mass*|gravity|*dt)`, quantizado em `10^-5`. Somente
razão estritamente maior que 4 mantida por 21 ticks produz dano. O excesso
acumula `delta_v += (ratio-4)*g*dt`; no 21º tick a energia externa é
`mass*(18+0,5*delta_v²)`. Qualquer interrupção zera a janela. A ordem causal é
`CrushDamageApplied → DamageApplied → EntityNeutralized`, com os dois
últimos apontando diretamente ao primeiro.

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

## Estado canônico binário v3

Sessões com `source_schema_version = 2` publicam exclusivamente
`canonical_state_v3`/`canonical_hash_v3`; nelas, v2 permanece vazio e com hash
zero. Sessões v1 continuam publicando exclusivamente `canonical_state_v2`; v3
fica vazio e com hash zero. Não existe conversão entre os dois layouts.

O encoding v3 usa little-endian e os seguintes primitivos:

- inteiros: largura declarada no campo (`u8`, `u32` ou `u64`);
- booleano: `u8`, somente 0 ou 1;
- texto: tamanho UTF-8 `u32`, seguido pelos bytes sem terminador;
- ID: a largura da representação forte (`u32`, exceto Event/Tick `u64`);
- opcional: booleano de presença seguido pelo payload quando presente;
- real/vetor/quaternion: `i64 = round(valor * 100000)`; qualquer zero,
  inclusive `-0`, vira `+0`; NaN, infinito e overflow falham sem substituir o
  cache anteriormente publicado;
- coleção: cardinalidade `u32` seguida pelos itens.

Nenhuma tag depende da posição de `std::variant`. As tags são append-only:

| Família | Tags v3 |
|---|---|
| world/gravity | `uniform=0`, `radial=1` |
| bounds | `aabb=0`, `spherical=1` |
| ability kind | `legacy_gravity_field=0`, `gravity_field=1`, `mass_boost=2`, `speed_boost=3`, `explosion=4`, `split=5` |
| ability runtime/payload | `gravity_field=0`, `mass_boost=1`, `speed_boost=2`, `explosion=3`, `split=4` |
| command | `begin_aim=0`, `set_aim=1`, `launch=2`, `activate_ability=3`, `cancel_aim=4`, `begin_grab=5`, `set_pull=6`, `release_bird=7`, `cancel_grab=8` |
| phase | `inspection=0`, `aim=1`, `flight_ability=2`, `resolution=3`, `evaluation=4`, `result=5`, `faulted=6`, `grabbed=7` |
| outcome | `none=0`, `victory=1`, `defeat=2` |
| command rejection | `none=0`, `invalid_phase=1`, `invalid_aim=2`, `not_armed=3`, `no_bird_available=4`, `ability_unavailable=5` |
| neutralization cause | `none=0`, `integrity_depleted=1`, `ejection=2`, `bounds_exit=3` |
| damage classification | `none=0`, `protected=1`, `vulnerable=2` |
| material response | `fibrous=0`, `masonry=1`, `brittle=2`, `compressible=3`, `ductile=4` |
| body | `static=0`, `dynamic=1` |
| shape | `box=0`, `sphere=1`, `capsule=2`, `convex_hull=3`, `compound=4` |
| joint | `pine_fit=0`, `glass_clamp=1`, `mortar=2`, `straw_bind=3`, `steel_ductile=4` |
| objective | `neutralize_entity=0` |
| environmental trigger | `damage_threshold=0` |
| event | `bird_launched=0`, `ability_activation_requested=1`, `command_rejected=2`, `ability_started=3`, `ability_affected_body=4`, `ability_pulse=5`, `ability_ended=6`, `damage_applied=7`, `entity_neutralized=8`, `joint_overloaded=9`, `piece_fracture_triggered=10`, `joint_broken=11`, `piece_fractured=12`, `mass_changed=13`, `speed_changed=14`, `projectile_split=15`, `projectile_spawned=16`, `explosion_fuse_armed=17`, `pressure_burst=18`, `environmental_trigger_armed=19`, `environmental_trigger_detonated=20`, `material_yielded=21`, `crush_damage_applied=22`, `score_awarded=23`, `chain_changed=24`, `stars_awarded=25` |

### Ordem do stream v3

O stream possui exatamente esta ordem de blocos:

1. texto `canonical_state_v3` e `u32` de versão igual a 3;
2. conteúdo imutável: MaterialCatalog, ArchetypeCatalog e LevelManifest;
3. `tick`, phase, outcome, birds_remaining, sequências do próximo comando e
   evento, último comando processado, launch_count, resolution_rest_ticks e
   objective_complete;
4. `current_score u64` e `stars u8`, ambos reservados como zero até a Task 16;
5. aim opcional, launcher/plano/pull opcional e last_impact opcional;
6. snapshots, joints publicados, eventos e estados de dano;
7. peças fraturadas, quebras/fraturas pendentes e fila de comandos;
8. ShotState opcional e contadores runtime de joints.

Conteúdo sem os novos contratos preserva os bytes anteriores. Quando existe
um `terrestrial_pig`, o bloco condicional acrescenta estados Crush ordenados por
`(EntityId,PartId)` (`streak`, excesso e causa) e `was_bounds_exit`. Quando
existem juntas `steel_ductile`, outro bloco condicional acrescenta JointId,
estado, pending, frames capturados, causa, tick de yield e confirmação de solver.

O bloco imutável escreve catálogos por ID crescente; registries de strings por
ordem lexicográfica; e bodies, joints, assemblies, objectives e triggers por ID.
A ordem autoral de `bird_queue` é preservada e nunca classificada. Cada ability
inclui os campos normalizados usados pelo runtime, a tag e o payload fechado.
World escreve gravity kind+payload e depois bounds kind+payload. Slingshot
escreve asset, rest position/rotation, `k`, eficiência, deadzone, extensão
máxima, plane policy, clearance e speed ceiling, nessa ordem.

Shapes escrevem tag e local transform antes do payload: half-extents para box;
radius para sphere; radius/half-height para capsule; vértices na ordem autoral
para hull; filhos na ordem autoral e recursivamente para compound. Nenhum mesh,
handle ou ponteiro entra no stream.

LauncherState escreve rest position, camera_right aceita, up, horizontal,
plane_normal, pull horizontal/vertical, extension, spring/launch energy,
launch_direction, predicted speed, deadzone e extensão máxima. Assim, uma
camera_right aceita diferente muda v3, embora input rejeitado não altere o plano.
Esse bloco público existe somente durante `Grabbed`: cancel, release abaixo da
deadzone e release válido o removem. No release válido, o plano e pull aceitos
já foram copiados para ShotState; portanto o stream pós-release usa ShotState e
não conserva ghost/frame residual no launcher. A transição seguinte para
`Inspection`, restart e reconfigure também publicam launcher ausente.

Cada comando pendente preserva a ordem da deque e escreve sequence, tag e seu
payload: SetAim escreve aim completo; BeginGrab escreve camera_right; SetPull
escreve os dois componentes; os demais não têm payload.

ShotState escreve `shot_id`, bird, ability, launch_tick, plano travado
(camera_right/up/horizontal/plane_normal), pull aceito, activation_consumed,
AbilityRuntime e projéteis. O runtime escreve tag, start/end ticks opcionais e
active. `speed_boost` anexa a última direção de voo válida. `split` anexa a
identidade da fonte, os três child IDs em ordem angular, o fim opcional do grace,
`applied` e `filters_restored`; nenhuma dessas extensões adiciona bytes às
outras alternativas. ShotState encapsula a coleção: `insert` mantém ordem estritamente
crescente por EntityId e rejeita duplicatas sem mutação; `replace` preserva a
identidade; `erase` nunca deixa um ShotState vazio; e o primário é sempre o
menor EntityId. `replace_all_projectiles` valida antecipadamente uma coleção
não vazia, única e crescente e a publica por `swap` sem exceção, permitindo que
o Split substitua a fonte pelas três filhas sem inserts alocantes. Assignment de
ProjectileState copia somente o runtime e preserva
a identidade do slot, inclusive através do acesso mutável ao primário. Insert e
erase publicam a nova coleção por rebuild+swap, sem usar assignment para mover
identidades. ShotState não possui construtor default nem move construction e
não pode ser atribuído; ele nasce com um ProjectileState obrigatório, pode ser
copiado e a sessão o constrói diretamente com `optional.emplace`. Assim, até o
caminho legado/default significa "um disparo com um projétil", nunca uma coleção
vazia ou uma origem esvaziada por move. O serializer valida essas invariantes e
falha sem publicar cache parcial se o estado estiver vazio, duplicado ou fora de
ordem; ele não ordena
uma cópia nem resolve empates. Cada projétil escreve EntityId, bullet, age/rest
ticks, finished e pending_destroy. O BodyHandle é deliberadamente excluído.

Snapshots são serializados por `(EntityId, PartId)` e incluem todos os campos
publicados, inclusive `ejected`, `exited_world` e `is_projectile`. Damage states,
peças e pendências são ordenados pela identidade de domínio; eventos e comandos
preservam a ordem causal/de fila. Métricas de tempo, durações de frame, handles,
endereços e estado de apresentação/save ficam fora do stream.

`canonical_hash_v3` é FNV-1a 64 sobre os bytes acima. O fixture mínimo fica
congelado independentemente dos goldens v2; 50 serializações sem mutação devem
ser byte-idênticas.

## Documentos auxiliares de produto

`game/data/assets/product_v2.assets.json` possui raiz fechada
`schema_version,assets`. Cada entrada contém exatamente `id`, `kind`, `status`,
`resource_path`, `node_path` e `presentation_only`. IDs são únicos e
case-sensitive. `status` é `required` quando o recurso já integra o produto e
`planned` quando o ID final está reservado para as Tasks 25–27. Um recurso
planned nunca autoriza fallback, primitiva final ou collider automático.

`resource_path` fica sob `res://assets/product_v2/` ou, somente para a fase
legada preservada, `res://assets/vertical_slice/`; `..` é inválido.
`node_path` é obrigatório e estável para subassets, inclusive todos os
`FRAG_*`, `CHR_BlueChild` e `KIT_Farm_Metal`. Assets presentation-only não
podem ser referenciados por body, shape ou fragmento físico.

`game/data/feedback/product_v2.feedback.json` possui raiz fechada
`schema_version,budgets,material_profiles,event_profiles,outcome_profiles,profiles`.
Ele registra os cinco material IDs e todos os eventos append-only até
`StarsAwarded=25`; não altera física, causalidade ou score.

## Fixture físico da Fazenda

O fixture `farm_reaction_layout_v1.json` contém apenas
`schema_version,level_id,layout_hash,counts,projection`. A projeção e o FNV-1a
64 estão definidos em `docs/gameplay/farm-reaction-layout-v1.md`. O hash é uma
evidência de autoria e não é campo de LevelManifest nem parte do canonical da
sessão. A tabela normativa tipada mantém `brick/masonry=5` e
`glass/brittle=9`; qualquer resumo posterior que inverta 5/9 é stale e não
renumera o contrato.

A projeção é sempre derivada de um `LevelManifest` v2 aceito pelo parser e
resserializado por `to_canonical_json`; não se calcula hash diretamente de uma
árvore JSON sem tipo. Depois disso, reais finitos são quantizados em `10^-5`,
de modo que grafias semanticamente equivalentes (`1`/`1.0`, `-0`/`0`) geram os
mesmos bytes. Mutation probes também precisam continuar válidos no parser e no
bundle fechado antes de poderem demonstrar mudança de hash. O gate físico da
Fazenda ainda executa 120 ticks ociosos e exige zero dano, yield, ruptura,
fratura, trigger ou neutralização, com todas as juntas ativas ao final.
