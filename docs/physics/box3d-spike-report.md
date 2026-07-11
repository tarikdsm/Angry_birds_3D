# Box3D Foundation Spike Report

Este relatório registra somente evidência nova produzida em 10 de julho de 2026 pelo gate final da fundação. Todos os comandos foram executados em processos completos, sem escolher amostras favoráveis. Os JSONs de Debug e Release têm zero violações normativas, oito linhas funcionais aprovadas e hashes iguais em seus dois repeats.

## Versions

| Componente | Versão/pin | Caminho usado |
| --- | --- | --- |
| Box3D 3D | `v0.1.0`, commit `8441b4a06d6d09dcfb0b0f704df4d847d1437b92` | `.fetchcontent-cache/box3d-src` (checkout limpo) |
| Godot | `4.5.1-stable`, commit `f62fdbde15035c5576dad93e586201f4d41ef0cb` | `.tools/godot/Godot_v4.5.1-stable_win64.exe` |
| godot-cpp | `godot-4.5-stable`, commit `e83fd0904c13356ed1d4c3d09f8bb9132bdc6b77` | `.fetchcontent-cache/godot_cpp-src` |
| Visual Studio Build Tools | `18.7.3`, build `11925.98` | `C:/Program Files (x86)/Microsoft Visual Studio/18/BuildTools` |
| MSVC | tools `14.44.35207`, compiler `19.44.35228.0`, x64 | `VC/Tools/MSVC/14.44.35207` |
| Windows SDK | target `10.0.26100.0` | `C:/Program Files (x86)/Windows Kits/10` |
| CMake | `4.3.3` | `.tools/cmake/cmake-4.3.3-windows-x86_64/bin/cmake.exe` |
| Ninja | `1.13.2` | `.tools/ninja/ninja.exe` |
| Python | `3.11.9` | versão mínima `3.11.0` |

O bootstrap retornou `ok=true`. O projeto e o upstream usaram generator Ninja, C17/C++20 conforme o alvo, SDK `10.0.26100.0`, biblioteca estática, `/MTd` em Debug e `/MT` em Release. O upstream foi configurado separadamente em `build/upstream-box3d/debug|release`, com `BOX3D_SAMPLES=OFF`, `BOX3D_BENCHMARKS=OFF`, `BOX3D_DOCS=OFF`, `BOX3D_BUILD_SHADERS=OFF` e `BOX3D_UNIT_TESTS=ON`.

## Capability Matrix

As linhas abaixo foram iguais em Debug e Release. `status` e `functional_status` foram `pass`; nenhum fallback foi ativado.

| Capacidade | Resultado medido | Fallback permitido/usado |
| --- | --- | --- |
| `ccd_dynamic_dynamic` | 20/20 seeds a 35 m/s e 4 substeps; 0 estados inválidos | 30 m/s e 6 substeps / não usado |
| `shape_cast_overlap` | cast de 3 m; primeiro handle `1`; overlap concordante; fraction `0.81077587604522705` | raycasts múltiplos somente para previsão / não usado |
| `contact_hit_events` | aproximação `15 m/s`; massa efetiva `41.887905120849609 kg`; energia `4105.0146484375 J`; normal `1`; materiais `111/222`; 1 par único em 6 substeps | energia derivada dos corpos / não usado |
| `joint_force_torque` | força máxima `12693.1640625 N`, acima do limiar `10000 N`, tolerância monotônica `50 N`; deformação máxima `0.01363062858581543 m` | deformação/impulso por dois ticks / não usado |
| `hulls_compounds` | 8 hulls; massa `42.666671752929688 kg` para esperada `42.666666666666664`; AABB `[-1.6200000048, 2.7799999714, -0.2199999988]..[1.6200000048, 3.2200000286, 0.2199999988]`, tolerância `1e-4`; contato e estado válidos; Box3 `0 -> 0` | shapes múltiplas no mesmo body / não usado |
| `radial_sleep` | sleep `1.0`; p95 linear/angular `0/0`; penetração `0.0015351474285125732 m`; crescimento de energia `0` | omitir força em bodies asleep / não usado |
| `batch_lifecycle` | 10.000 gerações; 0 handles inválidos; Box3 `0 -> 0`; Debug CRT 4 deltas `0`; fixture PrivateUsage Debug `4935680 -> 4935680`, Release `2428928 -> 2428928` | reduzir batch por tick / não usado |
| `upstream_replay` | replay oficial salvo, carregado, validado e removido; `6982` bytes; Box3 `0 -> 0` | replay próprio / não usado |

