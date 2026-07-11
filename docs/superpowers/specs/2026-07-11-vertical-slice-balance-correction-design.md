# Vertical Slice — Correção de balanceamento físico

**Data:** 11 de julho de 2026

**Status:** aprovado por delegação autônoma do usuário

**Escopo:** corrigir as duas rotas físicas da Task 7 sem alterar a arquitetura do kernel, usar façades de teste para vencer ou enfraquecer a identidade de Virela e do Javali-Âncora.

## 1. Evidência que motiva a correção

Os playthroughs públicos reproduzem derrota nas duas rotas de vitória. A investigação sistemática confirmou que o pipeline Box3D → energia → dano calcula os valores esperados, mas o conteúdo não entrega energia suficiente:

- o layout original não coloca corpos elegíveis a menos de 4 m da Virela durante os 75 ticks;
- atrasar a ativação produz corpos afetados, porém com apenas aproximadamente 0–2,2 m/s;
- impactos observados geram 755–1.062 J, abaixo dos 2.469,6 J exigidos pelo vidro atual;
- três hipóteses isoladas — velocidade 16 m/s, ativação atrasada e encontro antecipado a 14 m/s — não produzem vitória física.

A causa é de balanceamento e composição espacial, não de fórmula, determinismo ou integração com Box3D.

## 2. Direções consideradas

### 2.1 Funil de energia por conteúdo — escolhida

Reposicionar a fortificação em uma janela orbital interceptável, orientar o flanco vulnerável do Âncora para as linhas de solução e recalibrar dados de vidro/juntas/dano somente até o menor conjunto que faça as duas rotas passarem. Mantém Virela com 9/75 ticks, raio 4 m, aceleração 12 m/s² e pulso 4 m/s.

### 2.2 Aumentar força ou raio de Virela — rejeitada

Resolveria o slice, mas mudaria o comportamento sistêmico da ave e contaminaria os marcos 3/3 e 20 materiais.

### 2.3 Reduzir drasticamente integridade do Âncora — rejeitada

Tornaria impactos triviais e reduziria a importância da proteção frontal/lateral que o slice precisa validar.

## 3. Design escolhido

O nível passa a oferecer duas linhas intencionais:

1. **Rota Virela:** a trajetória pública cruza o volume de 4 m de um conjunto de corpos elegíveis de até 150 kg. A ativação real produz `AbilityStarted`, `AbilityAffectedBody`, `AbilityPulse` e `AbilityEnded`; os corpos liberados atingem o flanco exposto ou abrem uma linha para impactos subsequentes. A neutralização ocorre por dano/ejeção do runtime, nunca por façade.
2. **Rota estrutural:** uma trajetória pública atinge vidro/junta, produz `JointOverloaded`, `JointBroken` e `PieceFractured`, e abre uma linha física para neutralização do Âncora em no máximo três aves.

A derrota usa três lançamentos públicos sem concluir o objetivo.

## 4. Limites de recalibração

Permanecem imutáveis:

- armamento 9 ticks, campo 75 ticks, raio 4 m, massa elegível ≤150 kg, aceleração 12 m/s² e pulso 4 m/s;
- massa do Âncora 480 kg, integridade 100, teto 55, cone 45° e multiplicadores 0,25/1,0;
- densidades, atrito, restituição, contagem de corpos, materiais, aves, IDs, arco e velocidades públicas 8–16 m/s;
- fórmulas de energia, dano, ejeção e ruptura;
- PhysicsWorld como única fronteira Box3D e SimulationSession como autoridade.

Podem mudar, nesta ordem de preferência:

1. transforms/orientações e dimensões de proxies do nível, preservando o envelope e as contagens;
2. limites positivos das juntas, preservando a ordem de resistência `pine_fit > mortar > glass_clamp` quando aplicável à composição final;
3. tenacidade do vidro, mantendo resposta `brittle` e exigindo um único pico;
4. `damage_energy_j_per_kg` do Âncora, somente se as três categorias anteriores ainda deixarem três impactos laterais físicos abaixo de 100.

