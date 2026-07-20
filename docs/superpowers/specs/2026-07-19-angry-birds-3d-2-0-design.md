# Angry_birds_3D — Design da versão 2.0

**Data:** 19 de julho de 2026  
**Status:** decisões de produto aprovadas por entrevista; defaults restantes autorizados como “recomendado”  
**Plataforma inicial:** Windows desktop x86_64, mouse e teclado  
**Base técnica:** Godot 4.5.1, C++20, GDExtension, Box3D v0.1.0 e Blender 5.1.2

## 1. Visão

A versão 2.0 transforma o vertical slice atual em um jogo com dois mundos:

1. **Terra**, apresentado primeiro, com gravidade uniforme e uma fase inicial em uma fazenda.
2. **Orbital**, apresentado em segundo, contendo a fase atual Contrapeso de Aster, ainda com Virela e gravidade radial.

Os dois mundos ficam disponíveis desde o primeiro acesso. A Terra é destacada como a experiência recomendada, sem bloquear o Orbital.

O vocabulário de controle é o mesmo nos dois mundos:

- uma ave repousa em um estilingue;
- o jogador posiciona a câmera;
- clicar na ave trava um plano de lançamento 3D guiado;
- arrastar armazena energia elástica conforme a lei de Hooke;
- soltar o botão lança;
- um novo clique ou Espaço, durante o voo e depois do armamento, ativa uma única vez a habilidade da ave.

Esta mudança justifica a numeração 2.0 porque substitui o dispositivo de lançamento do produto, generaliza o modelo físico de mundo, adiciona navegação, progressão, save, pontuação, quatro novas aves e uma segunda ambientação completa.

## 2. Objetivos do marco

A 2.0 deve:

- recuperar a leitura imediata do estilingue clássico sem reduzir o projeto a 2D;
- manter a simulação C++ como única autoridade de gameplay;
- aplicar gravidade a toda entidade física dinâmica;
- suportar gravidade uniforme e radial pelo mesmo kernel;
- preservar a fase orbital atual como experiência jogável e como regressão;
- entregar uma fase Fazenda coesa, com quatro aves e quatro porcos;
- entregar modelos, rigs, animações, VFX e áudio finais, não placeholders;
- implementar pontuação, três estrelas e save local;
- manter UI e narrativa em pt-BR, preparadas para localização futura;
- manter 1080p/60 fps e os gates técnicos já existentes;
- identificar o projeto como fan game não oficial.

## 3. Escopo

### 3.1 Incluído

- App shell com aviso de fan game, menu, opções, carrossel 3D de mundos e seleção de fases.
- Mundo Terra com uma fase: **Fazenda — Reação em Cadeia**.
- Mundo Orbital com uma fase: **Primeira Órbita — Contrapeso de Aster**.
- Estilingue terrestre e estilingue orbital.
- Quatro aves terrestres: Vermelha, Amarela, Azul e Preta.
- Virela preservada na fase Orbital.
- Madeira, vidro, tijolo, palha e chapa metálica.
- Porco terrestre fisicamente plausível e não gráfico.
- Explosivos rurais contextualizados.
- Fila fixa de aves por fase.
- Score, multiplicador de cadeia, estrelas e recorde local.
- Trajetória curta por padrão e assist ampliado opcional.
- Áudio completo, música e ambience.
- Pipeline Blender reproduzível e assets premium.
- Testes, capturas, reviews e evidência Debug/Release.

### 3.2 Fora do marco

- Implementação das seis aves adicionais do roadmap.
- Novas fases de deserto, floresta, cidade, medieval ou outras regiões terrestres.
- Novas fases orbitais.
- Controle, toque ou versão mobile como suporte oficial.
- Multiplayer, serviços online, cloud save ou conta.
- Fratura procedural arbitrária em runtime.
- Campanha com cutscenes extensas.
- Distribuição pública ou marketing sem análise de direitos e autorização aplicável.

## 4. Alternativas arquiteturais consideradas

### 4.1 Núcleo único orientado a dados — escolhido

SimulationSession continua sendo o motor autoritativo. Mundo, gravidade, bounds, launcher, fila, score e habilidades são variantes tipadas escolhidas pelo conteúdo.

Vantagens:

- preserva determinismo, dano, fratura, replay, eventos e testes;
- evita duplicar regras entre Terra e Orbital;
- permite combinar aves e mundos futuramente por dados;
- mantém uma única fronteira GDExtension.

### 4.2 Sessões separadas Terra/Orbital — rejeitado

Reduziria a alteração local inicial, mas duplicaria FSM, score, habilidades, correções e certificação.

### 4.3 Reescrita em ECS/framework genérico — rejeitado

Ofereceria flexibilidade sem benefício proporcional para duas fases e colocaria a fundação Box3D aprovada em risco.

## 5. Arquitetura de produto

