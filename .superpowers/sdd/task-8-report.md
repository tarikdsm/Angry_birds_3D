# Task 8 — ShotState e canonical_state_v3

## Status

Implementação concluída no branch `codex/game-2-0`, sobre a base `0eb9c6d`,
sem push. O commit desta entrega usa a mensagem exata
`refactor(simulacao): criar estado de disparo e canonical v3`.

O caminho schema v1 continua publicando somente `canonical_state_v2`, com os
bytes e goldens anteriores preservados. O caminho schema v2 publica somente o
novo `canonical_state_v3`; o contrato inativo permanece vazio e com hash zero.

## Correções após revisão formal

- `ProjectileState::entity_id` deixou de ser mutável externamente e
  `ShotState` encapsula sua coleção. Inserção por `lower_bound` mantém ordem
  estritamente crescente, duplicatas são rejeitadas sem mutação, replace
  preserva identidade e erase não permite coleção vazia. O primário é sempre o
  menor EntityId.
- Copy/move assignment de ProjectileState preserva o EntityId do slot e copia
  somente os campos runtime. Insert/erase usam rebuild+swap, fechando o bypass
  que ainda permitiria trocar a identidade através do ponteiro mutável do
  primário ou do move-assignment interno do vector.
- ShotState é copy/move-constructible, porém não assignable; a sessão usa
  `optional.emplace` ao publicar um disparo. Static asserts impedem que uma
  futura atribuição de vector reabra o bypass de identidade.
- O serializer v2/v3 valida ShotState não vazio, único e ordenado e itera a
  coleção normalizada diretamente. Não ordena cópia, não aceita empate e não
  pode publicar o mesmo hash para primários diferentes.
- `complete_release` limpa o LauncherState público somente depois que plano e
  pull aceitos foram copiados para ShotState. Cancel/deadzone, retorno a
  Inspection, restart e reconfigure também deixam o launcher neutro e sem ghost
  residual.
- RED de invariantes: o build falhou pelos métodos encapsulados ainda ausentes;
  depois, ordem de inserção, primário mínimo, replace/erase e duplicata atômica
  ficaram verdes.
- RED de lifecycle: três testes falharam exatamente porque LauncherState ainda
  existia após release. Após a correção, release, settle/Inspection,
  restart/reconfigure e o isolamento canônico ShotState-versus-launcher ficaram
  verdes.
- RED defensivo adicional: assignment no primário trocou o EntityId e quebrou a
  ordem; o teste agora congela a identidade e permaneceu verde após tornar o
  assignment identity-preserving.
- RED de construção adicional: static asserts mostraram ShotState ainda
  assignable; o contrato agora bloqueia copy/move assignment e mantém somente
  construção/cópia/movimento seguros.

## TDD

### RED

- O primeiro build de `shot_state_tests.cpp` falhou porque
  `ability_runtime.hpp`/`ShotState` ainda não existiam.
- As verificações de tags falharam em compilação enquanto
  `detail::canonical_tag_of` não estava definido.
- Os primeiros testes v3 observaram bytes/hash vazios, não distinguiram a ordem
  autoral da fila e não distinguiram valores acima do quantum.
- Os testes adversariais de `exited_world`, múltiplos projéteis e runtime
  falharam em compilação antes das facades de teste correspondentes.
- Os primeiros goldens foram capturados somente após os contratos estruturais
  ficarem verdes: mínimo v3 `12470858622598513254` e seis estados v2 de comandos
  pendentes/fila.

### GREEN

- `ShotState` passou a ser a unidade proprietária do disparo: ID, ave,
  habilidade, tick, plano travado, pull aceito, consumo da ativação, runtime e
  projéteis ordenáveis.
- `AbilityRuntime` é uma variante append-only com tags explícitas e alternativas
  para gravity field, mass boost, speed boost, explosion e split.
- O lifecycle v1 foi migrado pelo adaptador de projétil primário sem alterar os
  eventos, fases, fórmulas ou bytes v2.
- O serializer v3 passou nos testes de ordem de fila, camera/pull, runtime,
  `exited_world`, ordenação dos projéteis, quantização, zero negativo,
  repetibilidade e falha atômica.

## Contratos implementados

- Todos os enums de domínio/ABI da simulação têm representação `uint8_t` e
  valores numéricos explícitos. Novos valores permanecem append-only.
- Serializers usam funções de tag explícitas; `variant.index()` não aparece em
  `canonical_state.cpp`.
- `canonical_state_v2` continua com o mesmo layout. O adaptador de `ShotState`
  escreve exatamente o antigo bloco de projétil e os goldens legados continuam
  verdes, inclusive comandos pendentes e ordem da deque.
- `canonical_state_v3` começa por nome e versão 3, usa little-endian, IDs com
  largura fixa, coleções com cardinalidade `u32` e reais em `i64` a `1e-5`.
- Conteúdo imutável, gravity/bounds tipados, launcher, fila autoral, snapshots,
  eventos, dano, pendências, `ShotState`, runtime e contadores relevantes entram
  no stream. Score/estrelas estão reservados como zero.
- Coleções sem ordem causal são serializadas por identidade de domínio; fila,
  eventos, hulls e filhos compound preservam a ordem contratada.
- `BodyHandle`, endereços, métricas/timings, apresentação e save são excluídos.
- Hash v3 é FNV-1a 64. O cache ativo só é substituído depois que bytes e hash
  completos são produzidos; erro numérico conserva a publicação anterior.
- Enqueue, tick, restart e reconfigure atualizam o cache correto; reconfigure
  inválido e falha de serialização não publicam estado parcial.

O layout completo, incluindo tags, ordem dos blocos, ordenação, quantização e
exclusões, está registrado em `docs/gameplay/product-v2-content-schema.md`.

## Goldens congelados

- v3 mínimo: `12470858622598513254`.
- v2 BeginAim pendente: `5744874256725084803`.
- v2 SetAim pendente: `515178963462208224`.
- v2 Launch pendente: `3869036764581120913`.
- v2 ActivateAbility pendente: `8268707775012186916`.
- v2 CancelAim pendente: `15779464696783206375`.
- v2 fila completa na ordem: `11212476156426193241`.
- Golden principal v2 legado preservado: `16778877272428821006`.

## Verificação

- `tools/build.ps1 -Configuration Debug`: PASS, sem warnings do compilador.
- CTest oficial
  `shot_state|canonical|launcher_system|legacy_orbital`: 4/4 PASS, 0 falhas.
- Binário completo
  `build/debug/native/simulation/ninho_simulation_tests.exe`: exit 0;
  170 testes registrados, todos PASS.
- `git diff --check`: exit 0.
- Revisão do diff confirmou listas CMake explícitas, `add_test(NAME shot_state)`,
  ausência de `file(GLOB)` novo e isolamento entre caches v2/v3.

## Escopo preservado

Godot/GDExtension, dispatcher genérico, implementação das quatro habilidades
novas, conteúdo final, evidence, thresholds e recaptura de foundation ficaram
fora desta task. Nenhum push foi executado.

## Concerns

Nenhum finding aberto da Task 8. O dispatcher e o lifecycle de múltiplos
projéteis permanecem deliberadamente para a Task 10; esta task fornece o estado,
as tags e o contrato canônico necessários sem antecipar esse comportamento.
