# Vertical Slice Balance Correction Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** tornar as rotas Virela e estrutural fisicamente vencíveis em até três aves, sem façades de vitória e sem alterar os contratos sistêmicos de Virela, Âncora, Box3D ou determinismo.

**Architecture:** `SimulationSession` continua autoritativa. A ruptura recebe um evento causal mecânico explícito; depois uma calibração limitada escolhe o menor tuple de dados do nível que faz dois playthroughs públicos vencerem e um perder, registrando o tuple final na especificação e nos JSONs. A busca é ferramenta de autoria/teste, nunca regra de runtime.

**Tech Stack:** C++20, Box3D 3D v0.1.0, nlohmann/json v3.11.3, CMake/CTest, JSON de conteúdo.

## Global Constraints

- Virela permanece com armamento 9 ticks, duração 75 ticks, raio 4 m, massa elegível ≤150 kg, aceleração 12 m/s² e pulso 4 m/s.
- Âncora permanece com massa 480 kg, integridade 100, teto 55, cone 45° e multiplicadores 0,25/1,0.
- Fórmulas de energia, dano, ejeção e ruptura não mudam.
- Densidades, atrito, restituição, contagens, IDs, arco e velocidades públicas 8–16 m/s não mudam.
- `PhysicsWorld` é a única fronteira Box3D; `SimulationSession` é a autoridade de gameplay.
- Playthroughs não podem chamar `neutralize_entity`, `request_fracture`, `finish_projectile` nem mutar objetivo/outcome.
- A busca minimiza primeiro campos alterados, depois variação relativa e por fim o tuple JSON canônico.

---

### Task 1: Causalidade mecânica autoritativa

**Files:**
- Modify: `native/simulation/include/ninho/simulation/events.hpp`
- Modify: `native/simulation/src/canonical_state.cpp`
- Modify: `native/simulation/src/fracture_system.cpp`
- Modify: `native/simulation/src/session.cpp`
- Modify: `native/simulation/src/session_internal.hpp`
- Test: `native/simulation/tests/fracture_objective_tests.cpp`

**Interfaces:**
- Consumes: reaction ratio pós-solver e fila de rupturas já existente.
- Produces: `DomainEventKind::JointOverloaded`, `DomainEvent::joint_load_ratio` e cadeia `JointOverloaded.id -> JointBroken/PieceFractured.cause_event_id`.

- [ ] **Step 1: Preserve the observed RED**

Confirmar no relatório scratch que os testes falharam por ausência de `JointOverloaded`/`joint_load_ratio` e que exigiam `1,5` e `1,0×2` sem `DamageApplied`.

- [ ] **Step 2: Complete the minimal mechanical event**

Implementar a sequência abaixo sem exigir evento de dano:

```cpp
if (ratio >= 1.5 || consecutive_overload_ticks >= 2) {
    DomainEvent overload{};
    overload.kind = DomainEventKind::JointOverloaded;
    overload.joint_id = joint.id;
    overload.joint_load_ratio = ratio;
    publish(overload);
    pending_joint_breaks.push_back({joint.id, overload.id});
}
```

`JointBroken` e `PieceFractured` usam o ID do overload; hooks de teste apenas sobrescrevem ratio e nunca publicam `DamageApplied`.

- [ ] **Step 3: Verify focused GREEN**

Run:

```powershell
.\tools\Invoke-Native.ps1 -Command 'ctest --preset debug -R fracture_objective --output-on-failure'
.\build\debug\native\simulation\ninho_simulation_tests.exe --filter "fracture objective"
```

Expected: `1/1` CTest e `5/5` casos; ruptura sem dano; nenhum `DamageApplied` fabricado; aplicação no tick seguinte; todo `cause_event_id` resolve exatamente um `JointOverloaded` anterior.

- [ ] **Step 4: Commit the mechanical correction**

```powershell
git add native/simulation/include/ninho/simulation/events.hpp native/simulation/src/canonical_state.cpp native/simulation/src/fracture_system.cpp native/simulation/src/session.cpp native/simulation/src/session_internal.hpp native/simulation/tests/fracture_objective_tests.cpp
git commit -m "fix: tornar ruptura mecanicamente autoritativa"
```

### Task 2: Calibração limitada e playthroughs físicos

**Files:**
- Modify: `game/data/levels/first_orbit.level.json`
- Modify if selected: `game/data/materials/vertical_slice.materials.json`
- Modify if selected: `game/data/archetypes/vertical_slice.archetypes.json`
- Modify: `docs/gameplay/vertical-slice-content-schema.md`
- Modify: `docs/superpowers/specs/2026-07-11-vertical-slice-design.md`
- Test: `native/simulation/tests/content_tests.cpp`
- Test: `native/simulation/tests/playthrough_tests.cpp`

**Interfaces:**
- Consumes: comandos públicos `BeginAim`, `SetAim`, `Launch`, `ActivateAbility`, `tick()` e eventos canônicos.
- Produces: dois scripts de vitória e um de derrota, tuple final explícito e assinatura canônica Debug/Release.

