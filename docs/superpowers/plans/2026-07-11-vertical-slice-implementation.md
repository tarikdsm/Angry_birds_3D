# Ninho Orbital — Plano de implementação do Vertical Slice

> **Para Codex:** executar este plano com `superpowers:subagent-driven-development`, uma tarefa por vez, usando implementador e revisor distintos. Aplicar TDD em cada mudança de comportamento e manter os gates da fundação verdes.

**Objetivo:** entregar uma fase Windows desktop jogável, “Primeira Órbita — Contrapeso de Aster”, com lançamento orbital, habilidade Virela, Javali-Âncora, pinho/vidro/tijolo, vitória, derrota e apresentação 3D original.

**Arquitetura:** um `SimulationSession` C++20 headless concentra gameplay determinístico sobre o `PhysicsWorld`; um novo `OrbitalSessionNode` expõe comandos e um pacote batched por frame ao Godot. Godot permanece responsável somente por input, câmera, UI, áudio, VFX e meshes. O `Box3DWorldNode` e o spike da fundação permanecem intactos como regressão.

**Tecnologias:** C++20, Box3D 3D v0.1.0, nlohmann/json v3.11.3 fixado, Godot 4.5.1 GDExtension, GDScript, Blender 5.1.2, CMake/CTest/PowerShell/Python.

**Especificação:** `docs/superpowers/specs/2026-07-11-vertical-slice-design.md`.

---

## Regras de execução

- Trabalhar apenas em `feature/vertical-slice` até todos os gates passarem.
- Antes de cada implementação: escrever o teste vermelho, executar e confirmar a falha correta.
- Após cada implementação: executar o filtro local, a suíte do novo módulo e a regressão proporcional.
- Nunca mover regra de dano, habilidade, objetivo ou resultado para GDScript.
- Nunca criar física Godot para gameplay.
- IDs de domínio são estáveis; handles Box3D nunca atravessam a API pública.
- Aves, habilidades, inimigos, weakpoints e materiais são registros data-driven; nomes de arquétipos nunca viram branches no kernel.
- Carga de conteúdo é estrita e atômica; valores não finitos e chaves desconhecidas falham.
- Eventos são acumulados por tick e consumidos uma única vez por frame.
- Toda dependência, asset e ferramenta nova recebe licença, versão, hash e caminho de reprodução.
- Cada tarefa termina com revisão independente; corrigir todo finding Critical/Important antes do commit.
- Toda tarefa que cria `.cpp` modifica a lista explícita de sources/tests em CMake e registra os `add_test(NAME ...)` citados; `file(GLOB)` é proibido.

## Comandos-base

Executar a partir da raiz do worktree:

```powershell
.\tools\bootstrap.ps1 -InstallPortable
.\tools\bootstrap.ps1 -CheckOnly
.\tools\build.ps1 -Configuration Debug
.\tools\test.ps1 -Configuration Debug
.\tools\build.ps1 -Configuration Release
.\tools\test.ps1 -Configuration Release
```

O runner específico será criado na Tarefa 10:

```powershell
.\tools\test_vertical_slice.ps1 -Configuration Debug
.\tools\test_vertical_slice.ps1 -Configuration Release -IncludeVisualGate
```

---

## Task 1 — Dependência JSON e esqueleto do kernel de simulação

**Arquivos:**

- Modificar: `cmake/Dependencies.cmake`
- Modificar: `CMakeLists.txt`
- Criar: `native/simulation/CMakeLists.txt`
- Criar: `native/simulation/include/ninho/simulation/content.hpp`
- Criar: `native/simulation/include/ninho/simulation/commands.hpp`
- Criar: `native/simulation/include/ninho/simulation/events.hpp`
- Criar: `native/simulation/include/ninho/simulation/session.hpp`
- Criar: `native/simulation/src/content.cpp`
- Criar: `native/simulation/src/session.cpp`
- Criar: `native/simulation/tests/test_framework.hpp`
- Criar: `native/simulation/tests/test_main.cpp`
- Criar: `native/simulation/tests/kernel_contract_tests.cpp`
- Modificar: `THIRD_PARTY_NOTICES.md`
- Modificar: `third_party/sbom.spdx.json`
- Criar: `third_party/nlohmann-json.LICENSE.txt`
- Modificar: `third_party/README.md`