~~~text
Godot
  AppShell, menus, input, câmera, HUD, save, áudio, VFX e meshes
                              |
              GameplaySessionNode (GDExtension)
                              |
               SimulationSession (C++20 headless)
  mundo, launcher, Hooke, fila, habilidades, dano, score e FSM
                              |
                        PhysicsWorld
          gravidade, bounds, queries, corpos e juntas
                              |
                         Box3D v0.1.0
~~~

Regras de fronteira:

- Godot não calcula velocidade de lançamento, dano, explosão, score, estrelas, objetivo ou vitória.
- Godot calcula apenas raios de câmera, interseção visual com o plano já publicado e apresentação.
- O kernel recebe deslocamento no plano, nunca uma velocidade arbitrária.
- Handles Box3D nunca atravessam a API pública.
- Toda habilidade faz dispatch por tipo de habilidade, nunca pelo nome ou cor da ave.
- Toda coleção que influencia resultado usa ordem canônica por IDs de domínio.

## 6. Compatibilidade com o slice atual

A 2.0 será adicionada como camada de produto paralela:

- game/scenes/vertical_slice.tscn e os três JSON v1 atuais permanecem como fixture certificada.
- OrbitalSessionNode permanece registrado como wrapper de compatibilidade.
- GameplaySessionNode é a API genérica nova.
- O app 2.0 usa conteúdo v2 e uma nova cena orbital de produto.
- O capture legado passa a abrir vertical_slice.tscn explicitamente, sem depender de run/main_scene.
- A migração v1 para o modelo interno v2 é explícita e testada; um parser nunca adivinha a versão.
- canonical_state_v2 permanece para o fixture legado.
- O produto 2.0 promove canonical_state_v3 e frame_schema_v2.

O comportamento orbital v1 deve ser caracterizado antes da generalização. Gravidade, preview, sequência de eventos, hashes, ejeção e rotas atuais viram regressões congeladas.

## 7. Contratos de conteúdo v2

### 7.1 WorldDefinition

Variante fechada:

- UniformWorldDefinition
  - acceleration_m_s2;
  - AabbWorldBounds;
  - local-up derivado de menos a aceleração.
- RadialWorldDefinition
  - center_m;
  - reference_radius_m;
  - reference_acceleration_m_s2;
  - SphericalWorldBounds;
  - local-up radial.

PhysicsWorld mantém a gravidade global do Box3D zerada e aplica força massa × aceleração antes de cada step para os dois modelos. Isso preserva a mesma ordem de tick e evita diferenças de sleep entre modos.

### 7.2 LaunchDeviceDefinition

SlingshotDefinition contém:

- asset_id;
- rest_position_m;
- rest_rotation;
- spring_constant_n_m;
- energy_efficiency;
- minimum_extension_m;
- maximum_extension_m;
- plane_policy = gravity_vertical_camera_yaw;
- projectile_clearance_m;
- world-specific speed ceiling.

O conteúdo Orbital v2 também usa SlingshotDefinition; OrbitalRingDefinition existe apenas no adaptador v1.

### 7.3 AbilityDefinition

Variante fechada:

- GravityFieldAbilityDefinition;
- MassBoostAbilityDefinition;
- SpeedBoostAbilityDefinition;
- ExplosionAbilityDefinition;
- SplitAbilityDefinition.

Cada payload contém apenas os campos válidos para seu tipo. Chaves desconhecidas, campos de outro tipo, números não finitos e valores fora de faixa bloqueiam a carga.

### 7.4 BirdArchetype

Campos:

- id e key;
- projectile_visual_id;
- ability_id;
- surface_id;
- mass_kg;
- radius_m ou shape;
- friction e restitution;
- bullet;
- launch_speed_cap_m_s;
- score/icon/animation presentation IDs.

### 7.5 LevelManifest

Campos novos:

- schema_version = 2;
- world_id e region_id;
- WorldDefinition;
- SlingshotDefinition;
- bird_queue em ordem exata;
- scoring;
- bodies, joints, assemblies, triggers e objectives;
- camera_profile_id;
- presentation_profile_id;
- watchdog e settle policy.

bird_queue substitui a coleção por contagem do produto novo. Duplicatas são válidas e a ordem nunca é classificada. O conteúdo orbital declara três Virelas; a Fazenda declara Vermelha, Amarela, Azul e Preta.

### 7.6 CampaignManifest

Declara:

- Terra como primeiro mundo e seleção inicial;
- Orbital como segundo;
- ambos inicialmente disponíveis;
- uma fase em cada mundo;
- ordem de fases futuras e unlock linear dentro de cada mundo;
- IDs de dioramas, textos e cenas registrados.

## 8. Gravidade e bounds

### 8.1 Terra

- aceleração: (0, -9,81, 0) m/s²;
- bounds iniciais: mínimo (-24, -12, -12) m e máximo (48, 32, 12) m;
- qualquer corpo dinâmico usa gravidade por default;
- exceções exigem affected_by_world_gravity = false explícito e só são válidas para corpos estáticos/ancorados ou estados temporários de habilidade documentados;
- sair dos bounds marca exited_world;
- porco que sai dos bounds pode ser neutralizado por queda, sem reutilizar semanticamente o termo orbital “ejeção”.

