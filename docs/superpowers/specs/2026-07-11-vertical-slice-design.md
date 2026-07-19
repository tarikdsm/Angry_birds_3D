# Ninho Orbital — Vertical Slice “Contrapeso de Aster”

**Data:** 11 de julho de 2026  
**Plataforma:** Windows desktop x86_64  
**Base:** `main` em `a4fbf33`, fundação Box3D aprovada com `prosseguir_com_limites`  
**Status:** decisão autônoma consolidada após revisão de game design, arquitetura e arte

## 1. Objetivo do marco

Entregar uma fatia jogável de produção, não uma cena descartável. O marco deve provar, numa única fase curta:

- inspeção 3D e câmera orbital;
- anel de impulso, mira tangencial e lançamento explícito por `Espaço`;
- Virela (codinome de produção sujeito a clearance) e sua habilidade gravitacional;
- Javali-Âncora com proteção direcional;
- pinho, vidro e tijolo com respostas físicas distintas;
- dano por energia, juntas rompíveis e construção pré-segmentada;
- fluxo completo de vitória, derrota, reinício, pausa e resultado;
- apresentação original com assets Blender reproduzíveis;
- Box3D v0.1.0 como único solver de gameplay.

Nox, Talo, Engenheiro, Sifonador, os outros 17 materiais, campanha, save, replay e seis fases permanecem nos marcos seguintes.

## 2. Experiência escolhida

### 2.1 Fase

`Primeira Órbita — Contrapeso de Aster` ocorre no polo norte de um miniplaneta-jardim de raio `10 m`. Uma fortificação de pinho, vidro e tijolo protege um único Javali-Âncora. O capacete aponta para o arco inicial, tornando impactos frontais pouco eficientes.

O jogador recebe três Virelas. A solução ensinada é passar pela estrutura e ativar o vórtice para puxar o contrapeso de tijolos contra o flanco vulnerável. Uma segunda rota rompe vidro e juntas de pinho para liberar o peso por colapso estrutural.

### 2.2 Rotas verificadas

1. **Gravitacional:** flyby, vórtice atrás da parede e tijolos atingindo lateral/traseira do Âncora.
2. **Estrutural:** impacto em vidro/pinho, ruptura de suportes e segundo lançamento reposicionando destroços.
3. **Derrota:** três lançamentos frontais ou sem energia suficiente, com alvo ainda ativo.

Tupla pública congelada: a rota gravitacional usa theta `{-2°,0°,0°}` e a
estrutural repete `theta=0°`; ambas usam `phase=0°`, velocidade `8 m/s` e no
máximo três lançamentos. A rota gravitacional ativa Virela `40 ticks` após cada
lançamento; a rota estrutural não ativa a habilidade. A derrota usa três tangentes que erram o
objetivo. Os testes exigem causalidade de habilidade na primeira rota e o par
exato sobrecarga→ruptura e gatilho→fratura na segunda.

Não deve existir uma solução confiável baseada em três impactos frontais diretos.

### 2.3 Ritmo

- flyover: até `4 s`, sem retirar controle depois dele;
- primeiro lançamento: até `90 s` para um jogador novo;
- fase completa: `2–6 min`;
- lançamento individual: `10–25 s`;
- dicas: somente após `12 s` parado ou dois impactos frontais.

## 3. Geometria e orçamento físico

Referencial local no polo: normal `+Y`, tangentes `+X/+Z`.

- planeta: raio `10 m`, `g_surface=9 m/s²`;
- plataforma permanente: aproximadamente `6,4 × 0,30 × 3,6 m`;
- fortificação congelada: `4,44 × 3,19 × 3,20 m`, com cada eixo até `4,5 m`;
- 8 peças de pinho, 3 painéis de vidro e 9 tijolos;
- Âncora: `480 kg`, silhueta aproximada `1,30 × 0,85 × 1,00 m`;
- máximo inicial: 25 corpos de gameplay e 18 juntas;
- máximo após rupturas: 45 corpos neste marco;
- teto da sessão continua 500 bodies, 800 shapes e 250 joints.