Os valores finais são escolhidos por busca limitada, reprodutível e lexicográfica: primeiro minimiza quantidade de campos alterados; depois minimiza a variação relativa total em relação à especificação anterior; por fim desempata pelo menor tuple JSON canônico. O relatório registra todos os candidatos testados e o primeiro tuple que passa. Não há busca ilimitada nem ajuste depois de observar Debug/Release divergentes.

## 5. Eventos e causalidade

`JointOverloaded` representa a causa mecânica autoritativa da ruptura, mesmo sem dano anterior. `JointBroken` e `PieceFractured` apontam por `cause_event_id` para exatamente um `JointOverloaded` anterior no stream agregado. Hooks não fabricam `DamageApplied`.

Cada neutralização continua emitindo causa tipada, posição, normal e energia. Playthroughs não podem chamar `neutralize_entity`, `request_fracture`, `finish_projectile` nem mutar objetivo/outcome.

## 6. Testes e aceite

A correção é aceita quando:

- as duas vitórias e a derrota usam somente comandos públicos e ticks físicos;
- a rota Virela contém todos os quatro eventos da habilidade e ao menos um corpo afetado real;
- a rota estrutural contém a cadeia overload → break → fracture com IDs resolvíveis;
- a neutralização é produzida pelo sistema de dano/ejeção, e vitória ocorre apenas em `Evaluation`;
- o stream canônico é idêntico em dez repetições e entre Debug/Release;
- conteúdo/parsers, Task 5, Task 6 e a suíte completa permanecem verdes;
- os valores finais e a evidência da busca limitada são registrados no relatório da Task 7 e refletidos na especificação principal.

## 7. Escopo posterior

Esta correção não cria novos materiais, habilidades, inimigos, UI, VFX, assets ou regras de pontuação. Tasks 8–12 continuam inalteradas depois do gate da Task 7.

## 8. Fallback autoral após a busca limitada

A primeira busca foi encerrada após 464 simulações completas e nenhum tuple aprovado. O melhor caso entregou 160,817 J ao vidro contra 926,1 J mínimos e 1,82361/100 de dano acumulado ao Âncora. Isso prova que pequenas rotações, offsets e ajustes nos intervalos anteriores não fecham o orçamento de energia.

O fallback aprovado substitui a pilha genérica por um **pórtico de contrapesos suspensos**, preservando IDs, contagens e materiais:

- os nove tijolos tornam-se contrapesos de `138–145 kg` cada, ainda elegíveis para Virela;
- eles formam três colunas acima e no flanco exposto do Âncora, com queda radial útil de `1,5–2,5 m` e envelope local máximo de `4,5 m`;
- três painéis de vidro funcionam como pinos de liberação; `glass.toughness = 0,01`, mantendo resposta frágil por pico único;
- joints de argamassa do contrapeso usam `950 N / 160 N·m`; pine fits e glass clamps mantêm seus valores salvo incoerência geométrica demonstrada por teste;
- o Âncora começa a correção em `damage_energy_j_per_kg = 40`, preservando massa 480 kg, integridade 100, teto 55, cone 45° e multiplicadores 0,25/1,0;
- o pórtico é orientado para que a linha estrutural atinja um pino e a linha Virela atravesse o volume dos contrapesos antes do pulso.

Esses valores formam o primeiro orçamento fechado. Até 500 simulações adicionais escolhem transforms e comandos públicos dentro do arco/velocidade existentes.

A investigação do fallback provou que nenhuma trajetória pública com velocidade `≥12 m/s` cruza o raio útil `10,8–12,5 m`; os impactos de 8 m/s fecham fratura, mas o melhor dano observado é `6,54748/100` com `40 J/kg`. Portanto o último eixo data-driven é autorizado como resgate discreto: testar `damage_energy_j_per_kg ∈ {40, 20, 10, 5, 2,5}` e congelar o **maior** valor que faz ambas as rotas vencerem. Nenhum valor intermediário ou menor que `2,5` é permitido. Integridade, teto e multiplicadores continuam imutáveis, preservando dois ou mais contatos quando o teto de 55 se aplica.

O tuple final deve provar massa individual dos tijolos, envelope, estabilidade pré-lançamento, ausência de eventos de ruptura antes de input e as duas rotas físicas completas.