O hash canônico da matriz foi `17270965158708886116` nos dois repeats de ambos os builds. O estado funcional, não timing/footprint/allocator, é a entrada do hash.

## Scenario Metrics

### Resultado funcional comum aos dois builds

| Cenário | Ticks | Métricas funcionais completas |
| --- | ---: | --- |
| `radial_fall` | 600 | separação `-0.00005951523780822754 m`; velocidade linear/angular final `0/0`; crescimento máximo de energia `0`; janela `120 ticks`; epsilon `1e-9 J` |
| `projectile_pile` | 11 | primary `20/20`; fallback `0` não avaliado; projétil `35 m/s`; estados inválidos primary/fallback `0/0` |
| `radial_pile` | 1800 | sleep `1`; p95 linear/angular `0/0`; penetração total/plataforma/par `0.0015351474285125732 / 0.00030809640884399414 / 0.0015351474285125732 m`; densidades `480..520 kg/m3`; energia no tick 600 e growth `0/0` |
| `mass_ratio` | 1800 | sleep `1`; p95 linear/angular `0/0`; penetração total/plataforma/par `0.014884665608406067 / 0.004117727279663086 / 0.014884665608406067 m`; densidades `85..3400 kg/m3`; energia no tick 600 e growth `0/0` |
| `stress` | 15000 | 500 bodies, 800 shapes, 250 joints; 10 warmups + 10 ciclos medidos; 4 substeps; target diagnóstico Private/WS growth e trimmed span `0.05`; 10 ciclos completos; zero violações |
| `capability_matrix` | 21800 | 8 pass, 0 fallback, 0 blocked; zero violações |

### Timing, picos e determinismo

Tempos são `min/p50/p95/max` em milissegundos e são informativos nesta máquina, não gates do hardware de referência.

| Build/cenário | Step ms | Bodies/shapes/joints | Awake/contacts pico | Hashes repeats 1/2 |
| --- | --- | --- | --- | --- |
| Debug `radial_fall` | `0.0015 / 0.0016 / 0.0566 / 0.2902` | `0/0/0` após remoção | `1/1` | `12100112409900625846 / 12100112409900625846` |
| Debug `projectile_pile` | `4.2806 / 4.3539 / 14.7583 / 14.7583` | `121/123/0` | `121/221` | `1431096453509785832 / 1431096453509785832` |
| Debug `radial_pile` | `0.0258 / 0.0276 / 0.0597 / 18.0348` | `80/81/0` | `80/204` | `10363635776067367757 / 10363635776067367757` |
| Debug `mass_ratio` | `0.0256 / 0.0287 / 0.1058 / 18.6551` | `80/81/0` | `80/227` | `17464060736204574665 / 17464060736204574665` |
| Debug `stress` | `0.4322 / 0.4726 / 1.3765 / 2.7016` | `500/800/250` | `500/0` | `10292935394449293550 / 10292935394449293550` |
| Debug `capability_matrix` | `0.0257 / 0.0379 / 0.0802 / 18.5556` | `0/0/0` agregado | `80/204` | `17270965158708886116 / 17270965158708886116` |
| Release `radial_fall` | `0.0002 / 0.0003 / 0.0041 / 0.0839` | `0/0/0` após remoção | `1/1` | `12100112409900625846 / 12100112409900625846` |
| Release `projectile_pile` | `0.2355 / 0.2594 / 1.2795 / 1.2795` | `121/123/0` | `121/221` | `1431096453509785832 / 1431096453509785832` |
| Release `radial_pile` | `0.0024 / 0.0026 / 0.0030 / 1.4647` | `80/81/0` | `80/204` | `10363635776067367757 / 10363635776067367757` |
| Release `mass_ratio` | `0.0024 / 0.0026 / 0.0052 / 1.4646` | `80/81/0` | `80/227` | `17464060736204574665 / 17464060736204574665` |
| Release `stress` | `0.0442 / 0.0465 / 0.0797 / 0.3599` | `500/800/250` | `500/0` | `10292935394449293550 / 10292935394449293550` |
| Release `capability_matrix` | `0.0023 / 0.0025 / 0.0079 / 1.2963` | `0/0/0` agregado | `80/204` | `17270965158708886116 / 17270965158708886116` |