O slice usa assemblies já segmentados como corpos ligados. Substituição arbitrária pai→fragmentos fica para “Destruição sistêmica”. Vidro pode revelar fragmentos visuais preparados, mas a autoridade física deste marco é a liberação das partes já declaradas.

## 4. Lançamento e controles

### 4.1 Estados

```text
Inspection → Aim → FlightAbility → Resolution → Evaluation
     ↑                                             |       |
     +----- objetivo pendente + aves restantes ----+       +→ Result

Qualquer falha física ou de conteúdo leva a `Faulted`, com `Outcome::None`; somente `restart` ou configuração válida libera novos ticks.
```

- `Inspection`: câmera livre, seleção do anel;
- `Aim`: origem na casca e direção no plano tangente;
- `FlightAbility`: voo e uma ativação após armamento;
- `Resolution`: câmera/pausa/reinício apenas;
- `Evaluation`: objetivos avaliados no limite de tick;
- `Result`: vitória ou derrota.

### 4.2 Comandos

- botão direito: órbita;
- roda: zoom;
- clique no anel: inicia mira;
- `Alt` + arraste: reposiciona anel dentro do arco permitido;
- arraste no gizmo: direção/potência;
- `Q/E`: rotação azimutal fina do anel, sem roll da câmera; `A/D`: potência fina;
- `Esc`: cancela a mira antes de abrir a pausa; `F`: recentra câmera no anel/fortificação;
- `Espaço`: lança ou ativa habilidade depois de 9 ticks;
- `R` mantido `0,5 s`: reinicia;
- `Esc`: pausa.

Soltar o mouse nunca lança.

### 4.3 Contrato numérico

- casca de lançamento: `R+3 m`, tolerância `5 cm`;
- arco geodésico: `radial(θ)=(-cos θ, 0, sin θ)` no referencial local do polo, com `θ∈[-50°, +50°]`; a origem é `13*radial(θ)` e a direção tangente permanece perpendicular ao radial;
- velocidade global aceita: `8–40 m/s`;
- faixa da fase: `8–16 m/s`, padrão `10,5 m/s`;
- tangência: `abs(dot(direction, radial)) ≤ 0,01`;
- posição quantizada a `1 mm`;
- direção quantizada a `1e-4` e renormalizada;
- velocidade quantizada a `0,01 m/s`;
- Virela física: esfera bullet de raio `0,45 m`.
- Virela: massa alvo `140 kg` (`densidade 366,76 kg/m³` para a esfera), atrito `0,35`, restituição `0,25`;

A previsão de trajetória é calculada pelo kernel com a mesma quantização, gravidade, shapes e filtros do lançamento. Ela termina no primeiro impacto provável, inclui amostras e hit autoritativos no pacote batched e é acompanhada de marca e sombra no planeta. Antes de corpos dinâmicos alterarem a cena, o erro permitido entre preview e lançamento é `≤0,10 m` no ponto de impacto.

## 5. Virela

- armamento: 9 ticks (`150 ms`);
- duração do campo: 75 ticks (`1,25 s`);
- raio: `4 m`;
- massa elegível por corpo: `≤150 kg`;
- até 20 corpos processados, ordenados por `(EntityId, PartId)`; o handle é detalhe interno usado somente após a seleção;
- aceleração máxima: `12 m/s²`;
- pulso final máximo: `4 m/s`;
- ignora planeta, cenário permanente, neutralizados e a própria Virela.

A origem do campo acompanha o centro físico da Virela. A ativação expira no primeiro impacto, na ejeção ou após 600 ticks se ainda não tiver começado. O tick que aceita `ActivateAbility` é o tick 1: aplica força antes do primeiro `step`. Os ticks 1–74 aplicam atração; o tick 75 aplica atração, pulso radial e publica `AbilityPulse/AbilityEnded` após o solver. Se o lançamento ocorre no tick `L`, a primeira ativação válida é `L+9` e o final correspondente é `L+83`. Depois de iniciada, a habilidade sempre conclui os 75 ticks antes da saída de `FlightAbility`.

