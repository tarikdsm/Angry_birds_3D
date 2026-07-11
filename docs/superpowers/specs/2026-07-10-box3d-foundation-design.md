# Fundação Box3D — Especificação do Primeiro Marco

**Produto:** Ninho Orbital: Mundos Partidos

**Data:** 10 de julho de 2026

**Dependência:** [especificação-mestra](2026-07-10-ninho-orbital-design.md)
**Objetivo:** provar, com testes e uma cena visual mínima, que Box3D v0.1.0 pode sustentar o lançamento orbital, pilhas destrutíveis e a integração Godot no Windows antes da produção de conteúdo.

## 1. Resultado do marco

Ao final deste marco, o repositório deve produzir três artefatos:

1. `ninho_physics_tests.exe`, suíte headless para o kernel físico;
2. `ninho_physics_spike.exe`, executável de cenários que grava métricas e resultados em JSON;
3. uma DLL GDExtension carregada por um projeto Godot mínimo que mostra planeta, projétil e pilha usando somente transforms do Box3D.

Este marco não implementa menu final, aves especiais, inimigos, vinte materiais, fases finais nem arte de produção. Ele existe para retirar o maior risco técnico: usar um solver 3D alfa como autoridade física dentro de uma engine madura.

## 2. Decisões fixas

- Windows x86_64 é o único alvo do marco.
- Godot é fixado em `4.5.1-stable`, commit `f62fdbde15035c5576dad93e586201f4d41ef0cb`.
- `godot-cpp` é fixado em `godot-4.5-stable`, commit `e83fd0904c13356ed1d4c3d09f8bb9132bdc6b77`.
- Box3D é fixado em `v0.1.0`, commit `8441b4a06d6d09dcfb0b0f704df4d847d1437b92`.
- Box3D é compilado estaticamente e não é modificado.
- Visual Studio 2026 `18.7.3` build `11925.98` com MSVC `14.44`/v143 e Windows SDK target `10.0.26100.0` formam o compilador principal.
- CMake `4.3.3`, Ninja `1.13.2` e generator `Ninja` são fixos; Debug e Release usam diretórios separados para selecionar o target correspondente do godot-cpp.
- O kernel usa C++20; Box3D usa C17.
- Todos os alvos nativos usam CRT estático `/MTd` em Debug e `/MT` em Release, x64, sem `/fp:fast`.
- A simulação usa `1/60 s`, quatro substeps por padrão e seis como fallback já aprovado exclusivamente para CCD, gravidade global zero e força radial explícita.
- Single-thread é o padrão. O marco mede, mas não habilita, workers adicionais.
- O projeto não usa a física 3D do Godot para objetos de gameplay.

## 3. Estrutura de código

```text
/
├─ CMakeLists.txt
├─ CMakePresets.json
├─ cmake/
│  └─ Dependencies.cmake
├─ native/
│  ├─ kernel/
│  │  ├─ include/ninho/physics/
│  │  │  ├─ physics_types.hpp
│  │  │  ├─ physics_world.hpp
│  │  │  ├─ radial_gravity.hpp
│  │  │  └─ scenario.hpp
│  │  └─ src/
│  │     ├─ physics_world.cpp
│  │     ├─ radial_gravity.cpp
│  │     └─ scenario.cpp
│  ├─ spike/
│  │  └─ main.cpp
│  ├─ tests/
│  │  ├─ test_main.cpp
│  │  ├─ radial_gravity_tests.cpp
│  │  ├─ world_lifecycle_tests.cpp
│  │  ├─ projectile_ccd_tests.cpp
│  │  ├─ pile_stability_tests.cpp
│  │  └─ determinism_tests.cpp
│  └─ extension/
│     ├─ include/ninho/extension/box3d_world_node.hpp
│     └─ src/
│        ├─ box3d_world_node.cpp
│        └─ register_types.cpp
├─ game/
│  ├─ project.godot
│  ├─ bin/ninho_physics.gdextension
│  ├─ scenes/physics_spike.tscn
│  └─ scripts/physics_spike_view.gd
├─ tools/
│  ├─ bootstrap.ps1
│  ├─ build.ps1
│  ├─ test.ps1
│  └─ run_spike.ps1
└─ third_party/
   └─ README.md
```