- [ ] **Step 1: Keep the two public victories RED**

Os testes devem construir sessões somente a partir dos três JSONs rastreados, enfileirar comandos públicos e acumular `events()` após cada tick. As rotas de vitória falham em `Outcome::Defeat`; a derrota passa.

- [ ] **Step 2: Add a bounded authoring search**

Criar um helper somente de teste que avalia no máximo 2.000 simulações completas e aborta se exceder o limite. Ele percorre, na ordem:

```cpp
constexpr double stage_pitch_deg[] = {15, 20, 25, 30, 35, 40};
constexpr double counterweight_radial_offset_m[] = {0, 1.5, 2.5, 3.5};
constexpr double mortar_force_n[] = {4000, 3000, 2000, 1500, 1200, 950};
constexpr double mortar_torque_nm[] = {700, 500, 350, 250, 160};
constexpr double glass_toughness[] = {0.16, 0.12, 0.09, 0.069, 0.06};
constexpr double anchor_energy_j_per_kg[] = {80, 78, 76.2, 75};
constexpr double launch_theta_deg[] = {-50, -40, -30, -20, -10, 0, 10, 20, 30, 40, 50};
constexpr double launch_plane_deg[] = {-90, -60, -30, 0, 30, 60, 90};
constexpr double launch_speed_m_s[] = {8, 10, 12, 14, 16};
constexpr std::uint32_t activation_delay_ticks[] = {9, 30, 60, 90, 120};
```

Antes de uma simulação completa, usar `trajectory_preview()` para rejeitar comandos cujo primeiro impacto/menor distância não cruza o envelope da fortificação. Avaliar primeiro somente transforms; abrir a dimensão seguinte apenas quando todas as anteriores falharem. Ordenar os tuples por número de campos alterados, soma das variações relativas e JSON canônico.

- [ ] **Step 3: Freeze the first passing tuple**

Quando um tuple fizer ambas as rotas vencerem, remover o loop de busca e gravar os valores explicitamente nos JSONs e na especificação. Manter um teste de contrato com o tuple exato e um limite de três aves. O relatório scratch registra candidatos executados, métricas e o primeiro tuple aprovado.

Se os intervalos iniciais terminarem sem passe, aplicar exatamente o fallback da seção 8 da emenda de design: tijolos `138–145 kg`, queda radial `1,5–2,5 m`, envelope `≤4,5 m`, vidro `0,01`, argamassa `950 N / 160 N·m` e Âncora `40 J/kg`. Nesse fallback, executar no máximo 500 simulações adicionais e variar somente transforms e comandos públicos; não reduzir novamente nenhum parâmetro.

- [ ] **Step 4: Prove route semantics**

Rota Virela exige, no stream:

```cpp
AbilityStarted, AbilityAffectedBody, AbilityPulse, AbilityEnded,
DamageApplied || EntityNeutralized, Outcome::Victory
```

Rota estrutural exige:

```cpp
JointOverloaded, JointBroken, PieceFractured,
DamageApplied || EntityNeutralized, Outcome::Victory
```

Para cada evento com `cause_event_id != 0`, procurar exatamente um evento anterior com o mesmo `id`. A derrota usa três aves e termina em `Outcome::Defeat`.

- [ ] **Step 5: Verify determinism and regressions**

Run:

```powershell
.\tools\Invoke-Native.ps1 -Command 'ctest --preset debug -R "fracture_objective|vertical_slice_playthrough|vertical_slice_determinism" --output-on-failure'
.\build\debug\native\simulation\ninho_simulation_tests.exe
.\tools\build.ps1 -Configuration Release
.\build\release\native\simulation\ninho_simulation_tests.exe
git diff --check
```

Expected: todos os testes passam; duas vitórias/uma derrota; dez repetições idênticas; assinatura canônica Debug/Release idêntica; nenhum diagnóstico temporário.

- [ ] **Step 6: Commit the calibrated content**

```powershell
git add game/data docs/gameplay/vertical-slice-content-schema.md docs/superpowers/specs/2026-07-11-vertical-slice-design.md native/simulation/tests/content_tests.cpp native/simulation/tests/playthrough_tests.cpp
git commit -m "fix: calibrar rotas fisicas da primeira orbita"
```

## Plan Self-Review

- [x] A causalidade mecânica não depende de dano.
- [x] As duas rotas não possuem atalhos de façade.
- [x] A busca é limitada, ordenada e removida antes do commit final.
- [x] Valores sistêmicos imutáveis estão copiados da emenda de design.
- [x] Conteúdo final, eventos, determinismo e Debug/Release têm gates explícitos.

## Autonomous Execution Choice

Use **Subagent-Driven Development**. O usuário delegou decisões e pediu conclusão autônoma; executar Task 1 e Task 2 com implementadores/revisores distintos, sem novo gate humano salvo nova incompatibilidade normativa comprovada.