Para distância `r`:

```text
t = clamp(1 - r/4, 0, 1)
weight = t²(3 - 2t)
a = 12 * weight
```

Cada tick aplica `force = mass*a` em direção ao campo. No tick final, `impulse = mass*4*weight` radial para fora.

Eventos: `BirdLaunched`, `AbilityStarted`, `AbilityAffectedBody`, `AbilityPulse`, `AbilityEnded`.

## 6. Materiais do slice

| ID | Chave | Resposta | Densidade | Atrito | Restituição | Tenacidade |
|---:|---|---|---:|---:|---:|---:|
| 1 | `pine` | fibrosa | 520 | 0,55 | 0,18 | 0,55 |
| 5 | `brick` | alvenaria | 1800 | 0,72 | 0,08 | 0,52 |
| 9 | `glass` | frágil | 2450 | 0,42 | 0,10 | 0,01 |

- pinho acumula dano durante a fase e rompe juntas/segmentos críticos;
- tijolo acumula dano nas juntas de argamassa, mas o bloco não fragmenta;
- vidro rompe somente quando um único dano normalizado alcança `1`.

Todos os contatos usam o evento de maior velocidade normal por par em cada tick:

```text
E = 0,5 * m_eff * max(0, v_n - 1)²
D = E / (massa_alvo * 250 J/kg * tenacidade)
```

Juntas rompem se o maior de força/limite e torque/limite for `≥1` por dois ticks ou `≥1,5` num tick. A remoção ocorre no início do tick seguinte.

Cada joint estrutural declara obrigatoriamente `force_limit_n > 0` e `torque_limit_nm > 0`, finitos. Valores congelados: encaixe de pinho `7000 N / 1200 N·m`, grampo de vidro `3000 N / 500 N·m`, sete argamassas de suporte `1400 N / 160 N·m` e uma argamassa sacrificial `950 N / 160 N·m`. Zero, ausência, não finito ou unidade implícita são inválidos.

## 7. Javali-Âncora

- integridade inicial: `100`;
- massa: `480 kg`;
- frente local do capacete: `-Z` no runtime Godot/Box3D;
- cone protegido: `45°`;
- vulnerabilidade frontal: `0,25`;
- vulnerabilidade lateral/traseira: `1,0`;
- dano por impacto limitado a `55` de integridade.

Calibração inicial:

```text
integrity_damage = min(55,
  100 * E / (480 kg * 2,5 J/kg) * vulnerability)
```

A convenção da normal de contato é testada explicitamente. O alvo neutraliza uma única vez por integridade zero ou transição para ejetado.

## 8. FSM, repouso e resultado

- repouso relevante: linear `<0,15 m/s` e angular `<0,20 rad/s` por 60 ticks;
- watchdog: 1800 ticks, sem interromper habilidade ou projétil relevante;
- vitória: Âncora neutralizado, avaliada somente em `Evaluation`;
- derrota: nenhuma Virela restante e objetivo pendente;
- se há aves restantes, `Evaluation` retorna a `Inspection`;
- falha física ou conteúdo inválido bloqueia a sessão; nunca vira derrota.

O projétil é relevante até impacto/ejeção/repouso ou 600 ticks de voo. Ao alcançar 600 ticks sem habilidade ativa, sua remoção é agendada e a FSM entra em `Resolution`; se a habilidade está ativa, ela termina e então ocorre a remoção. O watchdog absoluto em 1800 ticks desde o lançamento remove o projétil, encerra repouso e força `Evaluation`, garantindo progresso determinístico.

## 9. Arquitetura

### 9.1 Decisão

```text
Godot presentation/input
        |
OrbitalSessionNode (GDExtension)
        |
SimulationSession (C++20 headless)
        |
PhysicsWorld
        |
Box3D v0.1.0
```

`Box3DWorldNode` permanece intacto como regressão da fundação. O gameplay usa um novo `OrbitalSessionNode`. Godot nunca calcula dano, habilidade, objetivo ou vitória.

### 9.2 Módulo nativo

`native/simulation` produz `ninho_simulation_kernel` e liga `ninho::physics`.

