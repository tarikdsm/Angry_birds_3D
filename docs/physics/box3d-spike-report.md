# Box3D Foundation Spike Report

Este documento descreve os dois snapshots auditados e rastreados da fundacao Box3D. Ele nao representa o JSON volatil mais recente em `artifacts/physics/`. Os valores narrativos abaixo foram reconstruidos integralmente a partir dos snapshots Debug e Release atuais.

Evidence-Debug-Path: docs/physics/evidence/foundation-report-debug.json
Evidence-Debug-SHA256: FD87A8BE9B882CD7BB0F58BE2311196456C8C76943C1B347D1B0F534A35385C0
Evidence-Release-Path: docs/physics/evidence/foundation-report-release.json
Evidence-Release-SHA256: BB517B2550E4A53EAF68723D4E78D4A15BB7FF231F1206C9F72E6358AA9BF5B2
Matrix-Debug-Hash: 17104053157009575930
Matrix-Debug-Topology: 123/123/1;121/221
Recommendation-Debug: prosseguir_com_limites
Matrix-Release-Hash: 17104053157009575930
Matrix-Release-Topology: 123/123/1;121/221
Recommendation-Release: prosseguir_com_limites

Cada snapshot contem seis cenarios, dois repeats por cenario, oito capacidades aprovadas, zero violacao normativa, warning `private_commit_budget_unqualified` e recomendacao `prosseguir_com_limites`. A topologia codificada nos tokens `Matrix-*-Topology` usa `peak bodies/shapes/joints;peak awake/contacts`.

## Versions

| Componente | Versao/pin | Caminho usado |
| --- | --- | --- |
| Box3D 3D | `v0.1.0`, commit `8441b4a06d6d09dcfb0b0f704df4d847d1437b92` | `.fetchcontent-cache/box3d-src` limpo |
| Godot | `4.5.1-stable`, commit `f62fdbde15035c5576dad93e586201f4d41ef0cb` | `.tools/godot/Godot_v4.5.1-stable_win64.exe` |
| godot-cpp | `godot-4.5-stable`, commit `e83fd0904c13356ed1d4c3d09f8bb9132bdc6b77` | `.fetchcontent-cache/godot_cpp-src` |
| VS Build Tools | `18.7.3`, build `11925.98` | `C:/Program Files (x86)/Microsoft Visual Studio/18/BuildTools` |
| MSVC/SDK | tools `14.44.35207`, compiler `19.44.35228.0`; SDK `10.0.26100.0` | x64, `/MTd` Debug, `/MT` Release |
| CMake/Ninja/Python | `4.3.3` / `1.13.2` / `3.11.9` | toolchain portatil pinada |

Configuracao comum dos snapshots: seed `1`, `4` substeps, `2` repeats e passo `1/60 s`. O upstream e reconfigurado do zero em `build/upstream-box3d/debug|release`; todos os comandos Box3D/shared/test sao MSVC `cl.exe`, usam o CRT estatico da configuracao e rejeitam `/MD[d]`, `/fp:fast` e contaminacao entre Debug e Release.

## Capability Matrix

As oito linhas possuem `status=pass`, `functional_status=pass`, `fallback=null` e `functional_fallback=null` em ambos os snapshots. Os picos agregados da matriz sao `123/123/1` bodies/shapes/joints e `121/221` awake/contacts. O joint deixa de ser invisivel no agregado e o pico global vem do maximo medido entre todas as provas, nao apenas de `radial_sleep`.

