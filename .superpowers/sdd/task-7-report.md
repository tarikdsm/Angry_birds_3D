# Task 7 — Implementar launcher 3D guiado e Hooke

## Status

DONE. O launcher v2 foi implementado no kernel C++ sem dependência de Godot, com comandos próprios, plano 3D travado, Hooke autoritativo, ghost durante o grab, consumo ordenado da fila e preview uniforme/radial. O caminho orbital v1 permanece no adaptador legado e os formatos canônicos v1/v2 não foram alterados.

## RED observado

1. Primeiro build Debug após registrar `launcher_system_tests.cpp` e `add_test(NAME launcher_system)`:
   - falhou por `launcher_system.hpp` inexistente;
   - falhou porque `BeginGrabCommand`, `SetPullCommand`, `ReleaseBirdCommand` e `CancelGrabCommand` ainda não existiam;
   - exit code 1, como esperado pela ausência da feature.
2. Após a primeira implementação, o teste focado expôs que quantizar o `camera_right` normalizado antes da projeção deixava o plano terrestre variar alguns bits com pitch diferente:
   - `pitched.state()->horizontal == locked.horizontal` falhou;
   - a projeção passou a usar o vetor de câmera quantizado antes da normalização, mantendo pitch fora do plano.
3. Durante o self-review, um novo teste com rest `(-4.0004, 2.0004, 0.0004)` falhou porque a posição publicada ainda não obedecia ao quantum posicional de 1 mm:
   - `close(locked.rest_position_m, {-4, 2, 0})` falhou;
   - o rest passou a ser quantizado a 1 mm antes de calcular o plano e de resolver o spawn.

## GREEN observado

- `tools/build.ps1 -Configuration Debug`: PASS, build MSVC `/fp:precise` sem warnings de compilação.
- CTest focado `^launcher_system$`: PASS após cada correção RED.
- Binário completo `build/debug/native/simulation/ninho_simulation_tests.exe`: exit 0; 155 casos registrados, todos PASS.
- CTest oficial final:
  - `launcher_system`: PASS;
  - `launch_fsm`: PASS;
  - `vertical_slice_playthrough`: PASS;
  - `legacy_orbital_characterization`: PASS;
  - total 4/4, 0 falhas, 24,20 s.

## Arquivos

Criados:

- `native/simulation/src/launcher_system.hpp`
- `native/simulation/src/launcher_system.cpp`
- `native/simulation/tests/launcher_system_tests.cpp`

Modificados:

- `native/simulation/include/ninho/simulation/commands.hpp`
- `native/simulation/include/ninho/simulation/session.hpp`
- `native/simulation/src/launch_system.cpp`
- `native/simulation/src/session_builder.cpp`
- `native/simulation/src/session_internal.hpp`
- `native/simulation/CMakeLists.txt`
- `native/simulation/tests/launch_fsm_tests.cpp`

## Decisões e contratos

- Os cinco comandos v1 preservam os índices `0..4`; os quatro comandos v2 foram anexados como `5..8`.
- `SessionPhase::Grabbed` foi anexado depois de todos os valores v1, preservando os tags legados `0..6`.
- `LauncherState` publica rest, `camera_right` aceito, up/horizontal/plane normal travados, pull, extensão, energias, direção, velocidade prevista, deadzone e extensão máxima.
- Mundo uniforme usa up `+Y`; mundo radial usa `normalize(rest - center)`. O horizontal é a projeção tangente do `camera_right`; `plane_normal = normalize(cross(horizontal, up))`.
- Rest e pull usam quantum de 1 mm; vetores de base/direção usam componentes a `1e-4` seguidos de normalização; velocidade publicada/efetiva usa 0,01 m/s.
- O clamp é circular e conserva uma representação milimétrica sem ultrapassar 4,25 m.
- `spring_energy_j = 0,5*k*x²` e `launch_energy_j = eta*spring_energy_j`; a velocidade usa `x*sqrt(eta*k/m)` e o menor cap entre ave e launcher. Energia acima do cap não aumenta a velocidade.
- O corpo v2 usa a massa autorada para Hooke e deriva a densidade esférica necessária para o Box3D reproduzir essa massa. Ele nasce na origem resolvida, que é exatamente o rest quantizado.
- `BeginGrab` e `SetPull` não criam corpo. Release abaixo da deadzone e cancel limpam o grab sem consumir fila; release válido cria um corpo e decrementa exatamente uma entrada ordenada.
- Preview e release consomem o mesmo `LauncherSolution`. O preview usa `PhysicsWorld::gravity_at`, cast do mesmo raio da ave, bounds tipados e máximo de três segundos.
- O caminho v1 continua usando `BeginAim/SetAim/Launch`, o preview orbital legado e o roster por contagem. Nenhum comando v1 é traduzido para Hooke.
- Schema v2 continua retornando canonical state/hash v2 vazios. `ShotState` e canonical v3 permanecem integralmente para a Task 8.

## Cobertura do brief

- Planos terrestre e orbital, yaw/pitch e tangência: cobertos.
- Base byte-idêntica durante grab, relock rejeitado e câmera inválida: cobertos.
- Quantização, clamp, deadzone, Hooke, massa, direção e cap: cobertos.
- Ghost sem body, cancel/deadzone sem consumo, release único e visual v2: cobertos.
- Solved state compartilhado, gravidade uniforme/radial, parada por bounds/3 s e erro de primeiro impacto <= 0,10 m: cobertos.
- Índices/comandos e playthrough/canonical do adaptador v1: cobertos pelas regressões oficiais.

## Concerns

Nenhum finding aberto da Task 7. Deliberadamente não foram introduzidos `ShotState`, serializer canonical v3, frame ABI v2/GDExtension, habilidades novas, conteúdo final, evidências ou mudanças em física Godot; esses itens pertencem às tarefas posteriores.