Tipos de domínio usam IDs próprios (`EntityId`, `PartId`, `JointId`, `EventId`, `TickIndex`) e nunca expõem handles físicos.

API essencial:

```cpp
class SimulationSession {
public:
    static Result<std::unique_ptr<SimulationSession>> create(
        const MaterialCatalog&, const ArchetypeCatalog&, const LevelManifest&);
    Status reconfigure(
        const MaterialCatalog&, const ArchetypeCatalog&, const LevelManifest&);
    Status enqueue(PlayerCommand);
    Status tick();
    std::span<const EntitySnapshot> snapshots() const;
    std::span<const DomainEvent> events() const;
    TrajectoryPreview preview(const AimState&) const;
    const SessionState& state() const;
    WorldMetrics physics_metrics() const;
};
```

Comandos entram no próximo tick; a fila tem 128 entradas. Mira múltipla no mesmo tick é coalescida pela maior sequência. Snapshots ordenam `(EntityId, PartId)`; eventos ordenam `(tick,event_id)`. Os `std::span` publicados por `snapshots()`, `structural_joints()` e `events()` são views somente de leitura de buffers pertencentes à sessão e podem ser invalidados por qualquer operação não `const`, além de move ou destruição; quem precisar reter os dados deve copiá-los. `events()` contém somente eventos do último tick e é sobrescrito no próximo; o adaptador os copia após cada tick. Falha de `tick()` é latched em `Faulted`, preserva código/mensagem no frame e rejeita ticks posteriores até restart/configuração.

Como `PhysicsWorld` cria corpos/joints por comandos diferidos, a Tarefa 3 adiciona `commit_pending_initial_state()`: permitido somente antes do primeiro step, aplica criação canônica sem integrar física, não avança tick e não publica contatos. Chamadas posteriores falham. Assim o snapshot tick zero existe sem gravidade ou eventos ocultos.

`reconfigure()` constrói uma sessão candidata completa, incluindo commit inicial e snapshots; somente depois de sucesso troca o estado ativo. Qualquer erro preserva bytes canônicos, eventos, sequências e handles da sessão anterior.

Os joints estruturais são welds. Como o schema declara endpoints e poses, mas não frames manuais, o builder calcula um frame mundial compartilhado no midpoint dos centros iniciais, com orientação mundial identidade, e o converte para os dois espaços locais. Isso preserva a pose relativa do manifesto sem snap no primeiro solver step; teste exige drift inicial `≤1e-5 m/rad`.

Ordem do tick:

1. aplicar mutações agendadas;
2. consumir comandos;
3. atualizar FSM/criar projétil;
4. aplicar Virela;
5. `PhysicsWorld::step()`;
6. copiar contatos/ejeções/joints;
7. calcular dano, rupturas e objetivos;
8. enfileirar mutações;
9. publicar snapshots/eventos.

### 9.3 Conteúdo JSON

Adicionar `nlohmann/json v3.11.3`, tag/commit `9cca280a4d0ccf0c08f47a99aa71d1b0e52f8d03`, MIT, header-only e pinado.

O parser usa `json::parse` explícito, exige tipos/ranges e compara cada conjunto de chaves permitido. `parse_material_catalog()`, `parse_archetype_catalog()` e `parse_level_manifest()` têm resultados tipados independentes; o bundle só é aceito após referências cruzadas entre os três. Chave desconhecida, ID duplicado, referência órfã, material ausente, número não finito, shape inválida ou objetivo sem alvo bloqueiam carga. A criação é atômica.

Os registros já são data-driven por `BirdArchetype`, `AbilityArchetype`, `EnemyArchetype`, `WeakpointProfile` e `MaterialDefinition`; neste marco somente Virela, Âncora e três materiais destrutíveis ficam habilitados. Um catálogo separado `PhysicsSurfaceDefinition` contém superfícies não destrutíveis reservadas (`planet_soil=1001`, `platform_mineral=1002`, `virela_body=1003`, `anchor_armor=1004`) com atrito/restituição/densidade quando aplicável; elas não contam para os 20 materiais jogáveis.