| Capacidade | Pico body/shape/joint | Awake/contact | Prova medida | Fixture hash(es) |
| --- | --- | --- | --- | --- |
| `ccd_dynamic_dynamic` | `123/123/0` | `121/221` | 20/20 seeds primarias a `35 m/s`, 4 substeps, 0 estados invalidos; fallback `30 m/s`, 6 substeps nao avaliado | `1431096453509785832,5466060815846980033,8526278817898170285,2675255426310783444,10494942409035688541,7390650343477109358,14725012844580521641,14001126938762718496,2400058324701230849,10732887959698602526,5915366692680379584,14334879908416984375,6577404406665493359,16513243213841685896,9721495563768596328,13937513265211812359,9628435969084559192,18192646134465658732,18426867884952182109,11807578939564631579` |
| `shape_cast_overlap` | `1/1/0` | `0/0` | cast `3 m`, handle `1`, overlap concordante, fraction `0.810775876045227` | `8388802905423810155` |
| `contact_hit_events` | `2/2/0` | `2/1` | `15 m/s`, massa efetiva `41.88790512084961 kg`, energia `4105.0146484375 J`, normal `1`, materiais `111/222`, 1 par, 6 substeps | `10155543446163919611` |
| `joint_force_torque` | `2/2/1` | `1/1` | tolerancia monotona `50 N`; maximo `12693.1640625 N` cruza `10000 N`; deformacao maxima `0.01363062858581543 m`, threshold `0.01 m`, consecutive ticks `0` | `14239377403397705414` |
| `hulls_compounds` | `2/9/0` | `1/8` | 8 hulls; massa esperada/medida `42.666666666666664/42.66667175292969 kg`; bounds `[-1.6200000047683716,2.7799999713897705,-0.2199999988079071]..[1.6200000047683716,3.2200000286102295,0.2199999988079071]`, tolerancia `0.0001 m`; contato/estado `1/1`; Box3 `0 -> 0` | `7710058609812386530` |
| `radial_sleep` | `81/81/0` | `80/204` | sleep `1`, p95 linear/angular `0/0`, penetracao `0.0015351474285125732 m`, energy growth `0` | `10363635776067367757` |
| `batch_lifecycle` | `1/1/0` | `1/0` | 10.000 geracoes, 0 handles invalidos, crescimento PrivateUsage `0`, Box3 `0 -> 0`, quatro deltas CRT zero | `43224550866945` |
| `upstream_replay` | `1/1/0` | `1/0` | save/load/validate/remove, `6982` bytes, Box3 `0 -> 0` | `13184605773762244167` |

Os hashes canonicos da matriz, dos cenarios e das fixtures sao saidas pinadas de replay para seed `1` e dependencias fixadas. Eles sao comparados exatamente pelos gates, mas nao sao alegados como hashes criptograficamente rederivados do JSON. Topologia, ownership, memoria, CRT, status e limites derivaveis sao validados separadamente.

## Scenario Metrics

`Fixture dyn/shape/joint` representa a fixture declarada; `Pico body/shape/joint` e `Awake/contact` sao maximos observados. Cada repeat repete tanto o hash quanto os cinco picos do cenario. Tempos sao `min/p50/p95/max` em milissegundos e sao informativos nesta maquina.

| Build/cenario | Ticks | Fixture dyn/shape/joint | Pico body/shape/joint | Awake/contact | Step ms | Hash repeat 1 / repeat 2 |
| --- | ---: | --- | --- | --- | --- | --- |
| Debug `radial_fall` | 600 | `1/2/0` | `2/2/0` | `1/1` | `0.0015/0.0016/0.0591/0.4254` | `12100112409900625846/12100112409900625846` |
| Debug `projectile_pile` | 11 | `121/123/0` | `123/123/0` | `121/221` | `4.3134/4.3887/15.0769/15.0769` | `1431096453509785832/1431096453509785832` |
| Debug `radial_pile` | 1800 | `80/81/0` | `81/81/0` | `80/204` | `0.0256/0.0278/0.0762/17.5334` | `10363635776067367757/10363635776067367757` |
| Debug `mass_ratio` | 1800 | `80/81/0` | `81/81/0` | `80/227` | `0.0257/0.0349/0.0968/28.9355` | `17464060736204574665/17464060736204574665` |
| Debug `stress` | 15000 | `500/800/250` | `500/800/250` | `500/0` | `0.4241/0.4853/1.3612/2.9017` | `10292935394449293550/10292935394449293550` |
| Debug `capability_matrix` | 21800 | `0/0/0` | `123/123/1` | `121/221` | `0.0257/0.0282/0.1011/36.2721` | `17104053157009575930/17104053157009575930` |
| Release `radial_fall` | 600 | `1/2/0` | `2/2/0` | `1/1` | `0.0002/0.0003/0.0041/0.1596` | `12100112409900625846/12100112409900625846` |
| Release `projectile_pile` | 11 | `121/123/0` | `123/123/0` | `121/221` | `0.2407/0.2427/1.2817/1.2817` | `1431096453509785832/1431096453509785832` |
| Release `radial_pile` | 1800 | `80/81/0` | `81/81/0` | `80/204` | `0.0024/0.0025/0.0028/1.4872` | `10363635776067367757/10363635776067367757` |
| Release `mass_ratio` | 1800 | `80/81/0` | `81/81/0` | `80/227` | `0.0024/0.0026/0.0055/1.389` | `17464060736204574665/17464060736204574665` |
| Release `stress` | 15000 | `500/800/250` | `500/800/250` | `500/0` | `0.0447/0.0462/0.1026/0.3957` | `10292935394449293550/10292935394449293550` |
| Release `capability_matrix` | 21800 | `0/0/0` | `123/123/1` | `121/221` | `0.0023/0.0025/0.0104/1.2893` | `17104053157009575930/17104053157009575930` |

