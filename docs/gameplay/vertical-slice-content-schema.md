# Contrato de conteúdo do vertical slice (schema v1)

Os três documentos JSON de simulação são carregados separadamente e só formam um `ContentBundle` depois da validação cruzada. O quarto documento, de feedback audiovisual, é validado pelo `FeedbackDirector` antes da criação dos pools. O quinto documento contém o catálogo pt-BR do HUD. Todo objeto rejeita chaves desconhecidas, todo campo listado é obrigatório e nenhum valor ausente recebe default. Uma ausência intencional é representada por `null` explícito, exceto pelo texto vazio canônico de `hud.outcome.none`.

## Feedback audiovisual

Documento: `game/data/feedback/vertical_slice.feedback.json`.

Raiz: `schema_version`, `budgets`, `directional_anchor`, `material_profiles`,
`event_profiles`, `outcome_profiles`, `profiles`.

- `schema_version` é o inteiro `1`.
- `budgets` exige `vfx_pool_size`, `audio_voice_pool_size`, `fragment_pool_size`,
  `max_particles_per_slot`, `max_particles_total`, `max_fragments_total`,
  `glass_screen_coverage_limit` e `first_impact_limit_ms`. Contagens são inteiros
  positivos; limites são números finitos positivos. As capacidades totais não
  podem ser menores que os pools configurados e a cobertura pertence a `(0, 1]`.
- `directional_anchor` exige os inteiros positivos `entity_id` e
  `enemy_archetype_id`.
- `material_profiles` tem exatamente as chaves `1`, `5`, `9`;
  `event_profiles`, os treze eventos canônicos do slice; `outcome_profiles`,
  `victory` e `defeat`. Todo valor é o nome não vazio de um item existente em
  `profiles`. Em `event_profiles`, `damage_applied` também aceita o seletor
  `material_or_anchor`; `joint_overloaded`, `piece_fracture_triggered`,
  `joint_broken` e `piece_fractured` também aceitam `material`. Esses seletores
  resolvem o perfil por material ou pelo estado do Âncora antes da emissão e são
  inválidos nos demais eventos.
- Cada entrada de `profiles` exige exatamente `color`, `particles`, `lifetime_s`,
  `size_m` e `audio`. `color` é uma cor HTML válida, `particles` é inteiro
  positivo, tempos e tamanhos são finitos e positivos, e `audio` é string (vazia
  representa ausência intencional de cue).

Falha de parse ou de validação rejeita o documento inteiro antes de qualquer pool
ser criado; não há preenchimento por defaults.

## Catálogo de textos do HUD

Documento: `game/data/ui/vertical_slice.pt-BR.json`.

Raiz: `schema_version`, `locale`, `messages`.

- `schema_version` é o inteiro `1` e `locale` é exatamente `pt-BR`.
- `messages` contém o conjunto exato e fechado de IDs `hud.phase.*`,
  `hud.controls.*`, `hud.outcome.*`, `hud.format.*` e `hud.accessibility.*`
  consumidos pelo vertical slice. Os IDs de fase e resultado continuam sendo os
  IDs canônicos da simulação; somente sua apresentação é resolvida pelo catálogo.
- Todas as mensagens são strings. Somente `hud.outcome.none` pode ser vazia.
  Templates preservam seus tokens tipados: `%s`, `%d` ou `%03d`, conforme o ID.
- O HUD carrega e valida o documento uma única vez em `_ready()`. Arquivo ausente,
  JSON malformado, chave desconhecida, chave obrigatória ausente, tipo incorreto
  ou token incompatível rejeitam o documento inteiro. Nesse caso, nenhuma
  tradução parcial é usada: o HUD apresenta IDs técnicos seguros entre colchetes
  e mantém os controles de recuperação identificáveis para diagnóstico.
- Fase ou outcome desconhecido em runtime resolve exclusivamente para
  `hud.phase.unknown`, `hud.controls.unknown` e `hud.outcome.unknown`; nunca é
  interpolado como um novo ID de catálogo.
- Os estados canônicos de habilidade `unavailable`, `arming`, `armed`, `active`
  e `spent`, mais rejeição antecipada `not_armed`, possuem mensagens próprias;
  readiness desconhecida resolve para `hud.controls.flight_ability.unknown`.

## Catálogo de materiais

Raiz: `schema_version`, `materials`, `surfaces`.

- `materials[]`: `id`, `key`, `response`, `density_kg_m3`, `friction`, `restitution`, `toughness`.
- `response`: `fibrous`, `masonry` ou `brittle`.
- `surfaces[]`: `id`, `key`, `density_kg_m3`, `friction`, `restitution`.
- IDs de surface usam a faixa reservada a partir de 1000 e não contam como materiais destrutíveis.

O slice reserva material 1/pinho, 5/tijolo e 9/vidro; surfaces 1001/planeta, 1002/plataforma, 1003/Virela e 1004/armadura.

## Catálogo de arquétipos

Raiz: `schema_version`, `abilities`, `birds`, `weakpoints`, `enemies`.

- `abilities[]`: ID/chave, tipo e todos os limites físicos/temporais da habilidade.
- `birds[]`: ID/chave, referências de habilidade/surface, massa, densidade, raio, atrito, restituição e bullet.
- `weakpoints[]`: direção protegida normalizada, cone e multiplicadores.
- `enemies[]`: referências de weakpoint/surface, massa, integridade e calibração/teto de dano.