### Ownership normativo

- `process_box3d_allocator` foi `baseline=0`, `final=0`, `max_abs_delta=0`, `exact_return=true` em Debug e Release.
- Nos 12 repeat observations por build (6 cenários x 2), Box3 teve `baseline=0`, `final=0`, `max_abs_delta=0`, `exact_return=true`.
- Nos dois repeats de Stress de ambos os builds, `warmup_post_teardown` e `measured_post_teardown` foram exatamente `[0,0,0,0,0,0,0,0,0,0]`.
- Debug `/MTd`, repeats Stress 1 e 2: CRT `applicable=true`, `balanced=true`; `_NORMAL_BLOCK` count/bytes `0/0`; `_CLIENT_BLOCK` count/bytes `0/0`.
- Release `/MT`, repeats Stress 1 e 2: CRT `applicable=false`, `balanced=true`; campos de delta permanecem `0/0/0/0` e não são usados como gate.

### Footprint diagnóstico 10+10

Cada repeat abaixo emitiu `private_commit_budget_unqualified`, `gate_status=diagnostic`, `gate_applied=false`, `budget_qualified=false` e `budget_scope=future_packaged_reference_hardware`. `growth`, `unstable` e `pass` não alteraram exit, matriz ou hash.

**Debug repeat 1 — assessment Private `growth`; Working Set `growth`:**

- Private: available `true`, stable `false`, terminal `true`; baseline last/full-min/central-min/median/central-max/full-max `4825088/4628480/4628480/4825088/4853760/4907008`; final `6746112/4591616/5849088/6746112/6774784/7372800`; peak `7372800`; growth `0.39813242784380304`; warmup span trimmed/full `0.046689303904923603/0.057724957555178265`; measured `0.13721918639951428/0.41226472374013357`; W `[4890624,5038080,4661248,4902912,4636672,4907008,4628480,4853760,4628480,4825088]`; M `[4882432,4640768,4853760,6402048,4853760,4591616,5849088,7372800,6774784,6746112]`.
- Working Set: available `true`, stable `false`, terminal `true`; baseline `7430144/7401472/7401472/7430144/7458816/7462912`; final `8437760/7368704/7766016/8425472/8425472/8437760`; peak `10002432`; growth/instant `0.13395810363836824/0.13561190738699008`; warmup span trimmed/full `0.007717750826901874/0.008269018743109152`; measured `0.07826932425862908/0.12688381137579`; W `[7471104,7458816,7417856,7475200,7409664,7462912,7401472,7458816,7401472,7430144]`; M `[7483392,7417856,7454720,8372224,7454720,7368704,7766016,8425472,8425472,8437760]`.

**Debug repeat 2 — assessment Private `pass`; Working Set `pass`:**