### 8.2 Orbital

- centro: (0, 0, 0);
- raio de referência: 10 m;
- aceleração na superfície: 9 m/s²;
- limite de remoção preservado em 6R = 60 m;
- NeutralizationCause::Ejection continua válido;
- a fórmula e a ordem de força atuais permanecem regressão.

### 8.3 Elementos visuais

- corpos, fragmentos e projéteis relevantes são sempre C++/Box3D;
- partículas puramente visuais amostram a gravidade do mundo no emissor;
- efeitos terrestres usam vetor uniforme;
- efeitos orbitais usam vetor radial local ou shader radial quando a duração/área tornar a aproximação local visível;
- nenhum fragmento visual persistente pode subir ou flutuar contra a gravidade sem ser parte explícita de uma habilidade.

## 9. Controle 3D guiado

### 9.1 Construção do plano

No grab, o kernel recebe o eixo direito normalizado da câmera. Ele calcula:

~~~text
up = local_up_at(launcher)
horizontal = normalize(camera_right - dot(camera_right, up) * up)
plane_normal = normalize(cross(horizontal, up))
~~~

O plano passa pelo rest_position do estilingue e é composto por horizontal e up.

Esta escolha é deliberada: camera_forward é aproximadamente normal ao plano de jogo numa composição lateral e produziria um plano incorreto. O pitch da câmera muda o enquadramento; o yaw em torno de up escolhe a direção 3D do lançamento.

No Orbital, up é o radial para fora no launcher. O horizontal projetado é tangente ao planeta, de modo que o plano contém a direção radial e uma direção tangente escolhida pela câmera.

### 9.2 Gesto

- LMB pressionado sobre a ave: BeginGrab.
- O kernel quantiza o camera_right, constrói o plano e o publica.
- Godot intersecta o raio do cursor com esse plano e envia as coordenadas horizontal_m e vertical_m.
- O kernel reconstrói o deslocamento, aplica deadzone, clamp circular e quantização.
- A câmera fica travada até cancelamento ou release.
- Mouse-up com extensão válida: ReleaseBird.
- Mouse-up abaixo da deadzone: cancelamento elástico, sem consumir ave.
- Esc durante grab: cancela.
- RMB e roda controlam câmera apenas fora de Grabbed.
- Espaço ou novo LMB durante FlightAbility: ActivateAbility.

O mouse-up que lança nunca ativa habilidade. A apresentação ignora presses de habilidade por 120 ms após o release e o kernel exige nove ticks de armamento.

### 9.3 Quantização

- posição/deslocamento: 1 mm;
- vetores de base e direção: 1e-4, seguidos de normalização;
- velocidade publicada: 0,01 m/s;
- deadzone: 0,20 m;
- extensão máxima: 4,25 m.

## 10. Lei de Hooke autoritativa

Para extensão x:

~~~text
F = -k x
E_spring = 0,5 k |x|²
E_launch = eta E_spring
v = min(speed_cap, |x| sqrt(eta k / mass))
direction = -normalize(x)
~~~

eta é eficiência de energia e, portanto, fica dentro da raiz. O projétil nasce no rest_position, não no ponto puxado. A ave durante o grab é uma apresentação ghost; não existe corpo prematuro colidindo com o estilingue.

Parâmetros iniciais:

| Mundo | k | eta | extensão máxima | cap adicional |
|---|---:|---:|---:|---:|
| Terra | 5.200 N/m | 0,90 | 4,25 m | cap da ave |
| Orbital | 2.200 N/m | 0,90 | 4,25 m | 16 m/s |

As massas realmente afetam o lançamento. Não há normalização oculta por ave. Energia acima do cap é dissipada pelo estilingue e representada por deformação/áudio, sem aumentar silenciosamente a velocidade.

Preview e lançamento chamam o mesmo solucionador de launch state e a mesma gravity_at. Antes de corpos dinâmicos alterarem a cena, o erro permitido no primeiro impacto continua ≤0,10 m.

## 11. Trajetória

O kernel pode calcular até o primeiro impacto, saída dos bounds ou três segundos. A apresentação reduz as amostras:

- padrão: 12 pontos, até 0,825 s ou 6 m;
- assist ampliado: 24 pontos, até 1,65 s ou 12 m, com primeiro impacto e sombra de profundidade;
- pontos de 6–12 px, nunca linha contínua;
- nenhum modo extrapola física no Godot;
- o assist não altera score, trajetória ou estrelas.

## 12. FSM

~~~text
Loading
  → Inspection
  → Grabbed
  → FlightAbility
      Arming → Armed → Active/Spent
      Preta: Armed → FuseArmed → Detonated
  → Resolution
  → Evaluation
      → Inspection, se objetivo pendente e há aves
      → ResultVictory
      → ResultDefeat

Falha física/conteúdo → Faulted
Pause é overlay e não um estado autoritativo de simulação.
~~~

Regras:

- release válido consome exatamente a próxima entrada da fila;
- clones da Azul não consomem novas entradas;
- somente uma ativação é aceita por lançamento;
- repouso relevante permanece linear <0,15 m/s e angular <0,20 rad/s por 60 ticks;
- voo terrestre máximo: 600 ticks;
- watchdog terrestre: 1.500 ticks;
- watchdog orbital: 1.800 ticks;
- timeout espera habilidade, fuse ou clones relevantes terminarem;
- falha de conteúdo nunca vira derrota.

O estado interno de um lançamento passa a conter um grupo de um ou mais projéteis. Isso substitui o optional de projétil único sem permitir dois lançamentos simultâneos.

## 13. Aves da 2.0

Valores são iniciais e efetivos para gameplay; a calibração final é vinculada às rotas e ao playtest.

| Ave | Massa | Raio | Cap de lançamento | Habilidade |
|---|---:|---:|---:|---|
| Vermelha | 120 kg | 0,45 m | 28 m/s | massa ×2,25 |
| Amarela | 85 kg | 0,38 m | 34 m/s | velocidade ×1,55 |
| Azul | 60 kg | 0,32 m | 34 m/s | divisão em três |
| Preta | 180 kg | 0,50 m | 24 m/s | detonação/fuse |
| Virela | 140 kg | 0,45 m | 16 m/s orbital | campo gravitacional atual |

As massas das aves são massas efetivas de gameplay. Materiais e porcos seguem valores fisicamente plausíveis; as habilidades são deliberadamente fantásticas.

### 13.1 Vermelha — momento por massa

- armamento: 9 ticks;
- multiplicador de massa e inércia: 2,25;
- velocidades linear e angular preservadas no tick de ativação;
- shape e tamanho visual não mudam;
- massa elevada permanece até o projétil ser removido;
- a mudança publica AbilityStarted, MassChanged e AbilityEnded;
- a API PhysicsWorld altera densidade/massa sem recriar a identidade do corpo.

Isso aumenta momento sem duplicar a função da Amarela.

### 13.2 Amarela — velocidade

- armamento: 9 ticks;
- velocidade multiplicada por 1,55;
- direção preservada;
- cap absoluto pós-habilidade: 45 m/s;
- massa inalterada;
- a alteração ocorre por impulso central quantizado;
- se a velocidade instantânea for quase zero, usa a última direção de voo válida.

### 13.3 Azul — divisão

A original é substituída por três filhas:

- ângulos no plano travado: -11°, 0° e +11°;
- massa de cada filha: m/3;
- raio: cuberoot(1/3) ≈ 0,693 do raio original;
- velocidade comum:

~~~text
v_child = v_parent × 3 / (1 + 2 cos(11°))
~~~

- massa e momento linear total são conservados dentro da quantização;
- colisões entre irmãs ficam desabilitadas por três ticks;
- as três compartilham o mesmo shot e a habilidade já gasta;
- o solver espera todas terminarem antes de Evaluation.

### 13.4 Preta — explosão

- armamento: 9 ticks;
- ativação manual detona imediatamente;
- primeiro impacto qualificável inicia fuse de 39 ticks, ou 0,65 s;
- impacto qualificável: velocidade normal ≥3 m/s ou impulso normal ≥250 N·s;
- ativação durante o fuse antecipa a explosão;
- detona exatamente uma vez;
- raio: 4 m;
- máximo de 32 corpos, selecionados por ID;
- variação radial máxima de velocidade no centro: 12 m/s;
- choque estrutural equivalente no centro: 90 J/kg;
- falloff smoothstep;
- cobertura física atenua o burst por query determinística;
- a ave é removida depois de publicar todos os eventos causais.

### 13.5 Virela

Contrato preservado:

- armamento 9 ticks;
- campo por 75 ticks;
- raio 4 m;
- até 20 corpos de no máximo 150 kg;
- aceleração máxima 12 m/s²;
- pulso final 4 m/s.

## 14. Ability dispatcher e múltiplos projéteis

SimulationSession deixa de chamar apply_gravity_field nominalmente. O tick usa:

1. apply_ability_before_step;
2. PhysicsWorld::step;
3. process_ability_contacts_after_step;
4. finish_ability_after_step.

O runtime de habilidade é uma variante. Cada sistema vive em arquivo próprio e recebe apenas IDs de domínio/serviços públicos de PhysicsWorld.

ShotState contém:

- shot_id;
- bird_archetype_id;
- launch_tick;
- plano travado;
- activation_consumed;
- runtime de habilidade;
- lista ordenada de ProjectileState.

## 15. Materiais da Fazenda

| Material | Densidade | Atrito | Restituição | Falha inicial | Resposta |
|---|---:|---:|---:|---:|---|
| Madeira/pinho | 520 kg/m³ | 0,55 | 0,15 | 65 J/kg acumulados | fibrosa |
| Vidro | 2.450 kg/m³ | 0,38 | 0,08 | 18 J/kg em um pico | frágil |
| Tijolo | 1.800 kg/m³ | 0,72 | 0,05 | 130 J/kg | alvenaria |
| Palha prensada | 110 kg/m³ | 0,82 | 0,12 | 22 J/kg acumulados | compressível |
| Chapa de aço | 7.800 kg/m³ | 0,48 | 0,10 | yield por junta | dúctil |