**Passos:**

1. Criar teste de contrato que inclui os quatro headers públicos, valida tipos fortes `EntityId`, `PartId`, `MaterialId`, `JointId`, `TickIndex`, enums de fase/resultado e prova que `SessionState` começa em `Inspection/None`.
2. Executar a configuração/build e registrar a falha porque o target e os headers ainda não existem.
3. Fixar `nlohmann/json` `v3.11.3` no commit `9cca280a4d0ccf0c08f47a99aa71d1b0e52f8d03`, seguindo o mesmo cache/hash/proveniência usado nas dependências existentes.
4. Criar `ninho_simulation_kernel` estático, alias `ninho::simulation`, ligado a `ninho::physics`; habilitar `/fp:precise` e warnings equivalentes ao kernel.
5. Criar `ninho_simulation_tests` com runner que rejeita filtros vazios/desconhecidos e retorna erro quando nenhum teste casa.
6. Registrar `simulation_contracts` no CTest e inserir `native/simulation` entre kernel e extension no CMake raiz.
7. Atualizar NOTICE, licença e SPDX sem alterar versões da fundação.
8. Verificar:

```powershell
.\tools\bootstrap.ps1 -CheckOnly
.\tools\build.ps1 -Configuration Debug
.\tools\Invoke-Native.ps1 -Command "ctest --preset debug -R 'simulation_contracts|radial|world' --output-on-failure"
python -c "import json; json.load(open('third_party/sbom.spdx.json', encoding='utf-8')); print('SPDX JSON OK')"
```

9. Commit: `build: adicionar kernel de simulacao e JSON fixado`.

## Task 2 — Catálogo de materiais e manifesto estritos

**Arquivos:**

- Modificar: `native/simulation/include/ninho/simulation/content.hpp`
- Modificar: `native/simulation/src/content.cpp`
- Criar: `native/simulation/tests/content_tests.cpp`
- Criar: `game/data/materials/vertical_slice.materials.json`
- Criar: `game/data/archetypes/vertical_slice.archetypes.json`
- Criar: `game/data/levels/first_orbit.level.json`
- Criar: `docs/gameplay/vertical-slice-content-schema.md`
- Modificar: `native/simulation/CMakeLists.txt`

**Passos:**

1. Escrever testes vermelhos para os três materiais, arquétipos data-driven, schema v1, IDs explícitos, referências e invariantes geométricos.
2. Cobrir individualmente: chave desconhecida, campo ausente, enum inválido, ID duplicado, material/surface inexistente, número não finito/fora do intervalo, body dinâmico sem densidade, joint órfão, limite de joint ausente/zero/não finito, objetivo sem alvo, assembly vazio e visual inconsistente.
3. Implementar `parse_material_catalog()`, `parse_archetype_catalog()` e `parse_level_manifest()` retornando resultados tipados; validar referências cruzadas no `ContentBundle`, sem lançar pela fronteira pública.
4. Rejeitar recursivamente chaves desconhecidas por objeto; usar `at()` após validação de shape e tipo; não usar defaults silenciosos.
5. Definir pinho `1`, tijolo `5`, vidro `9` e respostas `fibrous/masonry/brittle`; definir surfaces reservadas `1001..1004` para planeta, plataforma, Virela e armadura, sem contá-las nos materiais jogáveis.
6. Definir `BirdArchetype`, `AbilityArchetype`, `EnemyArchetype`, `WeakpointProfile` e `MaterialDefinition`; Virela usa massa física `140 kg`, densidade `366,76 kg/m³`, atrito `0,35`, restituição `0,25`.
7. Definir arco `radial(θ)=(-cosθ,0,sinθ)`, `θ∈[-50°,+50°]`, planeta, anel, três Virelas, plataforma, oito peças de pinho, três vidros, nove tijolos, Âncora e objetivo.
8. Declarar `force_limit_n/torque_limit_nm` obrigatórios: pinho `6500/1000`, vidro `3000/500`, argamassa `4000/700`.
9. Provar round-trip canônico e erro contendo JSON pointer/código, sem ecoar conteúdo inteiro.
10. Verificar:

```powershell
.\tools\Invoke-Native.ps1 -Command 'ctest --preset debug -R content_manifest --output-on-failure'
.\build\debug\native\simulation\ninho_simulation_tests.exe --filter content
```

11. Commit: `feat: definir conteudo estrito da primeira orbita`.

## Task 3 — Bootstrap, mapeamento e reinício determinístico da sessão

**Arquivos:**

- Modificar: `native/simulation/include/ninho/simulation/session.hpp`
- Modificar: `native/simulation/src/session.cpp`
- Criar: `native/simulation/src/session_builder.cpp`
- Criar: `native/simulation/src/canonical_state.cpp`
- Criar: `native/simulation/tests/session_tests.cpp`
- Modificar: `native/simulation/CMakeLists.txt`
- Modificar: `native/kernel/include/ninho/physics/physics_world.hpp`
- Modificar: `native/kernel/src/physics_world.cpp`

**Passos:**

1. Testar `SimulationSession::create()` com os arquivos reais e snapshots ordenados por `(EntityId, PartId)`.
2. Testar contagem inicial de corpos/juntas, massa do Âncora, materiais das peças, planeta analítico e ausência de handles físicos no snapshot.
3. Testar falha atômica: uma reconfiguração inválida preserva a sessão anterior byte a byte.
4. Adicionar `PhysicsWorld::commit_pending_initial_state()`, permitido somente antes do primeiro step; testar que aplica criações sem integrar, avançar tick ou emitir contatos, e rejeita uso posterior.
5. Implementar registros internos de entidade/parte/junta e criação canônica por ID crescente, usando o commit inicial controlado para produzir snapshot tick zero.
6. Criar shapes Box3D e joints somente através de `PhysicsWorld`; cenário permanente usa `PhysicsSurfaceDefinition` e fica fora dos materiais destrutíveis.
7. Implementar `restart()` reconstruindo do manifesto imutável; limpar comandos/eventos e reiniciar sequências.
8. Implementar `canonical_state_v1`: campos fixos, inteiros little-endian, floats a `1e-5`, zero normalizado, strings UTF-8 length-prefixed; excluir timings, endereços, handles e métricas. Repetir 20 reinícios e comparar bodies, joints, snapshot/hash; obter bytes do alocador somente por façade interna ligada ao target de testes.
9. Verificar:

```powershell
.\tools\Invoke-Native.ps1 -Command 'ctest --preset debug -R session_bootstrap --output-on-failure'
.\build\debug\native\simulation\ninho_simulation_tests.exe --filter "session restart"
```

10. Commit: `feat: inicializar sessao deterministica da fase`.

## Task 4 — Fila de comandos, mira, lançamento e FSM

**Arquivos:**

- Modificar: `native/simulation/include/ninho/simulation/commands.hpp`
- Modificar: `native/simulation/include/ninho/simulation/events.hpp`
- Modificar: `native/simulation/include/ninho/simulation/session.hpp`
- Modificar: `native/simulation/src/session.cpp`
- Criar: `native/simulation/src/launch_system.cpp`
- Criar: `native/simulation/tests/launch_fsm_tests.cpp`
- Modificar: `native/simulation/CMakeLists.txt`

**Passos:**

1. Testar `Inspection -> Aim -> FlightAbility -> Resolution -> Evaluation`, com ramo para `Inspection` quando objetivo está pendente e há aves, para `Result` somente em vitória ou sem aves e para `Faulted` em erro latched; cobrir vitória no primeiro, segundo e terceiro lançamento e derrota após o terceiro.
2. Testar fila limitada a 128, sequência interna, execução no próximo tick e coalescência da última mira do tick.
3. Testar quantização de posição `1 mm`, direção `1e-4` renormalizada e velocidade `0,01 m/s`.
4. Testar shell `R+3`, arco `θ∈[-50°,+50°]`, tolerância `5 cm`, tangência `abs(dot)<=0,01`, limite global `8–40 m/s` e clamp da fase `8–16 m/s` com padrão `10,5`.
5. Testar criação do projétil `bullet`, raio `0,45 m`, velocidade tangente e evento `BirdLaunched` único.
6. Implementar processamento do tick na ordem especificada, sem callback mutar gameplay.
7. Definir `events()` como último tick somente; `tick()` retorna status, latcheia `Faulted` e rejeita avanço posterior até restart/configuração.
8. Implementar repouso por 60 ticks, vida do projétil 600 ticks e watchdog absoluto 1800, garantindo `Evaluation` após terminar habilidade ativa.
9. Implementar `TrajectoryPreview` no kernel usando a mesma quantização, gravidade, shapes e filtros do lançamento; testar amostras, primeiro impacto e erro `<=0,10 m` antes de alterações dinâmicas.
10. Salvar golden de trajetória como valores numéricos/hash no teste, não como arquivo dependente de locale.
11. Verificar:

