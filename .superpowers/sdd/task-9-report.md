# Task 9 — GameplaySessionNode e compatibilidade orbital v1

## Status

Implementação concluída no branch `codex/game-2-0`, sem push. O commit desta
entrega usa a mensagem exata
`feat(gdextension): expor sessão genérica de gameplay`.

`GameplaySessionNode` passa a expor a sessão genérica v2 em paralelo com
`OrbitalSessionNode`. O adapter legado continua usando exclusivamente o caminho
de anel v1 e seu frame canônico de 15 chaves permanece inalterado.

## TDD

### RED

- O contrato de registro falhou inicialmente porque
  `GDREGISTER_CLASS(GameplaySessionNode)` ainda não existia.
- O primeiro build do adapter v2 falhou pela ausência do header e da classe
  `GameplaySessionAdapter`.
- A extração do frame batch falhou em compilação enquanto o serviço comum e
  `clear_preview()` ainda não existiam.
- Os testes tipados do frame v2 falharam enquanto fila, ave atual, plano travado,
  disparo, projéteis, score/estrelas e gravidade ainda não eram publicados.
- O contrato de chaves encontrou a função de frame v2 ausente antes da nova
  serialização Godot.
- O registry test falhou com
  `Registry must contain every shipped game/tests entrypoint` antes do novo
  smoke entrar no registro central.
- O primeiro smoke observou o frame antes do processamento do node nativo e
  falhou com `grabbed frame did not publish locked plane`; o teste passou a
  respeitar a ordem real do physics process.
- Um teste adversarial de catch-up revelou que `shot` se perdia quando grab,
  pull e release eram consumidos no mesmo lote de três ticks. O adapter agora
  preserva a ave do release até o projétil autoritativo aparecer.

### GREEN

- O adapter v2 aceita somente `BeginGrabCommand`, `SetPullCommand`,
  `ReleaseBirdCommand`, activate e cancel grab; pull não finito retorna `false`
  sem fault.
- Configure e restart constroem sessão e frame candidatos antes do commit,
  preservando a última configuração válida em falhas.
- O frame batch mantém eventos de todos os ticks e somente o estado mais novo;
  acknowledge limpa eventos, contagem e tempo descartado sem perder snapshots.
- O smoke configura um mundo uniforme mínimo, congela as 23 chaves v2, exercita
  grab/pull/release, preview, plano, projétil, acknowledge e restart.

## Contratos implementados

- Métodos Godot v2: `configure_session`, `queue_begin_grab`, `queue_pull`,
  `queue_release`, `queue_activate_ability`, `queue_cancel_grab`,
  `restart_level` e `consume_frame`.
- Todas as entradas ABI são `noexcept`; conversões, criação de Dictionary,
  emissão de signal e chamadas de sessão ficam protegidas por fault boundaries.
  `consume_frame` só reconhece o lote depois de construir o Dictionary inteiro.
- O node usa prioridade de processo `-100` e emite `gameplay_fault(code,
  message)` uma vez por geração de fault.
- Frame schema 2 com exatamente 23 chaves: versão, tick/fase/outcome,
  launcher/plano, fila/ave atual, shot/projéteis, snapshots/eventos/objetivos,
  readiness, preview, score/estrelas, gravidade e métricas/tempo descartado.
- A gravidade local é consultada em `physics::GravityField` na posição de
  repouso do estilingue, sem duplicar a fórmula uniforme ou radial.
- Fila autoral preserva ordem e duplicatas; `shot` conserva a ave lançada e IDs
  dos projéteis autoritativos mesmo sob catch-up.
- Score e estrelas permanecem reservados como zero nesta task, conforme o estado
  canônico v3; a lógica de scoring continua reservada para a Task 16.
- Accumulator, fault handling e frame batching foram extraídos para
  `session_adapter_services` e compartilhados sem alterar o Dictionary v1.
- As listas CMake continuam explícitas; nenhum `file(GLOB)` novo foi adicionado.

## Compatibilidade v1

- `OrbitalSessionAdapter` continua chamando `BeginAimCommand`, `SetAimCommand` e
  `LaunchCommand` com origem, tangente e velocidade arbitrárias.
- O fonte orbital não referencia grab, pull, release, `camera_right`, massa,
  `SlingshotDefinition`, energia de mola ou tradução por Hooke.
- `OrbitalSessionNode` continua registrado junto com o node genérico.
- O contrato legado continua congelando as mesmas 15 chaves superiores do
  Dictionary; nenhum golden/canonical v1 foi modificado.

## Verificação

- `tools/build.ps1 -Configuration Debug -WithGodot`: PASS, incluindo a DLL
  GDExtension.
- CTest oficial focado
  `gameplay_session|gdextension_adapter|gdextension_vertical_slice`: 6/6 PASS,
  incluindo import, smoke orbital v1 e smoke gameplay v2.
- Binário completo `ninho_extension_adapter_tests.exe`: 29/29 PASS.
- Binário `ninho_session_frame_batch_tests.exe`: 6/6 PASS.
- `tools/test.ps1 -Configuration Debug`: todos os contratos executados antes do
  agregado passaram; o comando encerrou somente com o stale esperado
  `tested inputs aggregate SHA-256 mismatch` em
  `tools/TestedInputIdentity.psm1:133`. Evidence/thresholds da fundação não
  foram recapturados ou publicados.
- `git diff --check`: exit 0.
- O Godot não deixou alterações `.import` no worktree.

## Escopo preservado

Dispatcher das habilidades concretas, scoring, save/progressão, conteúdo final,
UI e recaptura de foundation permanecem para as tasks posteriores. Nenhum push
foi executado.

## Concerns

Nenhum finding aberto da Task 9. `score` e `stars` são campos ABI estáveis já
publicados, mas continuam deliberadamente zerados até a Task 16.
