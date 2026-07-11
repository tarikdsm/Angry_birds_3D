# Box3D Foundation Spike Report

Este documento descreve um snapshot auditado e imutável, não o último JSON volátil de `artifacts/physics/`. Os dois snapshots rastreados foram gerados após a inclusão explícita dos picos de bodies, shapes e joints. O gate lê os quatro tokens abaixo, valida JSON/contratos e recalcula SHA-256 antes de compilar.

Evidence-Debug-Path: docs/physics/evidence/foundation-report-debug.json
Evidence-Debug-SHA256: 7B70EFD0CA5D53C1513728D5B50AB794AFF6C390FA7BEB813ECE6CDC4D939275
Evidence-Release-Path: docs/physics/evidence/foundation-report-release.json
Evidence-Release-SHA256: 9D57D1ABBB07C45C44B93866A2DFD11766D9ABE4D7D35A6349656F5401110BA8

Ambos contêm seis cenários, dois repeats por cenário, hashes repetidos, oito capabilities aprovadas, zero violação normativa, warning `private_commit_budget_unqualified` e recommendation `prosseguir_com_limites`.

## Versions

| Componente | Versão/pin | Caminho usado |
| --- | --- | --- |
| Box3D 3D | `v0.1.0`, `8441b4a06d6d09dcfb0b0f704df4d847d1437b92` | `.fetchcontent-cache/box3d-src` limpo |
| Godot | `4.5.1-stable`, `f62fdbde15035c5576dad93e586201f4d41ef0cb` | `.tools/godot/Godot_v4.5.1-stable_win64.exe` |
| godot-cpp | `godot-4.5-stable`, `e83fd0904c13356ed1d4c3d09f8bb9132bdc6b77` | `.fetchcontent-cache/godot_cpp-src` |
| VS Build Tools | `18.7.3`, build `11925.98` | `C:/Program Files (x86)/Microsoft Visual Studio/18/BuildTools` |
| MSVC/SDK | tools `14.44.35207`, compiler `19.44.35228.0`; SDK `10.0.26100.0` | x64, `/MTd` Debug, `/MT` Release |
| CMake/Ninja/Python | `4.3.3` / `1.13.2` / `3.11.9` | toolchain portátil pinada |

O upstream é reconfigurado do zero em `build/upstream-box3d/debug|release`. O runner verifica path absoluto antes da remoção recursiva, rejeita reparse points e valida todos os objetos Box3D/shared/test em `compile_commands.json`: `cl.exe`, CRT estático correto, ausência de `/MD[d]`, `/fp:fast`, contaminação Debug/Release, samples/benchmarks/docs/shaders OFF e unit tests ON.

## Capability Matrix

As oito linhas têm `status=pass`, `functional_status=pass` e nenhum fallback ativado em Debug/Release.

| Capacidade | Prova medida | Fallback permitido/usado |
| --- | --- | --- |
| `ccd_dynamic_dynamic` | 20/20 seeds, 35 m/s, 4 substeps, 0 estados inválidos | 30 m/s + 6 substeps / não usado |
| `shape_cast_overlap` | cast 3 m, handle `1`, overlap concordante, fraction `0.81077587604522705` | raycasts de previsão / não usado |
| `contact_hit_events` | `15 m/s`, massa `41.887905120849609 kg`, energia `4105.0146484375 J`, normal `1`, materiais `111/222`, 1 par | derivar dos bodies / não usado |
| `joint_force_torque` | máximo `12693.1640625 N` cruza `10000 N`; deformação `0.01363062858581543 m` | deformação/impulso por dois ticks / não usado |
| `hulls_compounds` | 8 hulls; massa `42.666671752929688 kg`; AABB dentro de `1e-4`; contato/estado válidos; Box3 `0 -> 0` | múltiplas shapes/body / não usado |
| `radial_sleep` | sleep `1`; p95 linear/angular `0/0`; penetração `0.0015351474285125732 m`; energy growth `0` | omitir força em asleep / não usado |
| `batch_lifecycle` | 10.000 gerações; 0 handles inválidos; Box3 `0 -> 0`; Debug CRT quatro deltas zero | reduzir batch / não usado |
| `upstream_replay` | save/load/validate/remove; `6982` bytes; Box3 `0 -> 0` | replay próprio / não usado |