```powershell
.\tools\Invoke-Native.ps1 -Command 'ctest --preset debug -R launch_fsm --output-on-failure'
.\build\debug\native\simulation\ninho_simulation_tests.exe --filter "launch fsm"
```

12. Commit: `feat: implementar mira orbital e fluxo de lancamento`.

## Task 5 — Habilidade gravimétrica Virela

**Arquivos:**

- Modificar: `native/simulation/include/ninho/simulation/events.hpp`
- Modificar: `native/simulation/src/session.cpp`
- Criar: `native/simulation/src/gravity_field_ability_system.cpp`
- Criar: `native/simulation/tests/gravity_field_ability_tests.cpp`
- Modificar: `native/simulation/CMakeLists.txt`

**Passos:**

1. Testar armamento em 9 ticks, golden `L+9`/`L+83`, rejeição antes/depois da janela ou em segundo acionamento.
2. Testar duração exata de 75 ticks, raio `4 m`, massa `<=150 kg`, até 20 corpos e exclusões.
3. Testar origem acompanhando a Virela; tick aceito é o primeiro dos 75, aplica força antes do step; tick 75 aplica atração, pulso `4 m/s` e encerra após o solver.
4. Testar peso smoothstep, aceleração máxima `12 m/s²` e `force=mass*acceleration`.
5. Testar candidatos em ordem canônica `(EntityId, PartId)` e eventos `AbilityStarted/AffectedBody/Pulse/Ended` ordenados; handles entram somente depois da seleção.
6. Testar que o campo iniciado termina mesmo após impacto e que expiração sem ativação fecha no primeiro impacto/ejeção/600 ticks.
7. Implementar queries e forças via `PhysicsWorld`, sem acesso direto ao Box3D.
8. Repetir o mesmo cenário ao menos 10 vezes e comparar hash/eventos.
9. Verificar:

```powershell
.\tools\Invoke-Native.ps1 -Command 'ctest --preset debug -R gravity_field_ability --output-on-failure'
.\build\debug\native\simulation\ninho_simulation_tests.exe --filter "gravity field ability"
```

10. Commit: `feat: adicionar habilidade gravimetrica da Virela`.

## Task 6 — Dano material e Javali-Âncora

**Arquivos:**

- Modificar: `native/simulation/include/ninho/simulation/events.hpp`
- Modificar: `native/simulation/src/session.cpp`
- Criar: `native/simulation/src/damage_system.cpp`
- Criar: `native/simulation/tests/damage_anchor_tests.cpp`
- Modificar: `native/simulation/CMakeLists.txt`
- Modificar se necessário: `native/kernel/include/ninho/physics/physics_types.hpp`
- Modificar se necessário: `native/kernel/src/physics_world.cpp`

**Passos:**

1. Fixar em teste a convenção da normal de contato `a -> b` antes de implementar proteção direcional.
2. Testar deduplicação do maior impacto por par/tick e fórmula de energia existente.
3. Testar respostas: pinho e tijolo acumulam; vidro requer pico individual; dano não regenera.
4. Testar Âncora `480 kg`, integridade `100`, teto `55`, denominador `mass*80`, frente `-Z`, cone `45°`, frontal `0,25`, lateral/traseiro `1,0`.
5. Testar neutralização única por integridade e por primeira transição de ejeção, inclusive ocorrendo no mesmo tick.
6. Implementar dano somente após `PhysicsWorld::step()`, usando eventos canônicos e IDs de domínio.
7. Publicar `DamageApplied` e `EntityNeutralized` com causalidade, posição, normal e energia.
8. Verificar:

```powershell
.\tools\Invoke-Native.ps1 -Command 'ctest --preset debug -R damage_anchor --output-on-failure'
.\build\debug\native\simulation\ninho_simulation_tests.exe --filter "damage anchor contact normal"
```

9. Commit: `feat: implementar dano e protecao do javali ancora`.

## Task 7 — Juntas, fratura, objetivos e playthroughs

**Arquivos:**

- Modificar: `native/simulation/include/ninho/simulation/events.hpp`
- Modificar: `native/simulation/include/ninho/simulation/session.hpp`
- Modificar: `native/simulation/src/session.cpp`
- Criar: `native/simulation/src/fracture_system.cpp`
- Criar: `native/simulation/src/objective_system.cpp`
- Criar: `native/simulation/tests/fracture_objective_tests.cpp`
- Criar: `native/simulation/tests/playthrough_tests.cpp`
- Modificar: `native/simulation/CMakeLists.txt`

**Passos:**

1. Testar ruptura em `ratio>=1,5` num tick ou `>=1,0` em dois ticks consecutivos; abaixo do limite zera o contador.
2. Testar que ruptura é agendada após o solver, aplicada no início do tick seguinte e desempata joint incidente por distância/`JointId`.
3. Testar `JointBroken` e `PieceFractured` com `cause_event_id` válido e sem duplicidade.
4. Testar objetivo monotônico, vitória somente em `Evaluation`, retorno a `Inspection` com aves e derrota após três Virelas, sem conclusão em callback.
5. Criar dois scripts headless determinísticos de vitória (rota Virela e rota estrutural) e um de derrota.
6. Implementar assemblies pré-segmentados, sem substituição arbitrária pai->fragmentos neste marco.
7. Produzir `canonical_state_v1` e stream de eventos; comparar entre 10 repetições e Debug/Release usando somente os campos canônicos.
8. Verificar:

```powershell
.\tools\Invoke-Native.ps1 -Command 'ctest --preset debug -R "fracture_objective|vertical_slice_playthrough|vertical_slice_determinism" --output-on-failure'
.\build\debug\native\simulation\ninho_simulation_tests.exe --filter playthrough
```

9. Commit: `feat: concluir destruicao objetivos e resultados`.

## Task 8 — Adaptador `OrbitalSessionNode`

**Arquivos:**

- Criar: `native/extension/include/ninho/extension/orbital_session_node.hpp`
- Criar: `native/extension/src/orbital_session_node.cpp`
- Modificar: `native/extension/src/register_types.cpp`
- Modificar: `native/extension/CMakeLists.txt`
- Criar: `native/extension/tests/orbital_session_adapter_tests.cpp`
- Criar: `native/extension/tests/session_frame_batch_tests.cpp`
- Modificar: `native/extension/tests/registration_contract_tests.cpp`
- Criar: `game/tests/orbital_session_node_smoke.gd`

**Passos:**

1. Testar registro de `Box3DWorldNode` e `OrbitalSessionNode` sem alterar o contrato anterior.
2. Extrair acumulador, batching e conversões para helpers C++ puros; testar comandos, rollback, falhas e captura antes da ABI sem instanciar `Variant/Object` fora do engine.
3. Testar acumulador puro: `1/60`, até quatro ticks por frame, tempo descartado explicitamente medido.
4. Testar batching puro: copiar `events()` de cada tick exatamente uma vez, preservar todos os eventos, somente snapshot/preview mais recentes e fault latched.
5. Implementar `configure_session(materials_json, archetypes_json, level_json)`, cinco comandos/restart e `consume_frame()` com o dicionário aprovado.
6. Garantir que `consume_frame()` limpa eventos pendentes mas preserva estado/snapshots; nenhum getter por corpo.
7. Definir priority `-100` no node e `0` no controller. Testar bindings, sinal, prioridade e ordem node->controller dentro do Godot headless.
8. Emitir `gameplay_fault(code,message)` sem deixar exceção atravessar GDExtension; bloquear frames seguintes até restart/configuração válida.
9. Verificar:

```powershell
.\tools\Invoke-Native.ps1 -Command 'ctest --preset debug -R "gdextension_adapter_helpers|gdextension_vertical_slice|gdextension_binary_exists" --output-on-failure'
```

10. Commit: `feat: expor sessao orbital ao Godot em lote`.

## Task 9 — Cena Godot jogável, câmera, input e HUD

**Arquivos:**

- Criar: `game/scenes/vertical_slice.tscn`
- Criar: `game/scripts/game/vertical_slice_controller.gd`
- Criar: `game/scripts/game/launch_controller.gd`
- Criar: `game/scripts/game/body_view_registry.gd`
- Criar: `game/scripts/camera/orbital_camera.gd`
- Criar: `game/scripts/ui/vertical_slice_hud.gd`
- Criar: `game/scripts/ui/vertical_slice_hud.tscn`
- Criar: `game/tests/vertical_slice_smoke.gd`
- Criar: `game/tests/forbid_godot_physics.gd`
- Modificar: `game/project.godot`
- Modificar: `tools/test.ps1`
- Modificar: `tools/tests/godot-spike-gate-tests.ps1`

**Passos:**

1. Criar smoke headless vermelho que carrega a cena, configura sessão, mira, lança, ativa Virela, recebe resultado, reinicia e sai com marcador inequívoco.
2. Criar scanner vermelho que falha para `RigidBody3D`, `StaticBody3D`, `CharacterBody3D`, `Area3D`, `CollisionShape3D` e `CollisionObject3D`, inspecionando arquivos e árvore runtime/nodes criados por script.
3. Construir a cena exclusivamente com `Node3D`, `OrbitalSessionNode`, câmera/luzes, meshes, partículas, áudio e UI.
4. Fazer o controller chamar exatamente um `consume_frame()` por `_physics_process`; views interpolam snapshots sem autoridade.
5. Implementar inspeção, mira tangente, `Espaço` para lançar/ativar, `Esc` para cancelar antes da pausa, `F` para recentrar, `R` segurado 0,5 s para reiniciar e saída.
6. Implementar câmera sem roll, FOV 48°, distância 14–24 m, inclinação 15–70°, look-ahead 2 m, modo reduzir movimento e correção de oclusão/enquadramento seguro em 16:9, 16:10 e 21:9.
7. Renderizar o `TrajectoryPreview` do kernel, primeiro impacto, contador de Virelas, integridade, fase, vitória e derrota; Godot não recalcula a trajetória.
8. Alterar `tools/test.ps1` para abrir explicitamente `res://scenes/physics_spike.tscn` e provar que o marcador da fundação continua independente de `run/main_scene`.
9. Separar smoke lógico `--headless` de capturas ocultas renderizadas em Forward Mobile/Vulkan e Compatibility/OpenGL; exigir logs sem `ERROR`, `SCRIPT ERROR` ou `WARNING`.
10. Commit: `feat: criar experiencia jogavel da primeira orbita`.

## Task 10 — Pipeline Blender e assets autorais reproduzíveis

**Arquivos:**

- Criar: `art/config/vertical_slice_assets.json`
- Criar: `art/scripts/build_vertical_slice.py`
- Criar: `art/scripts/ninho_blender/__init__.py`
- Criar: `art/scripts/ninho_blender/materials.py`
- Criar: `art/scripts/ninho_blender/geometry.py`
- Criar: `art/scripts/ninho_blender/export.py`
- Criar: `art/source/vertical_slice/*.blend`
- Criar: `game/assets/vertical_slice/**/*.glb`
- Criar: `game/materials/**/*.tres`
- Criar: `game/shaders/stylized_glass.gdshader`
- Criar: `tools/art/build_assets.ps1`
- Criar: `tools/art/validate_assets.ps1`
- Criar: `tools/art/vertical_slice_asset_manifest.json`
- Criar: `tools/tests/art-pipeline-tests.ps1`
- Modificar: `game/data/levels/first_orbit.level.json`
- Modificar: `game/scenes/vertical_slice.tscn`
- Modificar: `game/scripts/game/body_view_registry.gd`