Dependências são obtidas pelo build em diretório de cache ignorado pelo Git. Arquivos de configuração guardam URL, tag e SHA. `third_party/README.md` registra versões, licenças e como reproduzir o download, sem duplicar o código de terceiros no histórico principal.

## 4. API do kernel

O namespace público é `ninho::physics`. Sua API mínima contém:

- `BodyHandle { uint32_t index; uint32_t generation; }`;
- `Vec3`, `Quat` e `Transform` próprios, triviais e sem dependência de Godot ou Box3D;
- `WorldConfig` com timestep, substeps, raio, gravidade superficial e limites;
- `ShapeDesc` como variante de esfera, caixa, cápsula, hull convexo e compound de shapes, sempre com transform local;
- `BodyDesc` para tipo, transform, uma ou mais `ShapeDesc`, densidade, atrito, restituição e flags;
- `JointDesc` como variante mínima de junta de distância e weld, com limites/motor necessários ao spike;
- `BodyState` para handle, transform, velocidades e estado de sleep;
- `WorldMetrics` para corpos, shapes, contatos, corpos acordados e duração do step;
- `PhysicsWorld::create_body`, `destroy_body`, `create_joint`, `destroy_joint`, `apply_force`, `apply_impulse`, `cast_shape`, `overlap_shape`, `step`, `states`, `contact_hits`, `joint_reactions` e `metrics`;
- `ScenarioResult` com nome, seed, ticks, hash final, métricas e violações.

Handles do jogo mapeiam internamente para IDs do Box3D. Um handle inválido nunca acessa o solver; operações retornam `Status` ou valor opcional e registram erro com contexto. Destruição incrementa a geração para impedir uso acidental de um slot reciclado.

Todos os comandos que alteram o mundo durante uma simulação são enfileirados. `step()` aplica a fila antes da gravidade, avança Box3D, copia eventos e publica um snapshot contíguo. A API não devolve ponteiros internos nem referências a eventos transitórios. `ScenarioBuilder` constrói stress, hulls, compounds e juntas exclusivamente por essa API; os testes não podem acessar `b3*` diretamente, exceto um teste de conformidade isolado do adaptador.

## 5. Gravidade radial

`RadialGravity` calcula aceleração com a fórmula definida na especificação-mestra. O kernel usa massa do corpo para converter aceleração em força aplicada ao centro de massa. A origem exata é protegida por raio mínimo e direção nula; um corpo no centro não recebe vetor indefinido.

Testes verificam:

- direção para o centro nos eixos X, Y e Z;
- magnitude na superfície, em `2R`, em `0,6R` e no centro;
- vetor exatamente zero abaixo de 1 mm e ausência de limite inferior longe do planeta;
- invariância aproximada sob rotação da entrada;
- ausência de `NaN` e infinito;
- um corpo largado de `R + 5 m` aproxima-se do planeta durante 120 ticks;
- um lançamento tangencial produz deslocamento orbital sem sair imediatamente do limite do mundo.
- um corpo além de `4R`, saindo a pelo menos 2 m/s por 0,5 segundo, emite ejeção e é removido em `6R`.

## 6. Cenários de risco

### 6.1 Queda radial

Uma esfera dinâmica de raio `0,4 m` começa cinco metros acima de um planeta estático representado por uma esfera. Deve colidir, quicar conforme restituição e repousar. Depois de 600 ticks, a separação da superfície fica entre `-0,02 m` e `+0,03 m`, velocidade linear `<0,05 m/s`, angular `<0,10 rad/s` e energia cinética não cresce mais que 2% em qualquer janela posterior de 120 ticks.

### 6.2 Projétil rápido contra pilha

Um projétil `isBullet` de raio `0,45 m` parte a `35 m/s` contra uma torre de 120 blocos dinâmicos apoiada numa plataforma tangente ao planeta. O teste falha se o projétil atravessar toda a pilha sem contato, gerar estado inválido ou escapar por tunneling do cenário.

### 6.3 Pilha em gravidade radial