Inventario completo das metricas funcionais comuns aos dois builds:

- `radial_fall`: surface separation `-0.00005951523780822754 m`, velocidades finais linear/angular `0/0`, energy growth `0`, janela `120 ticks`, epsilon `1e-9 J`.
- `projectile_pile`: primary/fallback passes `20/0`, velocidade `35 m/s`, estados invalidos primary/fallback `0/0`, fallback evaluated `0`.
- `radial_pile`: sleep `1`, p95 linear/angular `0/0`, penetracao max/platform/pair `0.0015351474285125732/0.00030809640884399414/0.0015351474285125732 m`, densidades `480..520 kg/m3`, energia no tick 600 `0 J`, growth `0`.
- `mass_ratio`: sleep `1`, p95 linear/angular `0/0`, penetracao max/platform/pair `0.014884665608406067/0.004117727279663086/0.014884665608406067 m`, densidades `85..3400 kg/m3`, energia no tick 600 `0 J`, growth `0`.
- `stress`: targets diagnosticos PrivateUsage growth/span e Working Set growth/span `0.05`; executed substeps `4`, allocator warmups `10`, completed cycles `10`. As doze metricas duplicadas de footprint sao enumeradas por build/repeat abaixo e cruzadas pelo gate com `scenario.memory` e o primeiro repeat.

## Ownership, allocator e CRT

- Process allocator Debug e Release: baseline/final/max delta `0/0/0`, `exact_return=true`.
- Todas as 24 observacoes de repeat: Box3 baseline/final/max delta `0/0/0`, `exact_return=true`.
- Nos quatro repeats de `stress`, `warmup_post_teardown` e `measured_post_teardown` sao arrays de dez zeros cada.
- Debug `stress` repeats 1 e 2: CRT `applicable=true`, `balanced=true`; deltas `_NORMAL_BLOCK` count/bytes `0/0` e `_CLIENT_BLOCK` count/bytes `0/0`.
- Release `stress` repeats 1 e 2: CRT `applicable=false`, `balanced=true`, com os quatro deltas em zero. `/MT` e verificado no banco de compilacao completo.
- `batch_lifecycle` tambem registra os quatro deltas CRT em zero. PrivateUsage baseline/final e `5160960/5160960` no Debug e `2433024/2433024` no Release; Working Set baseline/final e `7815168/7819264` no Debug e `5513216/5513216` no Release.

## Footprint diagnostico 10+10

Todos os repeats de `stress` usam `gate_scope=release_mt`, `gate_status=diagnostic`, `gate_applied=false`, `budget_qualified=false` e `budget_scope=future_packaged_reference_hardware`. O warning e `private_commit_budget_unqualified`; `budget_qualification.status=deferred` e o target futuro e `0.05`. PrivateUsage e Working Set nao alteram exit code, hash ou status funcional.

Os campos `baseline` e `final` abaixo seguem a ordem `last/full_min/central_min/median/central_max/full_max`; `spans` seguem `warmup_trimmed/warmup_full/measured_trimmed/measured_full`.

### Debug repeat 1