Limites iniciais de juntas:

- encaixe de madeira: 5.500 N / 900 N·m;
- grampo de vidro: 2.200 N / 350 N·m;
- argamassa: 1.400 N / 160 N·m;
- amarra de palha: 800 N / 90 N·m;
- chapa yield: 3.200 N / 450 N·m;
- chapa ruptura: 7.500 N / 900 N·m.

Comportamento:

- madeira acumula energia e se solta em segmentos autorados;
- vidro exige pico e revela fragmentos preparados;
- tijolos permanecem majoritariamente inteiros; argamassa falha primeiro;
- palha dissipa energia, comprime visualmente e rompe amarras;
- chapa entra em estado yielded, preserva deformação pela recriação canônica da junta no frame atual e só rompe no segundo limite.

As peças e padrões de quebra são produzidos no Blender. Não há corte procedural arbitrário.

## 16. Porcos

Arquétipo inicial:

- massa: 65 kg;
- altura aproximada: 0,9 m;
- compound collider derivado de hulls convexos autorados;
- atrito: 0,65;
- restituição: 0,05;
- integridade: 100;
- weakpoint uniforme.

Dano:

~~~text
specific_energy = 0,5 × effective_mass × normal_speed² / pig_mass
damage = clamp((specific_energy - 18) × 0,9, 0, 70)
~~~

- abaixo de 18 J/kg há reação, mas não dano;
- esmagamento acima de quatro vezes o peso por 0,35 s começa a causar dano;
- explosão injeta impulso e energia equivalente pelo mesmo DamageSystem;
- neutralização ocorre por integridade zero ou saída dos bounds;
- apresentação usa atordoamento, squash e expressão; não há sangue, ferimento, desmembramento ou morte explícita;
- o corpo perde relevância de objetivo e só é limpo depois do repouso.

## 17. Shapes físicos

O conteúdo de simulação v2 passa a aceitar:

- box;
- sphere;
- capsule;
- convex_hull;
- compound de primitivas/hulls.

Limites de vértices e filhos seguem PhysicsWorld. O pipeline Blender valida igualdade entre COL, bounds, massa calculada e manifesto. Mesh visual nunca vira collider automático.

## 18. Fazenda — Reação em Cadeia

### 18.1 Espaço

- área principal: X de -20 a 22 m e Z de -8 a 8 m;
- estilingue: aproximadamente (-16, 2, 0);
- área de alvos: X de 2 a 18 m;
- câmera terrestre: FOV 44°, yaw guiado ±24°, pitch 12–30°, sem roll;
- até 120 corpos dinâmicos, 60 juntas e 80 fragmentos ativos.

### 18.2 Composição

Um único cenário integrado contém:

- abrigo de madeira e palha na faixa próxima;
- celeiro central com painéis de vidro;
- silo de tijolos e chapa;
- moinho;
- cercas;
- fardos em rampa;
- vagão/equipamento agrícola;
- tanque de combustível contextualizado;
- quatro porcos distribuídos em profundidade.

Posições autorais são congeladas no manifesto; nenhuma composição depende de node físico Godot.

### 18.3 Cadeia principal

1. uma travessa de madeira sustenta um contrapeso de chapa;
2. a ruptura libera a chapa;
3. a chapa quebra o vidro;
4. a abertura libera fardos numa rampa;
5. os fardos atingem a argamassa lateral do silo;
6. o silo inclina e transfere carga à plataforma;
7. a área de equipamento oferece uma rota alternativa pela detonação da Preta ou pelo tanque contextualizado.

Nenhum elo recebe impulso roteirizado. A cadeia emerge de gravidade, juntas, contato e pressure burst.

### 18.4 Fila

1. Vermelha;
2. Amarela;
3. Azul;
4. Preta.

A fase introduz cada poder quando a ave chega ao estilingue. A quarta ave também funciona como recuperação.

### 18.5 Balanceamento

- duração de primeira vitória: 5–8 min;
- solução mínima estável: duas aves;
- solução comum: três;
- quatro aves garantem margem de recuperação;
- cada porco é alcançável por pelo menos duas orientações do plano;
- cadeia principal reproduz o mesmo resultado em pelo menos 8/10 replays idênticos antes do congelamento final; após congelamento, 50/50 replays devem ser idênticos;
- completar é acessível; três estrelas exigem eficiência;
- materiais nunca são diferenciados somente por cor.

Rotas normativas:

- tutorial: quatro aves, vitória de uma estrela;
- cadeia eficiente: até duas aves, três estrelas;
- alternativa: Azul/Preta, até três aves, pelo menos duas estrelas;
- derrota: quatro lançamentos sem neutralizar todos os porcos.