O nível declara `force_limit_n/torque_limit_nm` por joint, o arco numérico, archetype IDs, surface IDs e transforms/proxies. Arquivos:

- `game/data/materials/vertical_slice.materials.json`;
- `game/data/archetypes/vertical_slice.archetypes.json`;
- `game/data/levels/first_orbit.level.json`;
- `game/data/feedback/vertical_slice.feedback.json`;
- `game/data/ui/vertical_slice.pt-BR.json`.

O catálogo de UI mantém os textos pt-BR fora da cena e do script do HUD, sob IDs
canônicos `hud.*`. Ele é carregado uma vez, rejeita integralmente schema, chaves ou
tokens de formatação inválidos e usa somente IDs técnicos seguros quando o
documento não pode ser aceito; fase e outcome desconhecidos resolvem para chaves
`unknown` autoradas, sem criar chaves dinamicamente.

## 10. Fronteira GDExtension

`OrbitalSessionNode` expõe:

```text
configure_session(materials_json, archetypes_json, level_json)
queue_begin_aim()
queue_aim(origin, tangent_direction, speed)
queue_launch()
queue_activate_ability()
queue_cancel_aim()
restart_level()
consume_frame()
```

`consume_frame()` é a única leitura por frame e retorna tick, ticks executados, fase, outcome, aves, snapshots, eventos, objetivos, preview de trajetória, métricas e atraso descartado. Eventos acumulam todos os ticks do frame e são limpos ao consumir; o último snapshot permanece.

O `OrbitalSessionNode` roda com prioridade de physics process `-100`; o controller usa prioridade `0`, garantindo que a extensão avance antes da única leitura. Acumulador, batching e conversões ficam em helpers C++ puros; bindings e sinais reais são testados dentro do Godot headless.

Toda exceção é capturada antes da ABI e emite `gameplay_fault`.

`queue_aim()` rejeita valores não finitos ou não representáveis retornando `false`, sem enfileirar comando, emitir `gameplay_fault` ou bloquear a sessão. Falhas de conteúdo, exceções e falhas de `tick()` continuam latched e exigem `restart_level()` ou nova configuração válida.

## 11. Godot

A cena usa apenas nodes de apresentação: `Node3D`, `OrbitalSessionNode`, meshes, câmera, luzes, ambiente, partículas, áudio e UI. São proibidos nodes físicos Godot, incluindo `Area3D`.

O controller chama `consume_frame()` exatamente uma vez por `_physics_process`. A câmera resolve ray–sphere e plano tangente apenas para autoria; o kernel revalida.

O scanner verifica texto e árvore runtime. `tools/test.ps1` sempre abre explicitamente `res://scenes/physics_spike.tscn`, portanto trocar `run/main_scene` não altera o gate da fundação.

Câmera:

- FOV `48°`, distância `14–24 m`;
- inspeção 360°, inclinação `15–70°`;
- voo sem roll automático, look-ahead `2 m`;
- impacto com FOV kick máximo `2°`;
- opção de movimento reduzido remove shake/kick.

O enquadramento mantém anel, primeiro impacto e Âncora dentro de margens seguras de screen space. Ray–sphere contra o planeta e bounds visuais da fortificação detectam oclusão; a câmera eleva/afasta suavemente ou troca o foco para o ponto de impacto. Se projétil e alvo não couberem juntos, prioriza o projétil durante o voo e o impacto durante 0,6 s, depois retorna à composição da fortificação. `F` recentra. Overview, mira, flyby e colapso são validados em 16:9, 16:10 e 21:9.

## 12. Direção visual e Blender

Identidade: jardim cósmico estilizado contra mineração mineral.

- espaço `#07111F`, solo `#183D3A`, musgo `#4E9D6C`;
- coral `#F47C72`, pólen `#F6C95C`, energia `#42E1D0`;
- ferrugem `#B65336`, roxo `#674361`, latão `#D7A441`;
- pinho `#C88E55`, vidro `#8DE0E8`, tijolo `#B85B42`.