- PrivateUsage: assessment `growth`; available/stable/terminal `true/false/true`; baseline `4796416/4796416/5087232/5181440/5214208/6324224`; final `6365184/5107712/6365184/7077888/7323648/7348224`; peak `7348224`; growth `0.3660079051383399`; spans `0.02450592885375494/0.2948616600790514/0.13541666666666666/0.31655092592592593`; warmup `[5468160,5169152,5230592,5218304,4976640,5181440,5214208,5087232,6324224,4796416]`; measured `[6742016,5320704,5353472,7163904,6893568,7348224,5107712,7323648,7077888,6365184]`.
- Working Set: assessment `growth`; available/stable/terminal `true/true/true`; baseline `7503872/7503872/7524352/7524352/7557120/8413184`; final `8540160/7548928/8523776/8540160/8544256/8556544`; peak `10084352`; growth/instant `0.13500272182906914/0.1381004366812227`; spans `0.004354926510615134/0.12084921066956995/0.002398081534772182/0.11798561151079137`; warmup `[7561216,7544832,7565312,7540736,7561216,7524352,7557120,7524352,8413184,7503872]`; measured `[8404992,7573504,7606272,8617984,8531968,8544256,7548928,8556544,8523776,8540160]`.

### Debug repeat 2

- PrivateUsage: assessment `growth`; available/stable/terminal `true/true/true`; baseline `5566464/5541888/5566464/5767168/5767168/5824512`; final `6733824/5365760/6733824/6942720/6967296/7012352`; peak `7012352`; growth `0.20383522727272727`; spans `0.03480113636363636/0.049005681818181816/0.033628318584070796/0.23716814159292035`; warmup `[5623808,5406720,5455872,5312512,5906432,5824512,5767168,5541888,5767168,5566464]`; measured `[5001216,7000064,6639616,6979584,6758400,5365760,7012352,6942720,6967296,6733824]`.
- Working Set: assessment `growth`; available/stable/terminal `true/true/true`; baseline `8052736/7905280/7909376/7925760/7933952/8052736`; final `8765440/7753728/8749056/8765440/8777728/8790016`; peak `10448896`; growth/instant `0.10594315245478036/0.08850457782299084`; spans `0.0031007751937984496/0.018604651162790697/0.0032710280373831778/0.11822429906542056`; warmup `[7811072,7823360,7798784,7905280,7913472,7933952,7909376,7925760,7905280,8052736]`; measured `[7573504,8822784,8843264,8720384,8585216,7753728,8790016,8749056,8777728,8765440]`.

### Release repeat 1

- PrivateUsage: assessment `unstable`; available/stable/terminal `true/false/false`; baseline `2306048/2306048/2392064/2596864/2686976/2711552`; final `2510848/2510848/2592768/2670592/2842624/3981312`; peak `4100096`; growth `0.028391167192429023`; spans `0.11356466876971609/0.15615141955835962/0.09355828220858896/0.5506134969325154`; warmup `[2465792,2420736,2707456,2674688,2396160,2686976,2711552,2392064,2596864,2306048]`; measured `[2338816,2633728,2707456,4100096,2301952,2670592,2592768,3981312,2842624,2510848]`.
- Working Set: assessment `pass`; available/stable/terminal `true/true/false`; baseline `5476352/5476352/5480448/5484544/5496832/5513216`; final `5558272/5529600/5537792/5558272/5582848/5828608`; peak `6950912`; growth/instant `0.01344286781179985/0.014958863126402393`; spans `0.002987303958177745/0.006721433905899925/0.008106116433308769/0.05379513633014001`; warmup `[5402624,5398528,5488640,5484544,5496832,5480448,5496832,5484544,5513216,5476352]`; measured `[5509120,5525504,5529600,5808128,5500928,5529600,5537792,5828608,5582848,5558272]`.

### Release repeat 2

- PrivateUsage: assessment `growth`; available/stable/terminal `true/false/false`; baseline `2859008/2859008/3751936/3858432/3903488/4136960`; final `3796992/2736128/3796992/4214784/4341760/4358144`; peak `4358144`; growth `0.09235668789808917`; spans `0.03927813163481953/0.33121019108280253/0.1292517006802721/0.3848396501457726`; warmup `[3756032,3776512,3784704,2449408,2510848,3751936,3903488,3858432,4136960,2859008]`; measured `[2899968,3784704,4014080,3801088,2826240,4358144,4214784,4341760,2736128,3796992]`.
- Working Set: assessment `pass`; available/stable/terminal `true/true/false`; baseline `5599232/5599232/5771264/5771264/5791744/5918720`; final `5931008/5660672/5931008/5931008/5943296/5947392`; peak `7012352`; growth/instant `0.027679205110007096/0.0592538405267008`; spans `0.0035486160397444995/0.05535841022001419/0.0020718232044198894/0.04834254143646409`; warmup `[5787648,5771264,5783552,5566464,5611520,5771264,5918720,5771264,5791744,5599232]`; measured `[5644288,5767168,5799936,5808128,5652480,5931008,5943296,5947392,5660672,5931008]`.