## 19. Explosivos ambientais

Tanque e recipiente pressurizado usam PressureBurstDefinition:

- trigger por dano/temperatura;
- fuse opcional;
- raio, impulso, energia, line-of-sight, máximo de corpos e cooldown autorados;
- candidatos ordenados por IDs;
- detonação única;
- evento causal aponta para o impacto/ability que iniciou o trigger;
- fragmentos físicos continuam sujeitos à gravidade;
- não existe caixa explosiva sem contexto visual.

## 20. Score, cadeia e estrelas

Score é calculado no C++ apenas a partir de eventos canônicos.

Valores iniciais:

- porco neutralizado: 5.000;
- palha rompida: 80;
- madeira: 120;
- vidro: 160;
- tijolo/argamassa: 180;
- chapa yield/ruptura: 220;
- ave não usada após vitória: 10.000.

Uma entidade/peça pontua somente na primeira transição válida. Fragmentos cosméticos, contatos repetidos, TTL e limpeza não pontuam.

Eventos pontuáveis na mesma árvore causal e separados por no máximo 45 ticks formam cadeia:

~~~text
multiplier_percent = 100 + 10 × min(chain_index - 1, 10)
awarded = floor((base × multiplier_percent + 50) / 100)
~~~

Cap: 2,0×. A cadeia encerra após 45 ticks sem evento pontuável ou no próximo lançamento.

Estrelas:

- derrota: zero;
- qualquer vitória: ao menos uma;
- Fazenda: duas em 38.000 e três em 50.000;
- Orbital recebe thresholds congelados após recaptura das rotas existentes;
- score e estrelas fazem parte de canonical_state_v3;
- save grava somente ResultVictory e nunca reduz recordes.

## 21. Save local

Arquivos:

- user://progress.v2.json;
- user://progress.v2.json.bak;
- user://settings.v2.json;
- user://settings.v2.json.bak.

Progress:

- schema_version;
- profile_id;
- mundos/fases disponíveis;
- completed, best_score, best_stars, best_birds_used e completion_count por fase;
- briefings/tutorials vistos;
- last_world_id e last_level_id.

Settings:

- volumes;
- reduced_motion;
- shake;
- trajectory_assist;
- camera_sensitivity;
- UI scale 100/125/150/200;
- bindings semânticos.

Escrita:

1. serializar em .tmp no mesmo diretório;
2. flush;
3. mover principal válido para backup;
4. promover temporário;
5. recuperar backup se o principal falhar;
6. preservar arquivos corrompidos com sufixo diagnóstico e iniciar defaults seguros se ambos falharem.

Os dois mundos e suas primeiras fases continuam disponíveis em perfil novo ou recuperado. Save nunca entra no hash da simulação.

## 22. App shell e UI

Fluxo:

~~~text
Aviso de fan game
  → Menu
      → Continuar
      → Mundos
          → Carrossel 3D
              → Fases
                  → Briefing curto
                      → Gameplay
                          → Resultado
      → Opções
      → Sair
~~~

Carrossel:

- Terra central/selecionada no primeiro acesso;
- Orbital à direita;
- diorama central em LOD alto;
- vizinhos em LOD1;
- no máximo três cards e dois rigs animados.

HUD:

- porcos/objetivo no alto à esquerda;
- score e multiplicador no alto à direita;
- fila da ave atual + três próximas embaixo à esquerda;
- estado CARREGANDO/PRONTA/ATIVA/USADA;
- indicação PLANO TRAVADO;
- trajetória pontilhada;
- pausa, reinício, recenter e opções.

Resultado:

- vitória/derrota;
- score;
- estrelas;
- novo recorde;
- reiniciar;
- seleção de fases;
- continuar.

Nenhuma string visível fica hardcoded. Catálogos pt-BR permanecem fechados e fail-closed.

## 23. Input e acessibilidade

InputRouter é a única camada que conhece mouse/teclas físicas. Intenções:

- navigate;
- accept;
- back;
- orbit;
- zoom;
- begin_grab;
- update_pull;
- release;
- activate_ability;
- recenter;
- pause;
- restart.

Mouse e teclado são oficiais na 2.0. As intenções aceitam uma futura fonte de controle sem mudar gameplay.

Opções:

- movimento reduzido;
- shake desligável;
- trajetória ampliada;
- escala de UI;
- sensibilidade;
- remapeamento;
- áudio por buses;
- foco visível e navegação completa por teclado;
- informação de material/estado não depende somente de cor;
- contraste WCAG ≥4,5:1 para texto normal e ≥3:1 para texto grande/ícones.

## 24. Câmeras

CameraDirector seleciona um perfil data-driven.

- Carrossel: FOV 35°, transição 0,45 s; movimento reduzido usa fade/cut de 0,1 s.
- Terra: FOV 44°, sem roll, safe framing entre estilingue, projétil e alvos.
- Orbital: preserva FOV 48°, margem segura 8% e regras de oclusão do planeta.
- Impacto: foco máximo 0,6 s e kick máximo 2°; ambos desabilitados por movimento reduzido.
- Bounds de composição vêm do manifesto, não de constantes de uma fortificação.
- Durante Grabbed, yaw/pitch/zoom não mudam.