Uma torre de 80 blocos é simulada por 1.800 ticks. Nos últimos 300 ticks, pelo menos 90% dos corpos devem estar asleep, velocidade linear p95 `<0,05 m/s`, angular p95 `<0,10 rad/s`, penetração máxima estimada `<0,02 m` e energia cinética total não pode crescer mais que 2% sobre o valor no tick 600. `NaN`, infinito, body fora de `6R` sem ejeção válida ou velocidade espontânea `>100 m/s` são violações fatais.

### 6.4 Razão de massa

Uma pilha combina densidades de 85 a 3.400 kg/m³ nas dimensões típicas do jogo. Nos últimos 300 ticks, velocidade linear p95 deve ser `<0,10 m/s`, angular p95 `<0,20 rad/s`, penetração máxima `<0,025 m` e ao menos 80% dos corpos devem dormir. Crash, corrupção, valor não finito, handle inválido, velocidade espontânea `>100 m/s` ou violação desses limites bloqueiam; não se redefine tolerância depois de observar o resultado.

### 6.5 Stress

O cenário-alvo cria 500 corpos dinâmicos, 800 shapes e 250 juntas. Cada ciclo aquece por 300 ticks e mede 1.200. O protocolo toca previamente seus buffers, executa dez ciclos completos de allocator warm-up e depois outros dez ciclos completos medidos de criar/simular/destruir; toda amostra ocorre somente após teardown completo. Em todos os builds, `b3GetByteCount()` deve começar em zero no processo isolado e retornar exatamente ao baseline antes/depois de cada cenário e após cada um dos 20 ciclos Stress; delta positivo ou negativo é `box3d_allocator_imbalance`. Em Debug `/MTd`, um ciclo completo adicional é envolvido por `_CrtMemCheckpoint`; diferenças `_NORMAL_BLOCK`/`_CLIENT_BLOCK` em count ou bytes devem ser exatamente zero. `_CRT_BLOCK` e free blocks não entram no gate.

Uma única chamada `GetProcessMemoryInfo` também captura `PROCESS_MEMORY_COUNTERS_EX.PrivateUsage`, `WorkingSetSize` e `PeakWorkingSetSize`. Esses contadores medem footprint/residência do processo, não ownership do allocator Box3, e são sempre diagnósticos nesta foundation; Release `/MD` ainda é `configuration_mismatch` de toolchain, validado no início de `ScenarioRunner::run` para qualquer um dos seis cenários por um evaluator interno sem seam público de execução, não falha do assessment. Ambos os contadores serializam, por repetição, `gate_applied=false`, `gate_status=diagnostic`, `budget_qualified=false` e `budget_scope=future_packaged_reference_hardware`. Para os warmups 6–10 (`W`) e medidos 6–10 (`M`), a matemática permanece congelada e completa nos dois contadores: full/central min/max, mediana `s2`, growth, span central `(s3-s1)/s2`, span completo, growth instantâneo, estabilidade e guard M9/M10. Quando um contador está indisponível, seus raws ficam vazios/coerentes e o assessment é `unavailable`; isso não falha CTest nem altera exit. O assessment `pass|growth|unstable|unavailable` nunca altera exit, matriz ou hash. Os audits Debug e Release mostraram alternância/instabilidade de footprint e permanecem evidência do budget não qualificado. O target diagnóstico de 5% será gate futuro somente no Godot Release empacotado, em hardware de referência. Timing, PrivateUsage, Working Set, Box3 allocator, CRT e warnings nunca entram no hash. Cada linha da matriz preserva `functional_status` e `functional_fallback` antes de checks normativos de allocator/CRT/tooling; somente esses campos funcionais, valores funcionais e fixture hashes entram no hash canônico. O gate funcional também exige zero crash, corrupção, handle inválido ou valor não finito. Percentis locais são informativos. A meta p95 de 8 ms só é gate obrigatório no hardware de referência e protocolo da especificação-mestra.

### 6.6 Determinismo no mesmo build

Cada cenário roda duas vezes no mesmo executável, com mesma seed e ordem de comandos. Um hash quantizado de posição, rotação, velocidades e estado de vida deve coincidir. O resultado também armazena métricas tolerantes para diagnóstico. Debug e Release podem produzir hashes diferentes; cada configuração deve ser internamente repetível.

### 6.7 Matriz obrigatória go/no-go