Referências internas (ave→habilidade e inimigo→weakpoint) são verificadas no próprio catálogo. Surfaces são verificadas no bundle.

## Manifesto de nível

`EntityId` de conteúdo ocupa somente `1..0x7fffffff`. O bit alto (`0x80000000`) é reservado ao namespace determinístico de entidades criadas em runtime, como projéteis, e é rejeitado no planeta, corpos e objetivos do manifesto.

Raiz: `schema_version`, `id`, `planet`, `launch_ring`, `bird_roster`, `free_body_ids`, `bodies`, `joints`, `assemblies`, `objectives`.

- Cada body declara IDs de body/entity/part, tipo, `material_id` e `surface_id` (exatamente um não nulo), `enemy_archetype_id`, densidade, transform, shape e visual.
- Shapes `box` exigem `half_extents_m`; `sphere` exige `radius_m`. Bounds visuais devem ser iguais ao tamanho completo do proxy.
- Cada joint declara endpoints, assembly, enum de encaixe e limites finitos e estritamente positivos em N e N·m.
- `free_body_ids` declara explicitamente os corpos intencionalmente livres; todo outro body pertence a exatamente um assembly.
- Assemblies exigem listas não vazias de bodies e joints, ownership exclusivo, joints bidirecionalmente coerentes, endpoints internos e grafo conectado.
- Objetivos exigem alvo explícito.

IDs, pares `(EntityId,PartId)` e referências órfãs, números não finitos/fora do intervalo, quaternions/direções não normalizados e ranges geométricos inconsistentes bloqueiam a carga. `neutralize_entity` deve resolver para uma única entidade com `enemy_archetype_id`.

## Erros e serialização

As APIs públicas `parse_*` e `make_content_bundle` são `noexcept` e retornam `ContentErrorCode`, JSON pointer RFC 6901 e mensagem curta. Tokens escapam `~` como `~0` e `/` como `~1`; itens de array usam seu índice real. O conteúdo de entrada não é ecoado. `to_canonical_json` ordena chaves de forma estável e permite parse→serialize→parse sem perda semântica.

Chaves JSON duplicadas são rejeitadas antes da construção do documento, tanto na raiz quanto em objetos aninhados. Números físicos positivos precisam continuar finitos e estritamente positivos após conversão para `float`; o mínimo técnico é `std::numeric_limits<float>::denorm_min()`.

## Budgets de carga

- catálogos de materiais e arquétipos: até 256 KiB cada;
- manifesto de nível: até 1 MiB;
- nesting JSON: até 16 containers;
- materiais/surfaces: 64/64;
- abilities/birds/enemies: 16 cada; weakpoints: 32;
- roster: 16; bodies: 500; joints: 250; assemblies: 128; objectives: 64;
- corpos livres: 500; memberships por assembly: 500 bodies e 250 joints; total: 1000.

O tamanho em bytes é verificado antes do parse. Coleções reservam sua capacidade declarada e referências são validadas por índices hash para custo linear no número de registros.

## Consistência física cruzada

- density de body é igual à density do material/surface referenciado, com tolerância relativa `1e-9`;
- birds repetem density/friction/restitution da surface com tolerância `1e-9`;
- massa de bird é `4/3·π·r³·density` com erro relativo máximo de `0,1%` sobre o valor calculado;
- toda entity que contenha qualquer `enemy_archetype_id`, mesmo fora dos objetivos, contém somente parts inimigas de um único archetype; seus bodies são dinâmicos, usam somente a surface desse archetype e sua massa agregada é `volume·density`, com erro relativo máximo de `0,1%` sobre o valor calculado.

## Proxies físicos de produção

O manifesto `first_orbit` mantém as densidades autorais e dimensiona os proxies
que participam do campo gravitacional para o limite de `150 kg`:

| Material | Dimensões completas (m) | Densidade (kg/m³) | Massa (kg) |
|---|---:|---:|---:|
| Tijolo | `0.30 × 0.80 × 0.32` | `1800` | `138.240` |
| Vidro | `0.04 × 0.70 × 0.90` | `2450` | `61.740` |

Os tijolos formam uma grade no plano tangencial, com centros
`X={-0.30,0,0.30}` (o corpo 18 usa `X=0.299`), `Y=13.09` e
`Z={0.43,0.75,1.07}`. O AABB destrutível completo permanece dentro de
`4.44 × 3.19 × 3.20 m`, portanto nenhum eixo ultrapassa o envelope de design
de `4.5 m`. O Javali-Âncora repousa em `[-0.28,10.95,0.75]`, rotacionado em
`90°` ao redor de `+Z`. Os `visual.bounds_m` continuam exatamente iguais ao
tamanho completo dos respectivos proxies.

A calibração de produção usa tenacidade `0.01` para o vidro, limiar de dano
`2.5 J/kg` para o Âncora e juntas `pine_fit=7000 N/1200 N·m`,
`glass_clamp=3000 N/500 N·m`, sete argamassas de suporte em
`1400 N/160 N·m` e uma junta de argamassa sacrificial em `950 N/160 N·m`.
