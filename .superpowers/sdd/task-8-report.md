# Task 8 implementation report

## Outcome

Task 8 estabeleceu a evidencia normativa da fundacao Box3D, integrou os gates C++/upstream/Godot e fechou as revisoes de telemetria, determinismo, semantica e seguranca de filesystem. O estado documentado inclui os commits ate `2d90a52` e a sincronizacao final cujo commit usa a mensagem `fix: sincronizar relatorio e validacao normativa`.

Commits da entrega:

- `d0b3b76` — `docs: registrar gate da fundacao Box3D`.
- `3304f3e` — `fix: vincular telemetria e evidencias da fundacao`.
- `2d90a52` — `fix: fechar auditoria das evidencias Box3D`.
- Sincronizacao final: `fix: sincronizar relatorio e validacao normativa` (o hash e reportado no encerramento, pois este proprio arquivo integra o commit).

## Tracked evidence

- Debug: `docs/physics/evidence/foundation-report-debug.json`
  - SHA-256: `FD87A8BE9B882CD7BB0F58BE2311196456C8C76943C1B347D1B0F534A35385C0`
  - matrix hash: `17104053157009575930`
  - matrix topology: `123/123/1;121/221` (`peak body/shape/joint;awake/contact`)
- Release: `docs/physics/evidence/foundation-report-release.json`
  - SHA-256: `BB517B2550E4A53EAF68723D4E78D4A15BB7FF231F1206C9F72E6358AA9BF5B2`
  - matrix hash: `17104053157009575930`
  - matrix topology: `123/123/1;121/221`

Os hashes canonicos de matriz e fixture sao saidas pinadas de replay para seed `1` e dependencias fixadas. O gate os compara exatamente, mas nao alega rederiva-los criptograficamente do JSON; contratos derivaveis de topologia, repeticao, memoria, allocator, CRT, status e limites sao recalculados e validados separadamente.

## Implementation summary

### Foundation and upstream gates

- `tools/test.ps1 -IncludeUpstream` executa o projeto e o upstream Box3D limpo, exige o commit pinado e valida integralmente `compile_commands.json`.
- Debug exige `cl.exe`, `/MTd`, `/Od` e rejeita `/MD[d]`, `/O1`, `/O2`, `/Ox`, `/GL`, `/DNDEBUG`, `/fp:fast` e contaminacao Release.
- Release exige `cl.exe`, `/MT`, `/O2`, `/DNDEBUG` e rejeita `/MD[d]`, `/Od`, `/RTC`, `/fp:fast` e contaminacao Debug.
- Todos os alvos de remocao recursiva validam o caminho absoluto e cada ancestral existente contra reparse points antes de apagar.
- O gate de evidencia exige os caminhos rastreados exatos, verifica SHA-256 e chama o mesmo validador semantico integral usado pelo gate de relatorio fresco.

### Physics evidence

- `ScenarioResult`, cada `RepeatObservation` e cada `CapabilityRow` registram picos de bodies, shapes, joints, awake e contacts.
- Repeats com hash igual e topologia divergente falham; todos os 12 pares atuais repetem hash e os cinco picos.
- A matriz agrega o maximo das oito provas: `123/123/1`, awake `121`, contacts `221`.
- As oito capacidades possuem status funcional `pass`, nenhum fallback e hashes de fixture pinados.
- Process allocator e todas as observacoes Box3 retornam exatamente a zero. Os quatro repeats de stress tem arrays post-teardown `10+10` integralmente em zero.
- CRT Debug nos dois repeats stress e aplicavel/balanceado, com os quatro deltas zero. CRT Release e nao aplicavel/balanceado, tambem com deltas zero.
- PrivateUsage e Working Set permanecem diagnosticos: `gate_applied=false`, `budget_qualified=false`, warning `private_commit_budget_unqualified`, qualificacao `deferred` para hardware de referencia.

### Report and machine-readable binding

`docs/physics/box3d-spike-report.md` foi reconstruido a partir dos snapshots atuais, incluindo:

- SHA-256, matrix hashes, topologia agregada e recomendacoes em tokens machine-readable para Debug e Release;
- hashes, topologias e todos os tempos `min/p50/p95/max` dos seis cenarios em ambos os builds;
- oito capacidades, seus picos, medidas e fixture hashes;
- quatro avaliacoes completas de footprint, com todas as series brutas PrivateUsage/Working Set `10+10` e todos os derivados;
- allocator, CRT, limites conhecidos e recomendacao normativa;
- caminhos exatos dos logs C++/upstream, Godot headless e renderers, mantendo cada fonte de evidencia separada.

## TDD and review evidence

- O primeiro RED registrou relatorio ausente e parametro `-IncludeUpstream` ausente.
- REDs posteriores cobriram topologia sem campos de pico, freshness ausente, gate hermetico ausente, binding de evidencia ausente e mutacoes semanticas com SHA recalculado.
- A revisao encontrou e fechou o parsing de array JSON no PowerShell 5, a distincao fixture/pico, a topologia por repeat, o agregado completo de capacidades, a validacao integral compartilhada, junctions ancestrais e contaminacao do banco de compilacao.
- Testes de mutacao cobrem hashes, topologia, memoria/derived values, allocator, CRT, configuracao, status/fallback, budget e pins.

## Verification evidence

Gates finais desta sincronizacao:

```text
artifacts/physics/upstream-box3d-debug.log
  20/20, 9.70 s
artifacts/physics/upstream-box3d-release.log
  20/20, 0.92 s
build/debug/Testing/Temporary/LastTest.log
  24/24; total CTest 206.67 s
build/release/Testing/Temporary/LastTest.log
  24/24; total CTest 15.34 s
```

Godot headless usa `artifacts/physics/godot-smoke-{debug,release}.stdout.log` e `.stderr.log`; ambos exit `0` e stderr vazio. Vulkan usa `godot-scene-{debug,release}.{stdout,stderr}.log`; OpenGL usa `godot-scene-gl-{debug,release}.{stdout,stderr}.log`. Os quatro renderers gravaram 15 frames sem erro.

Verificacoes complementares concluidas na base `2d90a52`:

- `run_spike.ps1` Debug e Release: relatorios frescos, zero violacao normativa e recomendacao exata.
- Upstream Box3D: 20/20 em ambos os builds.
- Projeto: 24/24 em ambos os builds.
- Godot headless, Vulkan Forward Mobile e OpenGL Compatibility: exit `0`.
- SPDX 2.3: valido.
- Revisao independente final da base: nenhum Critical, Important ou Minor aberto.

## Recommendation

`prosseguir_com_limites`

Nao ha violacoes normativas. Box3D 3D `v0.1.0` permanece alfa, e o budget de crescimento de 5% continua diferido para Godot Release empacotado em hardware de referencia. O handoff deve ser atualizado com o hash do commit final de sincronizacao quando o controlador criar esse commit.