- Private: available/stable `true/true`, terminal `false`; baseline `7372800/6832128/7204864/7372800/7372800/7401472`; final `6901760/6590464/6758400/6901760/6901760/6914048`; peak `7401472`; growth `0`; warmup span trimmed/full `0.02277777777777778/0.07722222222222222`; measured `0.020771513353115726/0.046884272997032642`; W `[5005312,7372800,6832128,7204864,6819840,7372800,6832128,7204864,7401472,7372800]`; M `[6590464,6901760,6758400,6914048,6590464,6901760,6758400,6914048,6590464,6901760]`.
- Working Set: available/stable `true/true`, terminal `false`; baseline `8470528/8425472/8433664/8433664/8437760/8470528`; final `8458240/8445952/8450048/8454144/8458240/8458240`; peak `10002432`; growth/instant `0.0024283632831471587/0`; warmup span trimmed/full `0.00048567265662943174/0.005342399222923749`; measured `0.0009689922480620155/0.0014534883720930232`; W `[7548928,8433664,8433664,8433664,8437760,8437760,8433664,8433664,8425472,8470528]`; M `[8445952,8458240,8450048,8454144,8445952,8458240,8450048,8454144,8445952,8458240]`.

**Release repeat 1 — assessment Private `growth`; Working Set `growth`:**

- Private: available/stable `true/true`, terminal `true`; baseline `2252800/2252800/2260992/2310144/2363392/2502656`; final `3780608/2371584/3751936/3780608/3792896/3837952`; peak `3837952`; growth `0.6365248226950354`; warmup span trimmed/full `0.044326241134751775/0.10815602836879433`; measured `0.010834236186348862/0.38786565547128926`; W `[1896448,1896448,2203648,2293760,2445312,2310144,2502656,2260992,2363392,2252800]`; M `[2564096,2273280,3756032,3493888,2531328,3837952,3792896,2371584,3751936,3780608]`.
- Working Set: available/stable `true/true`, terminal `true`; baseline `5447680/5443584/5447680/5451776/5472256/5484544`; final `5779456/5505024/5668864/5775360/5779456/5787648`; peak `6950912`; growth/instant `0.059353869271224644/0.06090225563909774`; warmup span trimmed/full `0.004507888805409466/0.007513148009015778`; measured `0.019148936170212766/0.04893617021276596`; W `[5251072,5251072,5353472,5357568,5451776,5451776,5484544,5443584,5472256,5447680]`; M `[5509120,5472256,5775360,5656576,5496832,5668864,5775360,5505024,5787648,5779456]`.

**Release repeat 2 — assessment Private `unstable`; Working Set `pass`:**

- Private: available `true`, stable `false`, terminal `false`; baseline `3760128/2441216/3760128/4182016/4198400/4689920`; final `2633728/2633728/2699264/4112384/4120576/4182016`; peak `4694016`; growth `0`; warmup span trimmed/full `0.10479921645445642/0.5377081292850147`; measured `0.3456175298804781/0.37649402390438247`; W `[4120576,2383872,4116480,4694016,4493312,4198400,2441216,4182016,4689920,3760128]`; M `[4124672,4673536,4165632,4120576,4308992,4120576,4112384,4182016,2699264,2633728]`.
- Working Set: available/stable `true/true`, terminal `false`; baseline `5844992/5550080/5832704/5844992/5844992/5877760`; final `5586944/5586944/5591040/5853184/5877760/5902336`; peak `7069696`; growth/instant `0.001401541695865452/0`; warmup span trimmed/full `0.0021023125437981782/0.05606166783461808`; measured `0.0489853044086774/0.053883834849545134`; W `[5808128,5402624,5779456,5787648,5795840,5877760,5550080,5832704,5844992,5844992]`; M `[5890048,5873664,5885952,5861376,5885952,5902336,5877760,5853184,5591040,5586944]`.

## Godot Smoke

