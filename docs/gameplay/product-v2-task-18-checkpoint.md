# Task 18 — checkpoint de playthroughs determinísticos

**Data da pausa:** 21 de julho de 2026

**Branch:** `codex/game-2-0`

**Baseline de código antes deste checkpoint documental:** `b089315`

**Status:** pausada; nenhuma rota de vitória ou geometria experimental foi aceita

## Estado seguro para retomada

- O nível `farm_reaction.level.json` foi restaurado byte a byte ao blob Git
  `03996ef711da7092b97bd7f2673dcd0a9be1eac7`.
- O layout aprovado permanece em 72 bodies, 54 dynamic, 52 joints, oito
  assemblies, quatro porcos e dois triggers, com
  `layout_hash=fnv1a64:8324ce8a44754dad`.
- `product_v2_level_tests.cpp` não conserva instrumentação nem contagens das
  experiências.
- Nenhum processo de busca, CTest, Ninja, CMake, Godot ou Blender ficou aberto.
- Os stashes externos da Task 11 continuam preservados e não devem ser
  aplicados nem removidos durante a retomada.

## Trabalho parcial preservado

O stash local abaixo contém somente o WIP da Task 18; ele não faz parte do push:

```text
e70f471db82340484559720d063de2686e7271ca
wip-task18-playthrough-search-2026-07-21
```

Conteúdo do stash:

- `ordered_events_v3` e `canonical_playthrough_v5` em
  `playthrough.hpp/.cpp`;
- testes RED de playthrough e determinismo;
- helper `ninho_product_v2_route_search`, somente com `BUILD_TESTING` e sem
  registro em CTest;
- integração CMake correspondente.

Aplicar pelo hash, sem `pop`, para manter uma cópia recuperável:

```powershell
git stash apply e70f471db82340484559720d063de2686e7271ca
```

Antes de continuar, confirmar que nenhum dos stashes externos foi incorporado.

## Resultados já comprovados

### Infraestrutura determinística

- `ordered_events_v3` usa registro fixo de 274 bytes por `DomainEvent`, inclui
  todos os payloads estendidos e preserva ordem.
- `canonical_playthrough_v5` normaliza entradas e incorpora os bytes completos
  de eventos e estado terminal.
- O helper usa somente comandos públicos `BeginGrab`, `SetPull`, `ReleaseBird`
  e `ActivateAbility`; o tick de habilidade foi corrigido para o tick solicitado
  exato.
- Esses arquivos ainda são WIP: os sete fixtures não existem e os testes não
  devem ser conectados a um checkpoint verde antes de as rotas serem reais.

### Rota Farm derrota

Foi encontrada uma rota pública robusta com quatro disparos idênticos:

```text
camera_right = {1,0,0}
pull_horizontal_m = 0
pull_vertical_m = -4
ability = null
```

Resultado observado em Release: quatro launches válidos, zero aves restantes,
`Defeat`, score 0, stars 0, zero porcos neutralizados, zero dano, zero rupturas
e zero fraturas. Vizinhos `pull_horizontal_m=±0,02` preservaram o resultado.

## Causa raiz da cadeia original

A investigação descartou A–G e restaurou cada experiência. Os fatos relevantes
são:

1. cada fardo pesa aproximadamente 694 N e já consome cerca de 87% do limite
   de um `straw_bind` de 800 N;
2. os três welds originais prendem os fardos ao body 20 e impedem a liberação
   natural;
3. o body 26 original fica preso por `pine_fit` de 5.500 N e não converte o
   impacto do contrapeso em movimento útil;
4. Box3D combina os atritos por `sqrt(frictionA*frictionB)`, resultando em
   `μ=0,6715653` e ângulo crítico `33,88394°` para palha/pinho;
5. a rampa original de `-12°` desacelera os fardos e não os transporta
   passivamente até o silo;
6. a grade do layout original não produziu contato vidro → feno → rampa nem
   vitória do porco 3.

Não reduzir atrito, força ou torque para fazer uma rota passar. A solução deve
continuar emergindo de gravidade, contato, massa e geometria.

## Mecanismo J-v3 investigado e revertido

J-v3 foi a melhor topologia física encontrada, mas não é conteúdo aprovado.
Ela passou parsing, bounds, SAT/all-pairs, grafo e idle120 em Debug/Release;
falhou na rota causal e foi integralmente removida.

### Tuple suficiente para reprodução

- body 26: plataforma livre de pinho, box `half=[0.6,
  0.0190909090909091,1.1]`, local `[1,-0.239090909090909,0]`, mantendo
  volume `0,1008 m³` e massa `52,416 kg`;