Assets: Jardim Aster, Virela, Âncora, anel de impulso e kit dos três materiais. Blender 5.1 gera fontes `.blend`, GLBs, LODs, fragmentos, hulls, UVs e texturas originais com seed fixa.

Convenções:

- Blender: metros, Z-up, frente `-Y`;
- runtime: metros, Y-up, frente `-Z`;
- pivô físico no centro de massa; `VIS_ROOT` guarda offset artístico;
- `COL_*` são proxies lidos pelo importador próprio, nunca pelo sistema de física Godot;
- hull convexo preferencialmente `≤16` vértices;
- escala aplicada `1,1,1`, determinante positivo.

`art/config/vertical_slice_assets.json` é a fonte geométrica autoral; o pipeline gera um manifesto intermediário canônico de proxies. O validador exige igualdade de IDs, bounds e transforms com `first_orbit.level.json`; divergência bloqueia o build. A cena da Tarefa 9 usa placeholders pelos mesmos IDs até a substituição pelos GLBs.

Budgets do slice: `≤300k` triângulos, `≤180` draw calls, `≤128 MB` texturas, `≤30k` partículas e `≤120` fragmentos visuais/físicos.

## 13. Áudio, VFX e UI

- Virela: subida harmônica turquesa e pulso grave;
- pinho: estalo fibroso; vidro: prisma agudo; tijolo: impacto seco;
- capacete frontal: centelha branca/acorde abafado;
- flanco vulnerável: veios âmbar/pulso grave;
- VFX não encobre primeiro impacto por mais de `150 ms`;
- UI de cartografia astral, sem estrelas ou linguagem visual copiada.

Virela, Nox e Talo são codenames até clearance de nome. O checklist clean-room bloqueia nomes, logos, silhuetas, sons, UI, layouts e material promocional reconhecíveis de Angry Birds/Rovio ou de outras franquias. Cada asset registra autoria, seed/fonte e hash, e uma revisão comparativa precede o pacote. Isso reduz risco de associação, mas não substitui parecer jurídico antes da publicação comercial.

## 14. Testes e aceite

O marco passa quando:

- fundação + novos testes passam em Debug/Release;
- duas rotas vencem e uma rota perde deterministicamente;
- repetição preserva hash e ordem de eventos;
- Virela respeita armamento, raio, massa, duração, ordem e pulso;
- Âncora recebe `0,25` frontal e `1,0` lateral/traseiro;
- pinho, vidro e tijolo têm regras distintas;
- joints rompem somente pelas regras de um/dois ticks;
- objetivo/outcome transitam uma vez;
- 20 reinícios não aumentam bodies, joints, nodes ou bytes Box3D;
- `consume_frame()` é chamado uma vez por frame;
- zero nodes físicos Godot no gameplay;
- smoke vence, perde e reinicia;
- captura de 300 frames mostra mira, voo, Virela, impacto, ruptura e resultado;
- logs não contêm `ERROR`, `SCRIPT ERROR` ou `WARNING`;
- p95 do step no slice é `≤8 ms` nesta máquina, informando hardware;
- em 1080p médio, Vulkan e OpenGL mantêm p95 de frame `≤16,67 ms`, p99 `≤25 ms`, sem hitch `>50 ms` durante ruptura/VFX, e input do engine até feedback visual p95 `≤33,4 ms`; testar UI em escala 100% e 150%;
- assets são reproduzíveis e passam bounds/pivô/hull/LOD;
- capturas douradas: overview, aim, Virela, impacto vulnerável e resultado;
- rubric visual bloqueante: silhueta, hierarquia focal, leitura dos materiais, contraste, clipping, transparência e distinção frontal/lateral;
- playtest com cinco pessoas novas: 4/5 lançam em 90 s, 4/5 concluem em 6 min, 4/5 entendem a proteção frontal, 3/5 descobrem a rota de destroços e todos distinguem os três materiais. Enquanto não houver participantes e observações humanas, esse playtest permanece pendente e é obrigatório antes do marco Produto/lançamento. Reviews técnicos por agentes podem bloquear o slice por findings graves de código, arquitetura, gameplay ou arte, mas não contam participantes, não medem usabilidade humana e não substituem esse playtest;
- Box3D pinado continua sendo o único solver.

