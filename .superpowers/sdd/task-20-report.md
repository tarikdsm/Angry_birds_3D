# Task 20 — Save e opções recuperáveis

## Resultado

A aplicação agora mantém `progress.v2` e `settings.v2` em JSON canônico,
validado e recuperável. `SaveStore` publica por arquivo temporário, `flush`,
backup e promoção; restaura o backup quando o principal é inválido e preserva
ambos os documentos corrompidos com nomes únicos antes de usar defaults. O
`AppShell` recebe um `storage_root` injetável antes de entrar na árvore, aplica
as opções somente após validação integral e mostra aviso pt-BR não bloqueante
quando ocorre recuperação total.

`ProgressModel` usa schema v2 e IDs oficiais do catálogo: Terra e Orbital
começam disponíveis, desbloqueios posteriores são sequenciais, vitória exige
1–3 estrelas e incrementa `completion_count` explicitamente uma vez por
registro; score, estrelas e aves restantes só melhoram. Derrota não grava
recorde, outcomes desconhecidos e estados impossíveis são rejeitados.

`SettingsModel` é fechado, ordena os bindings oficiais e exige tokens físicos
canônicos. `BindingStore` remapeia por contexto de forma transacional, pede
confirmação para conflito, permite troca/cancelamento sem deixar intenção sem
binding e faz rollback integral em erro. `InputRouter` continua sendo o único
tradutor de input físico. Escala de UI 100/125/150/200 usa telas roláveis com
foco visível; sensibilidade atual aparece localizada e persiste em passos de
0,05. Os consumidores de motion/shake/trajectory e dos buses de áudio seguem
deliberadamente para as Tasks 24 e 29, conforme o plano.

## TDD e correções de revisão

Os três REDs iniciais falharam nos preloads ainda inexistentes de
`ProgressModel`, `SettingsModel`, `SaveStore` e `BindingStore`. A implementação
mínima levou os markers `SAVE_RECOVERY_SMOKE_OK`, `SETTINGS_MODEL_SMOKE_OK` e
`INPUT_REMAPPING_SMOKE_OK` a GREEN e registrou os três smokes no contrato.

As ondas seguintes adicionaram REDs causais para:

- aliases de tecla (`key:32`/`key:032`) e ordem arbitrária dos bindings;
- progresso impossível, unlock não sequencial, vitória com zero estrelas e
  outcome desconhecido;
- colisão de nomes `.corrupt` entre instâncias novas no mesmo segundo;
- rollback/cancelamento de conflito e restauração atômica dos defaults;
- catálogo com placeholder ausente/desconhecido;
- clipping/foco fora da viewport em UI scale 200%;
- sensibilidade persistida sem valor atual visível;
- frame/hash da simulação alterado por save/options.

O último caso configura um `OrbitalSessionNode` real, captura bytes e hash do
frame, executa save, recovery e escrita interrompida, e exige os mesmos bytes e
hash ao final. Nenhuma fonte em `native/simulation` foi alterada.

Dois REDs de tooling apareceram durante os gates e também foram corrigidos:

- a primeira recaptura Release publicou `build_type=Debug` porque o cache
  `build/release` ainda carregava `CMAKE_BUILD_TYPE=Debug` após a troca de
  compilador; uma segunda build oficial Release estabilizou o cache antes da
  promoção;
- o teste da evidência mutava `assessment_status` para `pass`, que já era o
  valor corrente e portanto não exercia o gate. A mutação agora alterna para
  um valor necessariamente diferente; o RED reproduziu e o teste focado
  voltou a PASS.

## Evidência e verificação

- Smokes focados Godot: `SAVE_RECOVERY_SMOKE_OK`,
  `SETTINGS_MODEL_SMOKE_OK`, `INPUT_REMAPPING_SMOKE_OK`,
  `APP_SHELL_SMOKE_OK` e `INPUT_ROUTER_SMOKE_OK`, todos exit 0.
- Import completeness: `NINHO_IMPORT_COMPLETENESS_OK resources=33
  entrypoints=5 documents=13`.
- Registry contract: PASS com os três novos markers registrados.
- Foundation evidence Debug e Release recapturada pelos comandos oficiais;
  ambos os relatórios usam
  `tested_inputs_sha256=06d2a6cdbedf1be8fbf2ba3bba1828333e8677b5d9ef726786c2080e2268216f`.
- `python tools/generate_foundation_report.py --root . --check`: PASS.
- `tools/tests/foundation-evidence-gate-tests.ps1`: PASS após a mutação causal.
- `tools/test.ps1 -Configuration Debug`: PASS, exit 0. CTest 69/69, zero
  falhas, total 8.947,96 s; `product_v2_determinism` PASS em 8.143,40 s.
  Todos os smokes registrados passaram e as capturas Vulkan/OpenGL chegaram ao
  frame 300 e foram validadas.
- `git diff --check`: PASS antes da entrega, com apenas avisos esperados de
  normalização CRLF/LF nos relatórios gerados e scripts PowerShell.

## Revisão e escopo operacional

A revisão independente após as correções retornou **Ready to merge: YES**, com
0 Critical, 0 Important e 0 Minor. Uma confirmação final foi solicitada após o
gate completo e a atualização deste relatório.

Nenhum `*.import` transitório entra no change set. Os oito `*.gd.uid` novos são
sidecars legítimos e únicos. Build, vídeos e cache `.godot` permanecem fora do
stage. A Task20 não altera fixtures, frame canônico nem hash da simulação.
