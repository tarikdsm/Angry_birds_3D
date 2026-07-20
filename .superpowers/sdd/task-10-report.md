# Task 10 — Múltiplos projéteis e dispatcher de habilidades

## Status

Implementação concluída no branch `codex/game-2-0`, sem push. A entrega usa o
commit `refactor(habilidades): adicionar dispatcher e grupos de projéteis`.

O tick da simulação deixou de conhecer a Virela nominalmente. Um dispatcher
tipado agora seleciona explicitamente o par payload/runtime, executa o lifecycle
comum e encaminha os hooks da habilidade concreta. O mesmo `ShotState` mantém um
grupo ordenado e não vazio de projéteis até todos deixarem de ser relevantes.

## TDD

### RED

- O primeiro build falhou pela ausência de `ability_system.cpp`, confirmando que
  o novo alvo CMake exercitava a unidade ainda inexistente.
- Com um dispatcher mínimo, o build falhou pelas APIs de facade necessárias aos
  cenários de grupo de projéteis.
- O primeiro `ability_dispatch` compilado expôs cinco falhas comportamentais:
  payload incompatível era aceito na carga; reconfigure incompatível não era
  atômico; finalizar o primário liberava o shot com filho relevante; remover um
  filho pulava a atualização dos demais; watchdog encerrava uma habilidade ativa
  com término limitado.

### GREEN

- `AbilitySystem` valida `AbilityKind`, tag textual e alternativa de payload na
  carga v2, antes da criação de física. Kind desconhecido e payload incompatível
  retornam erro de conteúdo em `/abilities/<n>/kind` ou `/payload`.
- O dispatch usa `switch` explícito e `holds_alternative`/`get`; não depende de
  `variant::index()`. Somente os hooks do payload/runtime correspondente são
  chamados.
- Ativação, janela start/end e consumo único pertencem ao lifecycle comum.
  Restart limpa o shot e reconfigure inválido preserva o estado canônico byte a
  byte.
- O FSM percorre uma cópia ordenada do grupo, atualiza cada membro uma vez e só
  remove os finalizados após a iteração. A próxima ave não é liberada enquanto
  qualquer filho permanece relevante.
- Watchdog v2 usa `level.watchdog_ticks` e aguarda runtime ativo com `end_tick`
  válido. O fallback v1 de 1.800 ticks e o tratamento fail-safe de runtime ativo
  malformado foram preservados.

## Dispatcher e lifecycle

- Ordem do tick: comandos/FSM, `apply_ability_before_step`, step físico,
  `process_ability_after_step`, dano/fratura/objetivo,
  `finish_ability_after_step`, remoções confirmadas e FSM pós-step.
- Os hooks são tipados para GravityField, MassBoost, SpeedBoost, Explosion e
  Split. As quatro habilidades futuras permanecem sem efeito físico nesta task;
  seus pontos de extensão estão prontos para as Tasks 11–14.
- Falha de hook é latched como `Faulted`, sem ser convertida em derrota.
- A Virela recebe o `GravityFieldAbilityDefinition` tipado, conservando seleção
  canônica, smoothstep, força, pulso e ordem dos eventos. Todos os handles dos
  candidatos são pré-validados antes da primeira força/impulso, evitando mutação
  parcial em caso de estado físico inválido.

## Grupos de projéteis

- Membership permanece ordenada por `EntityId`, sem duplicatas e nunca vazia,
  conforme o contrato de `ShotState` introduzido na Task 8.
- Ages, repouso, ejection, contato e lifetime são avaliados para cada membro sem
  invalidar a iteração.
- Finalizados são destruídos e retirados da membership apenas quando a ability
  não os possui mais. Quando todos terminam, um membro terminal é retido para
  preservar a invariável não vazia até a limpeza do shot.
- O primeiro impacto canônico permanece estável mesmo com vários projéteis.
- Objetivo completo, watchdog e transição para Resolution/Evaluation operam no
  grupo inteiro.

## Canonical e eventos

`canonical_state.cpp` e `events.hpp` já continham, desde a Task 8, os contratos
necessários: lista ordenada de projéteis, runtime com tags explícitas append-only,
payload com tag explícita e eventos de ability com numeração estável. A Task 10
consome esses contratos sem alterar bytes ou renumerar enums; não foi criado diff
artificial nesses arquivos. Os goldens v1 permaneceram intactos.

## Verificação

- `tools/build.ps1 -Configuration Debug`: PASS.
- CTest oficial focado
  `ability_dispatch|gravity_field_ability|vertical_slice_determinism|legacy_orbital`:
  4/4 PASS após a revisão final.
- Filtros adicionais `launch fsm`, `shot state`, `session bootstrap`,
  `session restart`, `session reconfigure`, `simulation contracts` e
  `product v2 content`: PASS.
- Binário completo `ninho_simulation_tests.exe`: 180/180 PASS, exit 0.
- Virela preservou ticks, eventos e hashes dos contratos de
  `vertical_slice_determinism` e `legacy_orbital_characterization`.
- `git diff --check`: exit 0; somente o aviso de conversão LF/CRLF do CMake foi
  emitido pelo Git.

## Escopo preservado

MassBoost, SpeedBoost, Split e Explosion permanecem sem implementação física;
materiais, scoring, UI e conteúdo final continuam reservados às tasks seguintes.
Nenhum golden, threshold, evidence ou fixture Godot/v1 foi recapturado ou
alterado. Nenhum push foi executado.

## Concerns

Nenhum finding aberto da Task 10.