As pré-condições de fundação, testes, reinícios, ABI, scanner, assets e smokes permanecem etapas fail-fast do orquestrador. Elas não são serializadas como booleanos `acceptance` na evidência, porque um valor escrito pelo mesmo processo que declara sucesso não constitui prova independente. O gate da evidência valida somente observações e artefatos concretos com identidade própria, como manifests, hashes, logs, rotas, métricas, goldens, reviews e pacote. Snapshots históricos que ainda contenham `acceptance` permanecem legíveis, mas esse campo legado é ignorado e não concede confiança.

O contrato corrente registra o playtest humano como `status=not_performed`, `participants=0`, `substitute=none` e `gate_status=pending`. O gate do vertical slice aceita essa pendência porque certifica controles técnicos, não prontidão de produto. O formato histórico `status=unavailable`/`substitute=independent_agents` continua legível exclusivamente como pendência legada e não satisfaz o requisito humano. Os `reviewer_id` do manifest são rótulos de atribuição de agentes, sem autenticação de identidade ou garantia de independência.

O hash de sessão usa contrato `canonical_state_v2`: campos de domínio em ordem fixa, inteiros little-endian, floats quantizados a `1e-5`, `-0` normalizado para `+0`, strings UTF-8 com tamanho e eventos em `(tick,event_id)`. A v2 inclui explicitamente o papel `is_projectile` dos snapshots e a classificação direcional de dano dos eventos; qualquer alteração futura de layout exige nova versão e golden congelado. Exclui timings, endereços, handles físicos e métricas de alocação. O diagnóstico de alocador é exposto somente ao target de testes por uma façade interna; o smoke Godot mede separadamente SceneTree, views e pools antes/depois de 20 reinícios.

As rotas usam `ordered_events_v2` para serializar o histórico completo de eventos em ordem de publicação, incluindo `damage_classification` imediatamente após `damage`, e `canonical_playthrough_v4` para combinar esse stream com `canonical_state_v2`. Produtor e gate aceitam somente essas versões; qualquer mudança futura em um dos layouts exige promover a respectiva versão antes de recapturar evidência.

## 15. Riscos e limites

- mira 3D: arco restrito, trajetória autoritativa, sombra e impacto marcado;
- dano ilegível: feedback distinto por material e face;
- vórtice instável: 20 corpos, `≤150 kg`, força fixa e ordem por IDs de domínio;
- fragmentos: assemblies pré-segmentados e teto local;
- câmera: sem roll, transições limitadas e movimento reduzido;
- arte: kit modular procedural, não escultura manual extensa;
- Box3D alfa: todos os gates da fundação permanecem obrigatórios.

## 16. Roadmap contratual pós-slice

O vertical slice termina deliberadamente em uma fase, uma ave, um inimigo e três materiais, mas a entrega pedida permanece contratada em marcos verificáveis:

1. **Arquétipos 3/3:** habilitar três aves e três inimigos exclusivamente por registros de dados; cada arquétipo recebe habilidade/fraqueza própria, testes de interação com todos os já existentes, assets autorais, balanceamento e uma fase de demonstração.
2. **Catálogo de 20 materiais:** habilitar 20 `MaterialDefinition` com resposta física, dano, ruptura, áudio/VFX e legibilidade distintos; matriz automatizada valida todo par ave/material e inimigo/material sem branches por nome.
3. **Conteúdo e produto:** campanha mínima, seleção/salvamento, onboarding progressivo, opções, pacote Windows e playtests de retenção/dificuldade; gates de performance, acessibilidade, IP e licenças continuam obrigatórios.

Matriz final: 3 aves × 20 materiais, 3 inimigos × 20 materiais e 3 aves × 3 inimigos, com cenários golden, determinismo e budgets. Nenhum marco pode hardcodar “Virela” ou “Âncora” no kernel; regras são resolvidas por archetype/ability/weakpoint IDs.
