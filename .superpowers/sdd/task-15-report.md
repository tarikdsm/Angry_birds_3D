# Task 15 — Relatório de implementação

## Escopo e base

- Base confirmada antes da implementação: `67bb94d0053a9de25aace79bc24c4ae2f89f1349`.
- Branch: `codex/game-2-0`.
- Escopo: materiais terrestres, juntas dúcteis, dano do porco terrestre, esmagamento, `BoundsExit`, fragmentos físicos autorados, canonical/adapters e documentação.
- Fora do escopo: Fazenda final, score e assets finais.

## RED — falhas observadas antes das correções

1. Os primeiros testes de materiais/porco não compilavam porque `CrushDamageSystem`, `DuctileJointSystem`, os novos enums e a API de substituição de junta ainda não existiam.
2. O teste de capacidade máxima mostrou que destruir e recriar uma junta não atendia ao contrato de handle estável nem de substituição atômica.
3. A regressão mínima `capability weld joint accepts dynamic endpoint before static endpoint` reproduziu uma interrupção Box3D (`weld_joint.c:94`, `bodyB->setIndex == b3_awakeSet`) quando uma weld recebia o endpoint dinâmico como A e o estático como B.
4. O teste de energia raw do porco retornou nenhum evento/dano: a deadzone da energia derivada causava retorno antecipado antes da fórmula terrestre.
5. O teste da recriação dúctil mostrou que limpar `pending_recreate` antes de `replace_joint` violava atomicidade se a substituição falhasse.
6. O teste de massa com tetraedro expôs o cálculo por AABB em vez do volume real do convex hull.
7. O teste semântico aceitou inicialmente um porco tipado cuja massa física não era `65 kg ±0,065 kg`.

## GREEN — solução entregue

### Materiais e conteúdo

- `MaterialResponse` recebeu `Compressible=3` e `Ductile=4`, sem renumerar valores existentes.
- `JointKind` recebeu `StrawBind=3` e `SteelDuctile=4` em parser, serializer, validação, canonical e documentação.
- Catálogo terrestre implementado com densidade, atrito, restituição e thresholds contratados para pinho, vidro, tijolo, palha e chapa.
- Fibrous/Masonry/Compressible usam energia acumulada; Brittle usa pico individual; Ductile não passa pela fratura comum.
- O volume de convex hull agora usa geometria física real, inclusive para shapes rotacionadas/compostas, em vez da caixa envolvente.

### Juntas dúcteis

- Novo `DuctileJointSystem`, ordenado por `JointId`, com estados `Elastic`, `Yielded` e `Broken`.
- Yield em `>=3200 N` ou `>=450 N·m`; ruptura somente após recriação e solver posterior em `>=7500 N` ou `>=900 N·m`.
- Carga extrema nunca salta diretamente de Elastic para Broken.
- A recriação é agendada para o tick seguinte, conserva frames locais e usa `PhysicsWorld::replace_joint` sob o mesmo handle, sem consumir capacidade extra.
- `pending_recreate` só é limpo após a substituição ter sucesso.
- A fronteira Box3D normaliza welds com endpoint dinâmico antes do não dinâmico, trocando também os frames; isso elimina a asserção sem mudar a semântica pública.

### Porco terrestre e esmagamento

- Novo contrato tipado `EnemyDamageModel::TerrestrialPig`; nenhuma inferência por nome, mesh ou visual.
- Validação semântica exige compound físico de `65 kg ±0,065 kg`, superfície `μ=0,65`, restituição `0,05`, weakpoint uniforme e ausência de padrão de fratura.
- Contato usa diretamente `0,5 × effective_mass × normal_speed²`, sem a deadzone legada; `18 J/kg` é inclusivo sem dano e `18,001 J/kg` inicia dano.
- Explosão e esmagamento entram no mesmo `DamageSystem`, preservando cap, integridade e deduplicação causal.
- Novo `CrushDamageSystem`: soma de impulso por porco/tick, razão quantizada em `1e-5`, comparação estrita `>4`, janela de 21 ticks, reset por interrupção e proteção para `g=0`, massa/body inválidos ou neutralizados.
- Ordem causal preservada: `CrushDamageApplied → DamageApplied → EntityNeutralized`.
- `NeutralizationCause::BoundsExit=3` foi anexado; mundo uniforme usa BoundsExit e mundo radial mantém Ejection, ambos exatamente uma vez.

### Fragmentação, canonical e adapters

- Padrões de fratura físicos são exclusivamente autorados e validados: massa dos filhos dentro de `0,1%`, máximo global de 80 e proibição para inimigos.
- Substituição do pai por filhos ocorre antes do próximo solver step, com IDs determinísticos, `v_child = v_parent + ω×r`, velocidade angular e gravidade desde o primeiro step.
- Fragmentos cosméticos não entram no kernel, `DamageState` ou identidade física.
- Canonical v3 ganhou blocos condicionais para ductilidade e crush; mutation probes provam mudança de hash por estado relevante e omissão quando o recurso não existe.
- Eventos append-only `MaterialYielded=21` e `CrushDamageApplied=22` foram mapeados nos adapters e preservados no `SessionFrameBatch`, sem `unknown`.

## Goldens e compatibilidade

- Canonical v2 legado preservado: `16778877272428821006`.
- Preview orbital legado preservado: `15211795551651615890`.
- Launch FSM golden preservado: `17815016211264547523`.
- Determinism state golden preservado: `8671678994761298067`.
- A suíte completa também preservou Javali-Âncora, fracture/objectives, gatilhos e habilidades das Tasks 11–14.

## Gates executados

- Debug build com GDExtension: `tools/build.ps1 -Configuration Debug -WithGodot` — exit 0.
- Debug completo: `ctest --preset debug --output-on-failure` — **64/64 testes passaram**, 0 falhas, 399,43 s.
- Release build com GDExtension: `tools/build.ps1 -Configuration Release -WithGodot` — exit 0.
- Release focado, incluindo materiais, porco, regressões de dano/fratura/Task 14, conteúdo v2, adapters e capability: **11/11 testes passaram**, 0 falhas, 3,89 s.
- `git diff --check` — exit 0; apenas aviso de normalização LF→CRLF do arquivo CMake, sem erro de whitespace.
- Busca por marcadores temporários de diagnóstico Box3D — nenhum resultado.

## Observações

- Nenhum push foi realizado.
- A revisão independente deve ocorrer após o commit; eventuais correções devem ser commits separados, conforme o brief operacional.