O hash da matriz é `17270965158708886116` nos dois repeats de ambos os builds. Timing, footprint e ownership probes não entram nesse hash.

## Scenario Metrics

`dynamic_body_count` é a quantidade de bodies dinâmicos da fixture; `shape_count` e `joint_count` são totais declarados da fixture. `peak_body_count`, `peak_shape_count` e `peak_joint_count` são máximos reais lidos de `WorldMetrics` durante step. Awake/contact são picos independentes. A capability matrix não possui uma única fixture (`0/0/0`); seus picos agregados vêm da maior prova instrumentada, a pilha radial.

Tempos são `min/p50/p95/max` em ms e permanecem informativos nesta máquina.

| Build/cenário | Fixture dyn/shape/joint | Pico body/shape/joint | Awake/contact | Step ms | Hashes 1/2 |
| --- | --- | --- | --- | --- | --- |
| Debug `radial_fall` | `1/2/0` | `2/2/0` | `1/1` | `0.0016/0.0018/0.0603/0.4569` | `12100112409900625846/12100112409900625846` |
| Debug `projectile_pile` | `121/123/0` | `123/123/0` | `121/221` | `4.4459/4.6375/15.3640/15.3640` | `1431096453509785832/1431096453509785832` |
| Debug `radial_pile` | `80/81/0` | `81/81/0` | `80/204` | `0.0287/0.0307/0.0740/17.1350` | `10363635776067367757/10363635776067367757` |
| Debug `mass_ratio` | `80/81/0` | `81/81/0` | `80/227` | `0.0284/0.0388/0.0990/16.6784` | `17464060736204574665/17464060736204574665` |
| Debug `stress` | `500/800/250` | `500/800/250` | `500/0` | `0.4541/0.5000/1.4393/3.3289` | `10292935394449293550/10292935394449293550` |
| Debug `capability_matrix` | `0/0/0` | `81/81/0` | `80/204` | `0.0258/0.0280/0.0895/24.1021` | `17270965158708886116/17270965158708886116` |
| Release `radial_fall` | `1/2/0` | `2/2/0` | `1/1` | `0.0002/0.0003/0.0040/0.1183` | `12100112409900625846/12100112409900625846` |
| Release `projectile_pile` | `121/123/0` | `123/123/0` | `121/221` | `0.2248/0.2314/1.1566/1.1566` | `1431096453509785832/1431096453509785832` |
| Release `radial_pile` | `80/81/0` | `81/81/0` | `80/204` | `0.0024/0.0025/0.0032/1.3914` | `10363635776067367757/10363635776067367757` |
| Release `mass_ratio` | `80/81/0` | `81/81/0` | `80/227` | `0.0024/0.0026/0.0040/1.4275` | `17464060736204574665/17464060736204574665` |
| Release `stress` | `500/800/250` | `500/800/250` | `500/0` | `0.0443/0.0477/0.1157/0.3590` | `10292935394449293550/10292935394449293550` |
| Release `capability_matrix` | `0/0/0` | `81/81/0` | `80/204` | `0.0023/0.0026/0.0078/1.4641` | `17270965158708886116/17270965158708886116` |

Métricas funcionais comuns: RadialFall separação `-0.00005951523780822754 m`, velocidades finais `0/0`, energy growth `0`; Projectile primary `20/20`; RadialPile sleep `1`, penetração `0.0015351474285125732 m`; MassRatio `85..3400 kg/m3`, sleep `1`, penetração `0.014884665608406067 m`; Stress executou 10 warmups + 10 medidos, 4 substeps, 500/800/250.

### Ownership normativo