| Capacidade Box3D | Prova | Passa quando | Fallback permitido | Bloqueia quando |
|---|---|---|---|---|
| CCD `isBullet` dinâmico–dinâmico | cenário 6.2 em 20 seeds | contato ocorre antes de atravessar a pilha em 20/20 | reduzir velocidade até 30 m/s e usar 6 substeps | tunneling persiste com o fallback |
| shape cast/overlap | cast de esfera por 3 m em hulls | primeira saída e overlap coincidem com fixtures | raycast múltiplo somente para previsão, não para Nox | não há saída segura determinística para habilidade |
| contact hit events | impactos com velocidades conhecidas | velocidade de aproximação, normal, materiais, massa efetiva e energia derivada são finitos, ordenáveis e coerentes | derivar energia das velocidades/massas dos bodies no tick | não é possível derivar energia ou evitar duplicidade/substep de modo estável |
| força/torque de juntas | junta carregada até ruptura | leitura cresce monotonicamente e cruza limite conhecido | avaliar deformação/impulso relativo por dois ticks | nenhuma métrica permite ruptura previsível |
| hulls/compounds | fixture com 8 hulls | massa, AABB e contatos passam tolerâncias | múltiplas shapes no mesmo body | crash, massa inválida ou contatos ausentes |
| sleep sob gravidade radial | cenário 6.3 | atende todos os limites numéricos de 6.3 | omitir força em bodies asleep, política já prevista | qualquer limite de 6.3 falha |
| criação/destruição em lote | 10 mil ciclos com handles, Box3/CRT e footprint diagnóstico | zero handle inválido/corrupção, retorno Box3 exato e CRT Debug sem live blocks | reduzir batch por tick | Box3/CRT desequilibrado ou configuração Release fora de `/MT`; PrivateUsage/Working Set não bloqueiam esta foundation |
| replay/validação upstream | cenário mínimo gravado | ferramenta oficial valida o arquivo do mesmo build | usar apenas replay próprio de entradas/métricas | falha impede diagnóstico de bug reproduzível, mas não o runtime |

Somente fallbacks listados podem ser adotados sem rever a especificação-mestra. Qualquer linha bloqueada produz relatório `bloquear` e interrompe o conteúdo.

## 7. Integração Godot

A GDExtension registra `Box3DWorldNode`, um `Node3D` que possui o kernel. A superfície inicial exposta a GDScript é pequena:

- `configure_planet(radius, surface_gravity)`;
- `spawn_box(size, transform, density)`;
- `spawn_projectile(radius, transform, velocity)`;
- `apply_impulse(handle, impulse, world_point)`;
- `step_fixed()` para teste controlado;
- `get_body_states()` em lote;
- sinal `physics_fault(code, message)`.

No jogo normal, o node avança pelo relógio físico do Godot usando acumulador fixo e publica snapshots. A cena mínima instancia meshes primitivos para cada handle e atualiza seus transforms. O planeta visual é uma esfera simples; a torre usa caixas coloridas; o projétil usa outra esfera. O Godot não possui corpos físicos correspondentes.

O smoke test headless abre o projeto, carrega a DLL, cria o node, executa 120 ticks, confirma movimento e encerra com código zero. O smoke test gráfico abre a cena, executa um lançamento automático e permite captura de screenshot. Nenhum desses testes exige o editor aberto.

## 8. Telemetria e falhas

O spike escreve JSON UTF-8 com:

- versões e commits de Godot, godot-cpp e Box3D;
- configuração de build e CPU detectada;
- configuração do mundo e seed;
- resultado por cenário;
- tempo mínimo, mediano, p95 e máximo do step;
- pico de corpos acordados, contatos e memória quando disponível;
- private commit e Working Set raw dos 10 warmups e 10 ciclos medidos, last/full/central min/max, median, peak, growth mediano e instantâneo, spans trimmed/full, estabilidade, assessment, scope/status e terminal guard;
- Box3 allocator process/scenario baseline/final/max delta, retorno exato e 10+10 pós-teardown; CRT applicability e deltas live-block;
- `repeat_observations[]` com índice, hash e telemetria completa Private/WS/Box3/CRT de cada repetição;
- hash final e lista de invariantes violadas.

