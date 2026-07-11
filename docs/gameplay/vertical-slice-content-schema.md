# Contrato de conteúdo do vertical slice (schema v1)

Os três documentos JSON são carregados separadamente e só formam um `ContentBundle` depois da validação cruzada. A carga é atômica. Todo objeto rejeita chaves desconhecidas, todo campo listado é obrigatório e nenhum valor ausente recebe default. Uma ausência intencional é representada por `null` explícito.

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