- Process allocator Debug/Release: baseline/final/max delta `0/0/0`, `exact_return=true`.
- Todos os 24 repeat observations: Box3 baseline/final/max delta `0/0/0`, exact return.
- Quatro repeats Stress: warmup e measured post-teardown são dez zeros cada.
- Debug Stress repeats 1/2: CRT applicable/balanced, `_NORMAL_BLOCK` count/bytes `0/0`, `_CLIENT_BLOCK` count/bytes `0/0`.
- Release Stress: CRT not applicable; `/MT` real é validado em todos os compile commands.

### Footprint diagnóstico 10+10

Todos os repeats têm warning `private_commit_budget_unqualified`, `gate_status=diagnostic`, `gate_applied=false`, `budget_qualified=false`, scope `future_packaged_reference_hardware`. O status nunca altera exit/matriz/hash.

**Debug repeat 1 — Private/WS `unstable/unstable`:**

- Private: stable/terminal `false/false`; baseline last/full-min/central-min/median/central-max/full-max `7073792/4751360/6361088/6574080/6828032/7073792`; final `5959680/5259264/5267456/5959680/6598656/6627328`; peak `7073792`; growth `0`; spans warm trimmed/full `0.07102803738317758/0.3532710280373832`, measured `0.22336769759450173/0.229553264604811`; W `[5492736,5238784,6963200,6320128,4718592,6828032,4751360,6574080,6361088,7073792]`; M `[6356992,6258688,4980736,6500352,6574080,5259264,6627328,6598656,5267456,5959680]`.
- WS: stable/terminal `false/false`; baseline `8359936/7483392/8351744/8359936/8364032/8384512`; final `8450048/7548928/7553024/8400896/8450048/8544256`; peak `10067968`; growth/instant `0.004899559039686428/0.010779029887310143`; spans warm `0.0014698677119059284/0.10779029887310142`, measured `0.10677718186250609/0.11847879083373963`; W `[7733248,7495680,8368128,8368128,7467008,8351744,7483392,8364032,8384512,8359936]`; M `[8372224,8417280,7520256,8556544,8536064,7548928,8544256,8400896,7553024,8450048]`.

**Debug repeat 2 — Private/WS `pass/pass`:**

- Private: stable/terminal `true/false`; baseline `6774784/6279168/6352896/6369280/6557696/6774784`; final `6692864/6291456/6365184/6647808/6692864/6905856`; peak `6905856`; growth `0.04372990353697749`; spans warm `0.03215434083601286/0.07781350482315112`, measured `0.04929143561306223/0.09242144177449169`; W `[6053888,4915200,6500352,6344704,5853184,6352896,6369280,6557696,6279168,6774784]`; M `[6488064,6340608,6410240,6606848,6176768,6905856,6647808,6291456,6365184,6692864]`.
- WS: stable/terminal `true/false`; baseline `8445952/8417280/8437760/8445952/8450048/8462336`; final `8585216/8450048/8531968/8536064/8548352/8585216`; peak `10088448`; growth/instant `0.01066925315227934/0.016488845780795344`; spans warm `0.001454898157129001/0.00533462657613967`, measured `0.0019193857965451055/0.01583493282149712`; W `[8417280,7532544,8421376,8433664,8409088,8450048,8417280,8462336,8437760,8445952]`; M `[8536064,8458240,8523776,8433664,8540160,8450048,8536064,8548352,8531968,8585216]`.

**Release repeat 1 — Private/WS `unstable/growth`:**

- Private: stable/terminal `false/true`; baseline `2437120/2281472/2314240/2437120/2441216/2486272`; final `3751936/3731456/3751936/3784704/3813376/3952640`; peak `4169728`; growth `0.5529411764705883`; spans warm `0.052100840336134456/0.08403361344537816`, measured `0.016233766233766232/0.05844155844155844`; W `[1896448,2203648,2457600,2572288,2301952,2486272,2441216,2281472,2314240,2437120]`; M `[2478080,2486272,2596864,4169728,2215936,3813376,3952640,3731456,3784704,3751936]`.
- WS: stable/terminal `true/true`; baseline `5390336/5373952/5378048/5390336/5398528/5402624`; final `5697536/5582848/5586944/5697536/5701632/5713920`; peak `6889472`; growth/instant `0.056990881458966566/0.056990881458966566`; spans warm `0.003799392097264438/0.005319148936170213`, measured `0.020129403306973402/0.023005032350826744`; W `[5230592,5283840,5361664,5373952,5382144,5402624,5373952,5378048,5398528,5390336]`; M `[5410816,5402624,5402624,5734400,5390336,5701632,5586944,5582848,5713920,5697536]`.