## Godot Smoke

Os logs de gate e os smokes Godot sao evidencias posteriores e nao alteram os snapshots nem seus SHA-256.

| Verificacao | Debug | Release | Log exato |
| --- | --- | --- | --- |
| Gate de projeto final | 24/24, `206.67 s` | 24/24, `15.34 s` | `build/debug/Testing/Temporary/LastTest.log`; `build/release/Testing/Temporary/LastTest.log` |
| Upstream final | 20/20, `9.70 s` | 20/20, `0.92 s` | `artifacts/physics/upstream-box3d-debug.log`; `artifacts/physics/upstream-box3d-release.log` |
| Godot headless API | exit `0`, stderr vazio; o log nao registra duracao interna | exit `0`, stderr vazio; o log nao registra duracao interna | `artifacts/physics/godot-smoke-debug.stdout.log`, `artifacts/physics/godot-smoke-debug.stderr.log`; `artifacts/physics/godot-smoke-release.stdout.log`, `artifacts/physics/godot-smoke-release.stderr.log` |

Os smokes e renderers possuem arquivos separados:

- Vulkan Forward Mobile Debug: `artifacts/physics/godot-scene-debug.stdout.log` e `artifacts/physics/godot-scene-debug.stderr.log`; 15 frames, filme `0.25 s`, gravacao `1 s`, CPU `0.56 ms/frame`, GPU `0.27 ms/frame`, encoding `10.08 ms/frame`.
- Vulkan Forward Mobile Release: `artifacts/physics/godot-scene-release.stdout.log` e `artifacts/physics/godot-scene-release.stderr.log`; 15 frames, filme `0.25 s`, gravacao `1 s`, CPU `0.55 ms/frame`, GPU `0.26 ms/frame`, encoding `9.79 ms/frame`.
- OpenGL Compatibility Debug: `artifacts/physics/godot-scene-gl-debug.stdout.log` e `artifacts/physics/godot-scene-gl-debug.stderr.log`; 15 frames, filme `0.25 s`, gravacao `2 s`, CPU `0.74 ms/frame`, GPU `0.38 ms/frame`, encoding `9.50 ms/frame`.
- OpenGL Compatibility Release: `artifacts/physics/godot-scene-gl-release.stdout.log` e `artifacts/physics/godot-scene-gl-release.stderr.log`; 15 frames, filme `0.25 s`, gravacao `2 s`, CPU `0.77 ms/frame`, GPU `0.33 ms/frame`, encoding `9.62 ms/frame`.

Evidencia visual auditada anterior: `artifacts/physics/foundation-release-300.avi`, 300 frames/5 s, SHA-256 `DB4FB24628447CA0096A7E0C3C8EB17F943AFCC5AD7BCF257A5B1634A826BDBB`. A cena usa apenas presentation nodes; transforms de gameplay vem do Box3D.

## Known Limits

- Box3D 3D `v0.1.0` permanece alfa.
- PrivateUsage mede commit privado e Working Set mede residencia; nenhum dos dois prova ownership. A variacao `growth/unstable/pass` mantem o budget de 5% nao qualificado e diferido.
- O gate futuro de 5% e a meta p95 de 8 ms exigem Godot Release empacotado em hardware de referencia.
- O scan inicial `--headless --editor` de godot-cpp ainda pode falhar nesta instalacao; runtime por manifest deterministico permanece o gate valido.
- Extracao individual do writer MJPEG deve usar `-vsync 0` para evitar duplicacao CFR.
- Windows x86_64 e o unico alvo desta fundacao.
- `artifacts/physics/box3d-spike-{debug,release}.json` e volatil. Somente os caminhos e hashes `Evidence-*` no topo identificam a evidencia normativa deste relatorio.

## Recommendation

Os snapshots tem zero crash, timeout, nao finito, handle invalido, quebra funcional, `/MD`, desequilibrio Box3/CRT ou divergencia de hash/topologia entre repeats. O unico warning global e `private_commit_budget_unqualified`; `budget_qualification.status=deferred` e target futuro `0.05`.

Recommendation: prosseguir_com_limites