**Passos:**

1. Escrever validação vermelha para Blender/version/hash, seed, nomes, transforms, bounds, LODs, materiais, pivôs, hulls, contagem, hashes e igualdade proxy->level manifest.
2. Gerar proceduralmente Jardim Aster, Virela, Âncora, anel e kit de pinho/vidro/tijolo com a paleta da especificação.
3. Usar convenções `AST_/CHR_/ENM_/DEV_/KIT_` e `VIS_/COL_/FRAG_/SOCKET_/RIG_`; metros, Z-up, frente local `-Y`, pivôs no COM e escala aplicada.
4. Exportar GLB com custom properties; nunca usar sufixos Godot `-col`.
5. Reabrir exportados em processo limpo e validar hulls convexos, bounds, LODs, determinante positivo, materiais e budgets.
6. Gerar fontes `.blend`, GLBs e manifesto com Blender 5.1.2, hash do executável, seed, SHA-256 dos outputs, triângulos, materiais e volumes.
7. Usar `art/config/vertical_slice_assets.json` como fonte autoral e gerar manifesto intermediário canônico; validar IDs/bounds/transforms iguais aos proxies de `first_orbit.level.json`.
8. Substituir placeholders da Tarefa 9 pelos GLBs nas views Godot, sem criar colliders Godot.
9. Verificar reprodução limpa duas vezes com hashes idênticos.
10. Commit: `art: gerar assets autorais da primeira orbita`.

## Task 11 — VFX, áudio, leitura material e polimento

**Arquivos:**

- Criar/Modificar: `game/scripts/vfx/*.gd`
- Criar/Modificar: `game/scripts/audio/*.gd`
- Criar: `game/assets/audio/README.md`
- Criar: `game/assets/audio/generated/*`
- Modificar: `game/scenes/vertical_slice.tscn`
- Modificar: `game/scripts/ui/vertical_slice_hud.gd`
- Criar: `game/tests/feedback_smoke.gd`

**Passos:**

1. Testar mapeamento evento->feedback e pooling, incluindo nenhuma instância órfã após 20 reinícios.
2. Implementar constelação gravimétrica turquesa, pulso Virela, respostas distintas de vidro/pinho/tijolo e leitura frontal/vulnerável do Âncora.
3. Gerar áudio original/procedural para lançamento, vórtice, três materiais, capacete, vulnerabilidade e resultados; documentar origem/licença.
4. Garantir primeiro impacto visível em menos de 150 ms e VFX sem ocultá-lo.
5. Aplicar budgets: `<=30k` partículas, `<=120` fragmentos, transparência de vidro `<=15%` da tela, pools fixos.
6. Polir UI astral, acessibilidade de contraste, volumes e reduzir movimento.
7. Verificar smoke e logs nos dois renderers.
8. Commit: `feat: polir feedback audiovisual e acessibilidade`.

## Task 12 — Gate profissional, evidências e pacote Windows

**Arquivos:**

- Criar: `tools/test_vertical_slice.ps1`
- Criar: `tools/VerticalSliceGate.psm1`
- Criar: `tools/tests/vertical-slice-gate-tests.ps1`
- Criar: `tools/capture_vertical_slice.ps1`
- Criar: `tools/package_windows.ps1`
- Criar: `tools/generate_vertical_slice_report.py`
- Criar: `tools/tests/test_generate_vertical_slice_report.py`
- Criar: `docs/gameplay/vertical-slice-report.md`
- Criar: `docs/gameplay/evidence/vertical-slice-*.json`
- Criar: `docs/art/goldens/vertical-slice/*`
- Modificar: `.gitignore`
- Modificar: `tools/toolchain.lock.json`
- Modificar: `tools/bootstrap.ps1`
- Criar: `game/export_presets.cfg`
- Criar: `third_party/godot-export-templates.LICENSE.txt`
- Modificar: `THIRD_PARTY_NOTICES.md`
- Modificar: `third_party/sbom.spdx.json`
- Modificar: `third_party/README.md`
- Modificar: `README.md` se existir; caso contrário criar `README.md`

**Passos:**