**Release repeat 2 — Private/WS `unstable/pass`:**

- Private: stable/terminal `false/false`; baseline `3739648/2301952/2314240/3715072/3719168/3739648`; final `4190208/2355200/3846144/4149248/4190208/4218880`; peak `4329472`; growth `0.11686879823594266`; spans warm `0.37816979051819183/0.38699007717750827`, measured `0.08292201382033564/0.4491609081934847`; W `[2420736,2220032,4329472,3784704,3956736,2301952,2314240,3719168,3715072,3739648]`; M `[4001792,4005888,2310144,2379776,3944448,4149248,4218880,3846144,2355200,4190208]`.
- WS: stable/terminal `true/false`; baseline `5578752/5414912/5427200/5574656/5578752/5578752`; final `5746688/5472256/5746688/5754880/5754880/5758976`; peak `6930432`; growth/instant `0.032329169728141073/0.03010279001468429`; spans warm `0.02718589272593681/0.029390154298310066`, measured `0.001423487544483986/0.0498220640569395`; W `[5427200,5398528,5722112,5599232,5713920,5414912,5427200,5574656,5578752,5578752]`; M `[5709824,5726208,5455872,5480448,5730304,5754880,5758976,5754880,5472256,5746688]`.

## Godot Smoke

| Verificação posterior ao snapshot | Debug | Release |
| --- | --- | --- |
| Projeto | 24/24, 0 falhas, `203.56 s` | 24/24, 0 falhas, `15.40 s` |
| Upstream limpo | 20/20, `9.81 s` | 20/20, `0.92 s` |
| Headless API | exit 0, log limpo | exit 0, log limpo |
| Renderers | Vulkan Forward Mobile + OpenGL Compatibility, filmes novos | Vulkan Forward Mobile + OpenGL Compatibility, filmes novos |

A verificação completa foi executada depois da criação do snapshot e está em `artifacts/physics/review-final2-{debug,release}-gate.log`. Esses logs confirmam o código e o vínculo, mas não alteram os snapshots ou seus hashes.

Evidência visual auditada: `artifacts/physics/foundation-release-300.avi`, 300 frames/5 s, SHA-256 `DB4FB24628447CA0096A7E0C3C8EB17F943AFCC5AD7BCF257A5B1634A826BDBB`. Frames inspecionados: launch 2 `841538...`, impact 22 com `-vsync 0` `BCF084...`, settled 280 `E1E46D...`. A cena usa somente presentation nodes; gameplay transforms vêm de Box3D.

## Known Limits

- Box3D 3D `v0.1.0` permanece alfa.
- PrivateUsage/Working Set medem footprint/residência, não ownership. A alternância `pass/growth/unstable` mantém o budget de 5% não qualificado e diferido.
- O gate futuro de 5% e a meta p95 de 8 ms exigem Godot Release empacotado em hardware de referência.
- O scan inicial `--headless --editor` de godot-cpp ainda pode falhar nesta instalação; runtime por manifest determinístico permanece o gate válido.
- Extração individual do writer MJPEG deve usar `-vsync 0` para evitar duplicação CFR.
- Windows x86_64 é o único alvo desta fundação.
- `artifacts/physics/box3d-spike-{debug,release}.json` é volátil por definição. Somente os caminhos e hashes `Evidence-*` no topo são a evidência normativa deste relatório.

## Recommendation

Os snapshots têm zero crash, timeout, não finito, handle inválido, quebra funcional, `/MD`, desequilíbrio Box3/CRT ou divergência de hash. O warning permanente é `private_commit_budget_unqualified`; `budget_qualification.status=deferred`, target futuro `0.05`.

Recommendation: prosseguir_com_limites