- body 13: fulcro primário world center `[7.60,2.92181818181818,0]`,
  `half=[0.15,0.02,0.08]`, edge direita `x=7.75`; duas guias em
  `[7.60,2.88181818181818,±0.45]`, `half=[1.2,0.02,0.35]`;
- body 13: calha estática de superfície superior a `-40°`, do ponto
  `[8.2585927597,2.5352579083]` até `[10.32,0.8055318533]`, box
  `half=[1.34548801888,0.03,1.1]`, center
  `[9.27001275157,1.64741354748,0]` e quaternion Z/W
  `[-0.342020143326,0.939692620786]`;
- body 20: base baixa world center `[9.025,0.15,0]`,
  `half=[1.775,0.15,0.5]`; local center
  `[0.5631375000144092,-1.5671634107767833,0]`, rotação local `+12°` e
  visual bounds `[3.5347974898503383,2.237345556967495,1.0]`;
- novo body 73/entity 3073: lingueta dinâmica de pinho, transform
  `[6.8,2.91181818181818,0]`, massa `16,6608 kg`; head
  `half=[0.06,0.02,1.1] local=[1.54,0.01,0]`, duas hastes
  `half=[0.74,0.01,0.05] local=[0.74,0,±0.15]`, duas faces de ataque a
  `+42°` com `half=[0.25,0.01,0.35] local=[0,0.174714099844,±0.45]` e
  dois conectores `half=[0.015,0.13,0.05] local=[0.1,0.13,±0.15]`;
- remover joints 13–15; joint 18 vira `straw_bind` body20→body73 com os
  limites normativos 800 N/90 N·m; bodies 21–23 e 26 ficam livres;
- assembly 3 fica com bodies `[20,24,25,73]` e joints `[16,17,18]`;
  inventário experimental: 73 bodies, 55 dynamic, 49 joints e 16 free.

O envelope idle correto, revalidado contra `B3_LINEAR_SLOP=5 mm`, é:

- deslocamento total authored→tick120 ≤2,5 mm e sempre <5 mm;
- deriva tick90→120 ≤0,25 mm;
- rotação total ≤0,1°;
- velocidades finais de repouso e zero overload/break/objective;
- 50/50 Debug/Release antes de congelar qualquer fixture.

### Falha funcional observada

No seed Red `yaw=-10°, pull=(-4,-0.4), ability=10` seguido de Yellow
`yaw=-1°, ext=4 m, elev=10°, ability=18`, joint 8 rompeu nos ticks
1536–1537, mas o contrapeso colidiu com bodies 13, 18, 16 e 14, desviando-se
para `x≈5,3`. A menor separação SAT do OBB do body 19 para as faces do body 73
ficou positiva em `0,553532 m`; não houve contato nem ruptura do latch.

A grade coarse definida foi:

```text
yaw      = [-6,-3,0,3,6]
extensão = [3.8,4.05,4.25]
elevação = [5,12.5,20]
ability  = [10,22,34]
```

O candidato lexicográfico 28 (`yaw=-3°, ext=3,8 m, elev=5°, ability=10`)
rompeu joint 18, porém moveu body 73 apenas `0,0497 m`, abaixo dos `0,18 m`
necessários; não houve `body19→body73`, nem rampa→silo. É falso positivo.
A busca reiniciada com predicado correto foi interrompida por solicitação do
usuário antes de avaliar os candidatos restantes.

## Ordem recomendada de retomada

1. Aplicar o stash WIP pelo hash e reconstruir o helper em Release.
2. Reproduzir J-v3 exatamente, primeiro com os gates estruturais e idle acima.
3. Fazer o helper aceitar apenas:
   - `JointBroken(18)`;
   - deslocamento incremental de body 73 ≥0,18 m em até 12 ticks;
   - causa que passe pelo portão/contrapeso, rejeitando impacto direto da
     Yellow na lingueta;
   - depois body26→ao menos dois fardos→calha→body24/25→silo/porco.
4. Retomar a grade em ordem lexicográfica; reexecutar 1–28 para provar
   determinismo antes de continuar do 29.
5. Se os 135 candidatos falharem, rejeitar J-v3 formalmente. Usar a trace OBB
   já implementada no helper para redesenhar a pá de impacto, sem mexer em
   thresholds, atritos ou massas.
6. Só depois encontrar as demais rotas Farm/Orbital, gerar fixtures completos,
   provar 50/50 Debug/Release e solicitar revisão independente.

Tasks 19–32 permanecem bloqueadas pelo marco D até a Task 18 ficar verde.
