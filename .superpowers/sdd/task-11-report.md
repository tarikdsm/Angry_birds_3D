# Task 11 — Aumento de massa da ave vermelha

## Status

Implementação concluída no branch `codex/game-2-0`, sem push. O commit desta
task usa `feat(habilidades): adicionar aumento de massa da ave vermelha`.

A habilidade `MassBoost` agora é concreta: ela arma exatamente nove ticks após
o lançamento, aceita uma única ativação e multiplica massa e inércia da ave por
2,25 sem alterar velocidade, geometria, identidade, visual, gravidade ou estado
de repouso. A massa permanece aumentada durante o restante da vida do projétil e
só desaparece quando o corpo é removido.

## Proveniência e TDD

O snapshot inicial desta task continha somente a inscrição do teste no CMake e
o arquivo de teste ainda não rastreado. O primeiro build RED falhou porque
`PhysicsWorld` não oferecia as consultas físicas nem a operação de escala de
massa exigidas pelo contrato. Durante a execução, um buffer de produção e testes
da própria task foi recuperado/materializado no worktree compartilhado. Ele foi
preservado, auditado contra o brief e completado; não foi tratado como evidência
GREEN automática.

### RED

- O primeiro build não compilou por ausência de `local_inertia`, `local_center`,
  `shape_keys`, `shape_densities` e `set_body_mass_scale` no boundary físico.
- Depois da primeira implementação física, o teste compilado ainda falhou por
  ausência de `DomainEventKind::MassChanged`.
- Com física e tag presentes, os casos físicos passaram, mas os três cenários de
  sessão continuaram RED: `SimulationSession::create()` recusava `MassBoost`
  porque a Task 10 mantinha todas as habilidades futuras fail-closed.
- O novo teste de adapter Godot falhou RED até `MassChanged` ser mapeado
  explicitamente para `mass_changed` nos dois nodes.

### GREEN

- A física e os caches foram implementados primeiro; depois, o evento e o
  sistema concreto de habilidade foram ligados ao dispatcher.
- O arming fixo de nove ticks foi centralizado em `AbilitySystem`, de modo que
  não depende do valor autorado em `arm_ticks`.
- Os testes cobrem o boundary físico isolado, o dispatcher, o lifecycle completo
  da sessão, canonical/eventos, restart/reuso e adapters Godot.

## Boundary físico

`PhysicsWorld::set_body_mass_scale(BodyHandle, float)` aceita somente handles
dinâmicos vivos e fatores positivos e finitos. Cada slot conserva as densidades
base imutáveis das shapes, o `b3MassData` base e a escala vigente.

A operação:

1. pré-valida o fator e todas as densidades antes de qualquer mutação;
2. aplica densidades absolutas com `b3Shape_SetDensity(..., false)`;
3. recompõe massa, centro e inércia uma única vez com
   `b3Body_ApplyMassFromShapes`;
4. restaura velocidade linear, velocidade angular e estado awake;
5. valida massa e inércia resultantes; em erro, restaura densidades e mass data
   anteriores.

A escala é absoluta, não cumulativa. Reservation rollback, remoção e reuso do
slot limpam shapes, densidades base, mass data e escala. Um seam restrito à
facade de teste injeta overflow tardio em compound shapes para provar que a
pré-validação impede mutação parcial.

## Lifecycle da habilidade

- `MassBoost` é o único kind futuro promovido a sistema concreto nesta task;
  `SpeedBoost`, `Explosion` e `Split` continuam fail-closed.
- O parser deriva `arm_ticks = 9`, preserva a duração autorada e não acrescenta
  uma chave nova ao payload JSON canônico. O cenário end-to-end desta task usa
  uma janela de 75 ticks.
- Offsets `L+0` a `L+8` geram exatamente nove `CommandRejected` com razão
  `NotArmed`.
- Em `L+9`, a ativação gera uma única sequência `AbilityStarted` e
  `MassChanged`, com `weight = 2.25` e a entidade primária.
- Uma segunda ativação é recusada, sem novo `MassChanged`.
- No fim da janela é publicado exatamente um `AbilityEnded`; a massa aumentada
  não é revertida. Ela deixa de existir somente com a remoção física da ave.
- O evento `MassChanged` recebeu a tag canônica append-only 13; as tags 0–12 não
  foram renumeradas.
- Os dois adapters Godot expõem o evento como `mass_changed`; enums desconhecidos
  continuam fail-closed em `unknown`.

## Invariantes físicos verificados

Para a ave de teste, massa e inércia passam de 6 para 13,5 (2,25×). A comparação
com uma sessão controle confirma, dentro de `1e-5`, que a ativação não produz
salto de posição nem altera velocidade linear/angular. Nos ticks seguintes, a
trajetória continua submetida à mesma gravidade; shape, bounds, visual id,
entity id e part id permanecem idênticos.

Também foram exercitados handles pendentes, estáticos e inválidos; fatores zero,
negativo, NaN e infinito; overflow em compound shape; corpo dormindo; escalas
absolutas sucessivas; vinte ciclos de remoção/reuso de slot; vinte restarts de
sessão; e reconfigure. Os caches retornam ao baseline e a nova geração não herda
massa ou shapes do corpo anterior.

## Verificação

- `tools/build.ps1 -Configuration Debug`: PASS.
- CTest oficial focado `(^world$|mass_boost_ability|ability_dispatch)`: 3/3
  PASS.
- Regressões `shot_state|product_v2_content|session_restart|` +
  `legacy_orbital_characterization|vertical_slice_determinism|determinism|` +
  `canonical_hash`: 7/7 PASS.
- Binário completo `ninho_simulation_tests.exe`: PASS, exit 0, em 92,6 s.
- Build Debug com `NINHO_BUILD_GDEXTENSION=ON`: PASS.
- Binário `ninho_extension_adapter_tests.exe --filter gameplay`: 10/10 PASS.
- `git diff --check`: exit 0.

O teste de extension `gameplay frame follows authoritative shot lifecycle and
ordered membership` ainda congelava membership histórica da Task 9, enquanto a
Task 10 passou a remover da membership atual cada filho finalizado. O teste foi
alinhado ao contrato atual usando bodies distintos: ao finalizar os IDs `0`,
`2` e `3`, a view publica `[2,3]`, depois `[3]` e por fim retém apenas o membro
terminal `[3]` durante Resolution. Isso prova ordem, remoção do membro correto e
ausência de skip na iteração, sem alterar o fluxo de produção da Task 10.

Durante uma tentativa intermediária, o Ninja reportou `failed recompaction:
Permission denied`. A falha não se repetiu no build Debug normal seguinte, nem
houve lock/processo persistente que justificasse limpeza seletiva. A causa mais
provável é uma disputa transitória de acesso aos metadados `.ninja_deps` ou
`.ninja_log`; o build e todas as verificações posteriores concluíram normalmente.

## Escopo preservado

Nenhum golden, evidence, threshold, fixture v1 ou conteúdo de foundation foi
recapturado ou alterado. Não foram implementadas as habilidades das Tasks 12–14,
nem houve alteração de materiais, scoring ou UI além do nome tipado do novo
evento. O stash está vazio e nenhum push foi executado.

## Concerns

Nenhum finding aberto da Task 11.