## 25. Arte Blender

### 25.1 Pipeline

- Blender 5.1.2 pinado;
- sources .blend versionados;
- GLB, LODs, COL, FRAG, SOCKET e RIG validados;
- hash semântico inclui objetos, meshes, UVs, materiais, armatures, pesos, actions, markers e custom properties;
- seed, autoria, licença/proveniência e hash registrados;
- pivô físico no centro de massa e VIS_ROOT para offset artístico;
- Blender em metros/Z-up/-Y; runtime em metros/Y-up/-Z;
- mesh visual nunca é collider implícito.

### 25.2 Personagens

Assets:

- CHR_Red;
- CHR_Yellow;
- CHR_Blue;
- CHR_BlueChild;
- CHR_Black;
- CHR_Virela preservada;
- ENM_PigFarm;
- ENM_Anchor preservado.

Por ave nova:

- LOD0/1/2: até 26k/12k/4k triângulos;
- até 44 bones;
- até quatro influências por vértice;
- até três materiais;
- albedo/normal 1024² e ORM 512²;
- até quatro blend shapes faciais;
- root motion zerado;
- animações a 30 fps.

Clips comuns:

- Idle_A;
- Idle_B;
- Select;
- Aim_Charge;
- Launch;
- Flight_Loop;
- Impact_Light;
- Impact_Heavy;
- Stun;
- Victory;
- Defeat.

Clips exclusivos:

- Ability_Mass;
- Ability_Speed;
- Ability_Split;
- Ability_Explosion.

Porco:

- Idle;
- Alert;
- Impact;
- Squash;
- Stunned;
- Defeat;
- sem animação gráfica.

### 25.3 Mundo e dispositivos

Assets mínimos:

- WRD_EarthFarm_Diorama;
- WRD_Aster_Diorama;
- TER_EarthFarm_Ground;
- BLD_Farm_Barn;
- BLD_Farm_Silo;
- BLD_Farm_Windmill;
- BLD_Farm_Glasshouse;
- KIT_Farm_Fence_A/B;
- KIT_Farm_Wood;
- KIT_Farm_Glass;
- KIT_Farm_Brick;
- PROP_Farm_HayBale_A/B;
- PROP_Farm_Tractor;
- PROP_Farm_Wagon;
- DEV_Farm_FuelTank;
- DEV_Farm_PressurizedTank;
- DEV_SlingshotFarm;
- DEV_SlingshotOrbital.

As tiras do estilingue ligam sockets ao ghost da ave e são deformadas visualmente. Elas não são solver autoritativo.

### 25.4 Budgets

| Recurso | Limite |
|---|---:|
| Triângulos visíveis | 300.000 |
| Draw calls | 180 |
| Texturas RGBA8 + mips | 128 MiB |
| Partículas | 30.000 |
| Fragmentos ativos | 120 |
| Skinned vertices ativos | 35.000 |
| Rig completo em gameplay | 1 |
| Rigs animados no carrossel | 2 em LOD1 |
| Luzes com sombra | 1 direcional |

LODs trocam por ocupação de tela com 15% de histerese.

## 26. Áudio e VFX

Áudio:

- WAV mono 48 kHz PCM16 para SFX 3D;
- OGG estéreo streamed para música/ambience;
- pico ≤-1 dBFS;
- música -16 LUFS-I;
- ambience -24 LUFS-I;
- UI/SFX -20 a -14 LUFS-I;
- até 24 vozes e três instâncias por cue;
- memória decodificada não streamed ≤32 MiB;
- primeiro impacto com feedback <150 ms, alvo 80 ms.

Cues mínimos:

- menu, mundos, Fazenda calma/ação, vitória e derrota;
- ambience de dia, celeiro, silo e moinho;
- foco/confirmar/voltar/rotação de mundo;
- tensão, release e snap dos dois estilingues;
- seleção, lançamento, habilidade, impactos e celebração por ave;
- reações do porco;
- impactos leves/pesados e ruptura por cinco materiais;
- fuse, pressure burst e detonação;
- hum/campo da Virela preservados.

VFX:

- massa: compressão visual/onda densa;
- velocidade: trail amarelo e compressão de ar;
- divisão: três rastros legíveis;
- explosão: flash curto, shockwave, fumaça e detritos;
- vidro, madeira, tijolo, palha e chapa com perfis próprios;
- cobertura do primeiro impacto por partículas ≤150 ms;
- movimento reduzido remove shake/kick, não feedback causal.

Nenhum áudio da franquia é extraído ou reutilizado sem licença.

## 27. Roadmap de aves aprovado

Estas aves recebem conceito e contrato preliminar, mas não assets/implementação na 2.0:

| Ave | Poder | Valores iniciais |
|---|---|---|
| Branca | Ovo cinético | ovo 35 kg a 12 m/s na gravidade local; recuo compensa momento |
| Verde | Retorno | giro de 160° em 18 ticks no plano, velocidade em ±5% |
| Laranja | Expansão | raio até 3× em 24 ticks, massa constante e impulso limitado |
| Prateada | Mergulho | 30 m/s² na gravidade por 21 ticks, cap 42 m/s |
| Magnética | Campo metálico | raio 5 m, 54 ticks, 12 corpos e 10 m/s² |
| Gélida | Choque térmico | burst 2,5 m, tenacidade a 35% por 120 ticks |

Todas usam uma ativação e a gravidade local quando houver direção.

## 28. Fan game e publicação

Decisão de produto: fan game assumido.

Consequências:

- primeira execução e tela Sobre exibem “fan game não oficial, sem afiliação ou endosso”;
- o manifesto de assets 2.0 não declara clean-room para personagens clássicos;
- o parecer clean-room v1 permanece histórico e limitado ao slice legado;
- models, rigs, animações, texturas, áudio e código produzidos pelo projeto mantêm proveniência própria;
- nenhum asset oficial é copiado para o repositório sem licença explícita;
- engenharia aprovada não equivale a autorização de uso de propriedade intelectual;
- publicação/distribuição pública é um gate humano separado e permanece bloqueada até revisão jurídica/autorização aplicável.

## 29. Critérios técnicos de aceite

- 50 replays idênticos preservam outcome, score, estrelas, eventos e hash.
- Debug e Release produzem a mesma sequência canônica.
- plano fica bit-identical do grab ao release.
- release abaixo da deadzone não consome ave.
- release válido consome uma única entrada.
- mouse-up nunca ativa habilidade.
- preview/impacto diverge no máximo 0,10 m em cena estática.
- toda entidade dinâmica recebe a gravidade do mundo.
- Vermelha preserva velocidades ao mudar massa.
- Amarela preserva direção e respeita cap.
- Azul conserva massa e momento nas tolerâncias.
- Preta detona exatamente uma vez.
- Virela preserva o contrato orbital.
- score não duplica eventos.
- save interrompido preserva principal ou backup.
- Terra e Orbital estão disponíveis num perfil novo.
- zero nodes de física Godot autoritativos.
- 20 reinícios não aumentam bodies, joints, nodes ou bytes Box3D.
- physics step p95 ≤8 ms.
- frame p95 ≤16,67 ms e p99 ≤25 ms.
- nenhum hitch >50 ms.
- input→feedback p95 ≤33,4 ms.
- Vulkan e OpenGL em 1920×1080.
- UI 100% e 150% sem clipping.
- stderr vazio e nenhum ERROR, SCRIPT ERROR ou WARNING.

## 30. Playtest humano

Oito participantes que não tenham jogado a build:

- 7/8 agarram e lançam em até 45 s sem instrução verbal;
- 7/8 entendem que a câmera muda o plano em até três tentativas;
- 7/8 ativam uma habilidade corretamente;
- 0/8 ativa poder acidentalmente no release;
- 6/8 provocam ao menos três elos da cadeia;
- 7/8 vencem a Fazenda em até oito minutos;
- 6/8 relacionam corretamente três das cinco respostas de material;
- 7/8 encontram os dois mundos sem ajuda;
- mediana da primeira vitória: duas estrelas;
- taxa de três estrelas após até três tentativas: 15–35%;
- zero relato de violência gráfica excessiva.

Playtest técnico por agentes não substitui participantes humanos.

## 31. Marcos

1. Congelar regressão orbital e contratos v2.
2. Generalizar gravidade, bounds, shapes e conteúdo.
3. Implementar plano guiado, Hooke e GameplaySessionNode.
4. Implementar grupo de projéteis e quatro habilidades.
5. Implementar materiais, porco, pressure burst, score e Fazenda headless.
6. Implementar app shell, save, carrossel, HUD e câmeras.
7. Produzir arte, animação, áudio e VFX finais.
8. Migrar a fase Orbital de produto para estilingue e preservar Virela.
9. Balancear, otimizar, playtestar e certificar Debug/Release.

Cada marco termina verde, com commit atômico e revisão independente.

## 32. Auto-revisão

- Não há decisão pendente marcada como TBD.
- A fila fixa é compatível com a divisão da Azul.
- A Preta tem uma única fonte de detonação efetiva.
- O release e o clique de habilidade não compartilham o mesmo evento.
- A fórmula de energia do estilingue usa eficiência dimensionalmente correta.
- O plano usa camera_right, coerente com uma câmera lateral.
- As massas influenciam realmente a velocidade.
- A gravidade é aplicada a todos os corpos dinâmicos nos dois mundos.
- O Orbital inicial mantém Virela.
- Somente uma fase terrestre e uma orbital fazem parte da 2.0.
- As seis aves adicionais permanecem roadmap, não escopo oculto.
- O fixture v1 e a certificação histórica não são sobrescritos.
- O status de fan game não é apresentado como autorização jurídica.