1. Escrever testes vermelhos do gate contra evidência ausente, stale, renderer incompleto, hash divergente e log contaminado.
2. Implementar runner que primeiro executa o gate completo da fundação e depois suíte do slice.
3. Executar Debug e Release com upstream Box3D, playthroughs, determinismo, 20 reinícios, ABI, scene scanner, assets e smokes.
4. Capturar 300 frames e goldens de overview, mira, Virela, impacto e resultado em Vulkan/OpenGL; registrar tick/evento/câmera/exposição/hash e aprovar rubric editorial bloqueante.
5. Medir p95 físico `<=8 ms`; em hardware registrado exigir 1080p médio p95 de frame `<=16,67 ms`, p99 `<=25 ms`, nenhum hitch `>50 ms` durante ruptura/VFX e input-engine->feedback p95 `<=33,4 ms`, nos dois renderers e UI 100%/150%.
6. Executar playtest com cinco pessoas novas quando disponível usando as métricas da especificação; até lá, usar agentes independentes com a mesma rubric e bloquear findings graves de legibilidade.
7. Executar checklist clean-room, proveniência de assets e revisão comparativa de nomes/silhuetas/sons/UI; manter Virela/Nox/Talo como codenames até clearance e registrar que isso não substitui parecer jurídico.
8. Fixar export templates Godot 4.5.1 por URL/SHA-256 no lock/bootstrap/licenças, atualizar NOTICE/SBOM/README com versão, URL, hash, licença e `DEPENDS_ON`, validar SPDX semanticamente, criar `game/export_presets.cfg` e gerar pacote Windows com executável, DLL correta, dados, licenças e manifesto de hashes; iniciar a partir de caminho com espaços.
9. Rodar revisão de código, arquitetura, gameplay e arte com agentes distintos; corrigir Critical/Important e registrar findings restantes aceitos.
10. Executar gates finais frescos:

```powershell
.\tools\test_vertical_slice.ps1 -Configuration Debug -IncludeUpstream -IncludeVisualGate
.\tools\test_vertical_slice.ps1 -Configuration Release -IncludeUpstream -IncludeVisualGate
python .\tools\generate_vertical_slice_report.py --check
git diff --check
git status --short
```

11. Commit: `test: certificar vertical slice da primeira orbita`.

---

## Gate de conclusão

O branch só está pronto para integração quando:

- a fundação e o slice passam integralmente em Debug e Release;
- as duas vitórias e a derrota são determinísticas;
- Virela, materiais, Âncora, juntas, objetivos e eventos obedecem exatamente à especificação;
- `consume_frame()` preserva eventos intermediários e é usado uma vez por frame;
- 20 reinícios não aumentam recursos;
- não existe física Godot no gameplay;
- assets Blender são autorais, reproduzíveis e dentro dos budgets;
- capturas de 300 frames demonstram o fluxo completo sem erro/warning;
- p95 físico é `<=8 ms` e os gates gráficos/latência aprovados passam no hardware registrado;
- pacote Windows abre a partir de caminho com espaços;
- revisões independentes não possuem finding Critical/Important aberto;
- relatório, SPDX, NOTICE e hashes correspondem exatamente aos artefatos verificados.

## Roadmap contratado após o gate

Este plano entrega o primeiro Vertical Slice, não encerra o pedido 3/3/20. A sequência obrigatória é:

1. **Marco Arquétipos 3/3:** habilitar três aves e três inimigos pelos registros já criados, com matriz 3×3, fases de demonstração, assets, balanceamento e testes determinísticos.
2. **Marco Catálogo 20:** habilitar vinte materiais com física, dano, ruptura, áudio/VFX e legibilidade próprios; executar matrizes 3 aves × 20 materiais e 3 inimigos × 20 materiais sem branches por nome.
3. **Marco Produto Windows:** campanha mínima, seleção/save, onboarding, opções, playtests e pacote candidato, preservando gates de performance, acessibilidade, IP, licenças e Box3D.

Cada marco terá especificação e plano próprios, revisão independente e Debug/Release verdes. O kernel do slice deve tornar essa expansão aditiva, sem reescrever Virela ou Âncora.