Erros de configuração encerram o executável com código diferente de zero. “Violação fatal” significa crash, assert, timeout de 60 segundos, handle inválido, valor `NaN`/infinito, velocidade espontânea acima de `100 m/s`, body fora de `6R` sem ejeção, desequilíbrio Box3/CRT, configuração Release fora de `/MT`, ou quebra funcional das seções 6.1–6.7. PrivateUsage e Working Set nunca transformam uma execução foundation em pass/fail; todo relatório inclui `private_commit_budget_unqualified` e `budget_qualification.status=deferred`. Sem violações normativas, a recomendação obrigatória é `prosseguir_com_limites`; qualquer violação produz `bloquear`. Cada violação identifica cenário, tick, handle e valores relevantes. Logs do Godot não podem conter erro ao carregar a extensão. Artefatos de falha ficam em `artifacts/physics/`, ignorados pelo Git, com um resumo reproduzível que pode ser anexado a uma issue do Box3D.

## 9. Toolchain reproduzível

`tools/bootstrap.ps1` detecta antes de instalar. Ele prepara:

- Visual Studio Community/Build Tools 2026 `18.7.3` build `11925.98`, workload C++ x64, MSVC `14.44`/v143 e Windows SDK target `10.0.26100.0`;
- CMake `4.3.3` e Ninja `1.13.2` portáteis;
- Python 3.11+ para automação e scripts de validação;
- Godot 4.5.1 x86_64 em diretório local de ferramentas;
- templates de exportação somente quando o marco precisar gerar executável Godot.

`tools/toolchain.lock.json` registra URL oficial, versão, SHA-256 e componente de cada download. O script recusa checksum divergente, verifica `cl`, SDK, `cmake`, `ninja` e Godot, e falha com mensagem acionável. Instalações que exigem elevação são separadas das etapas portáteis. `tools/build.ps1` e `tools/test.ps1` não alteram a máquina; usam a toolchain já detectada, generator `Ninja`, `build/debug|release`, e aceitam `-Configuration Debug|Release`.

Box3D, godot-cpp e extensão usam a mesma arquitetura, toolset, Windows SDK e CRT. A extensão declara `compatibility_minimum = "4.5"` e usa a `extension_api.json` do tag fixado do godot-cpp. O smoke test do marco verifica ABI no Godot 4.5.1; o marco de entrega repete o pacote em VMs limpas Windows 10/11.

## 10. Sequência de teste orientada a comportamento

Cada comportamento do kernel segue RED–GREEN–REFACTOR:

1. escrever um teste mínimo contra a API desejada;
2. executar e confirmar falha pela ausência do comportamento;
3. implementar apenas o necessário;
4. executar o teste e a suíte relevante;
5. refatorar mantendo todos verdes.

Configuração, arquivos de projeto gerados e a cena visual mínima são validados por smoke tests e inspeção de build, pois não possuem lógica isolada suficiente para justificar testes artificiais.

## 11. Critérios de aceite

O primeiro marco termina somente quando:

- bootstrap detecta ou instala a toolchain documentada;
- todos os downloads correspondem às versões e SHA-256 do lockfile;
- configure e build Debug/Release x64 terminam com código zero;
- Box3D compilado corresponde à tag e ao SHA fixados;
- todos os testes headless passam nas duas configurações;
- os seis cenários produzem JSON válido e nenhuma violação fatal;
- todas as linhas da matriz go/no-go passam ou usam exatamente o fallback permitido;
- o cenário de projétil confirma ao menos um contato antes de ultrapassar a pilha;
- a repetição de cada cenário no mesmo build gera o mesmo hash;
- a DLL carrega no Godot 4.5.1 sem erro;
- o smoke test headless termina com código zero;
- a cena gráfica mostra planeta, pilha e projétil sincronizados por Box3D;
- não existem `RigidBody3D` nem outra física Godot na cena;
- licenças MIT de Box3D, Godot e godot-cpp estão registradas;
- o relatório do spike recomenda `prosseguir_com_limites` quando não há violações normativas e `bloquear` quando há, preservando o warning de budget futuro.

Se o resultado for `bloquear`, o produto não avança para conteúdo. A decisão documenta a falha e escolhe entre patch isolado, redução explícita de escopo ou espera por versão posterior do Box3D.