| Gate | Debug | Release |
| --- | --- | --- |
| Suíte do projeto | 24/24, 0 falhas, `201.62 s` | 24/24, 0 falhas, `15.41 s` |
| Upstream Box3D via CTest build-and-test | 20/20, `10.11 s` | 20/20, `0.92 s` |
| Headless API | exit 0; log sem warning/error | exit 0; log sem warning/error |
| Vulkan | `Vulkan 1.4.312`, Forward Mobile, RTX 3070 Laptop GPU; AVI novo | mesmo renderer/device; AVI novo |
| OpenGL | `OpenGL 3.3.0 NVIDIA 581.95`, Compatibility; AVI novo | mesmo renderer/device; AVI novo |

Logs e filmes curtos: `artifacts/physics/foundation-final-{debug,release}-gate.log`, `godot-smoke-*.stdout.log`, `godot-scene-*.avi`, `godot-scene-gl-*.avi`. O filme Release de inspeção contém 300 frames/5 s em `artifacts/physics/foundation-release-300.avi` (SHA-256 `DB4FB24628447CA0096A7E0C3C8EB17F943AFCC5AD7BCF257A5B1634A826BDBB`).

Capturas extraídas com seleção sem duplicação (`-vsync 0`) e inspecionadas em resolução 1280x720:

- launch frame 2: `artifacts/physics/frames/foundation-release/launch-n0002.png`, SHA-256 `8415386162B64BFE2A525F7F7E219A9EF3B29B12798B014C6435733F0B60932A`;
- impact frame 22: `artifacts/physics/frames/foundation-release/impact-n0022-vsync0.png`, SHA-256 `BCF084D671689F908529D07E9D39A3291967C1E4BD136908FAC2A9B16B16D59A`;
- settled frame 280: `artifacts/physics/frames/foundation-release/settled-n0280.png`, SHA-256 `E1E46D32B2930129CED9D725C05FC4569FD1E280FE7B3F4C8A4400867A2FE7DB`.

As imagens mostram, respectivamente, projétil antes do contato, resposta estrutural no impacto e pilha acomodada radialmente. A cena contém somente presentation nodes; a autoridade de transform é Box3D.

## Known Limits

- Box3D 3D `v0.1.0` é alfa. O marco aprova a fundação técnica, não produção sem limites.
- PrivateUsage e Working Set são footprint/residência do processo, não ownership do allocator. As amostras acima alternaram entre `pass`, `growth` e `unstable`; portanto o budget de 5% não foi qualificado nesta máquina.
- O gate futuro de footprint exige Godot Release empacotado, hardware de referência e protocolo controlado. Nesta fundação ele é obrigatoriamente diagnóstico.
- A meta de p95 `8 ms` também só se torna normativa no hardware/protocolo de referência. O pico/p95 Debug do cenário de CCD não bloqueia esta rodada local.
- O primeiro scan `--headless --editor` de uma classe godot-cpp registrada ainda pode falhar nesta instalação Windows; runtime determinístico por `extension_list.cfg` foi limpo em Debug/Release. Este é limite de ambiente/editor, não falha do kernel.
- O writer MJPEG do Godot pode produzir regiões antigas/escuras quando a extração usa CFR implícito. A captura auditada usa seleção `-vsync 0`; os 300 frames decodificam e a física não depende do writer.
- Windows x86_64 é o único alvo deste marco. VMs limpas Windows 10/11 e pacote final pertencem ao gate de entrega.
- Artefatos reprodutíveis preservados para este relatório são ignorados pelo Git: `artifacts/physics/foundation-report-debug.json` (SHA-256 `85923C308DF37050645C56D7D11173329C29F88ED5968080748287BF9621D5B2`) e `foundation-report-release.json` (SHA-256 `AA9F8ECA1B81DAFFB06FDF873407ABBA6BCB5E186226AB6372BAEC6992ED2966`).

## Recommendation

Não houve crash, timeout, estado não finito, handle inválido, quebra funcional, configuração `/MD`, desequilíbrio Box3/CRT, divergência de hash ou falha de integração. O warning permanente é `private_commit_budget_unqualified`; `budget_qualification.status=deferred`, target futuro `0.05`.

Recommendation: prosseguir_com_limites
