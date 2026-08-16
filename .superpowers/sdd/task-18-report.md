# Task 18 — relatório final

## Resultado

A Task 18 congela sete playthrough fixtures determinísticos e o layout físico
final da Fazenda. O layout farm_reaction usa fnv1a64:ee2ad0b8c2912aa: 106
bodies (86 dynamic, 20 static), 49 joints, oito assemblies, 49 free bodies, 256
primitivas autoradas expandidas e 68 fragmentos físicos autorados. A ASM_HayRamp
contém bodies [20,24,25,73] e joints [16,17,18].

## TDD causal

Os testes fixaram a evidência observável antes do congelamento: rota pública,
estado terminal, score, estrelas, aves restantes, objetivos, stream ordenado de
eventos e hashes canônicos. O replay estrito reexecuta a mesma entrada pública e
compara bytes e resultados terminais; assim, uma mudança de layout ou de
causalidade não atualiza o golden implicitamente.

A fix wave de revisão também cobre raízes de score recíprocas sem
last-writer-wins, cancelamento de fratura física pendente cujo parent desaparece
e separação entre watermark de receipt externo e âncora causal de dano. Para
enemies multi-part, dano e saída publicados no mesmo batch correlacionam por
EntityId; materiais continuam correlacionados pelo par `(EntityId,PartId)`.

## Fixtures congelados

| Fixture | Resultado | Score / estrelas | Eventos | State hash | Playthrough hash |
|---|---|---:|---:|---|---|
| orbital_virela | victory | 15000 / 2 | 98 | fnv1a64:bc5c56f534487248 | fnv1a64:bd9242c062d66d5b |
| orbital_structural | victory | 15660 / 2 | 134 | fnv1a64:265e25de741e3021 | fnv1a64:f44123ac03f5b730 |
| orbital_defeat | defeat | 0 / 0 | 4 | fnv1a64:68cce21cb8e0834c | fnv1a64:c01b329c93a46e92 |
| farm_tutorial | victory | 28972 / 1 | 685 | fnv1a64:be8e145e5f6906b8 | fnv1a64:bf76362e1837cf7d |
| farm_chain | victory | 50054 / 3 | 610 | fnv1a64:e7f42c188b13ecb8 | fnv1a64:045ed982a2424afe |
| farm_blue_alternative | victory | 40022 / 2 | 792 | fnv1a64:19f45d529b7f2bd2 | fnv1a64:2c4dc5c367ee0f9d |
| farm_defeat | defeat | 0 / 0 | 8 | fnv1a64:7f0cfbe6a8765d67 | fnv1a64:a6c7fe1bd98f9694 |

A rota farm_chain aceita dois releases; seus detalhes causais permanecem
encapsulados no fixture e no replay, não nesta síntese.

## Boundary Orbital e equivalência legada

O catálogo v1 continua usando `BeginAim`/`SetAim`/`Launch`, enquanto o catálogo
v2 aceita apenas o gesto `BeginGrab`/`SetPull`/`ReleaseBird`. A prova de
compatibilidade não reabre a FSM: `LegacyAimSeed` gera um AimState pela
matemática legada com `theta=0`; uma sessão v1 real quantiza esse estado e também
o AimState extraído do `LauncherState` produzido pelo gesto v2. Os quanta
completos e as unidades congeladas coincidem shot a shot para Virela
(-0,1°@8; +8°@8,18), estrutural (+5°@8; +1°@8) e derrota (+90°@8 três vezes).

Essa equivalência é gesto v2 → AimState canônico da API legada, com a mesma
semântica/outcome da rota v2; **não** é identidade da sequência cross-catalog
original da Virela (`theta=-2°`, três launches e offsets históricos), nem dos
tuples históricos da rota estrutural. Os scripts v1 permanecem separados e seus
goldens de characterization continuam byte-idênticos.

## Parser e promoção segura

O parser foi nomeado `parse_frozen_playthrough_fixture_envelope` para explicitar
seu limite: ele valida JSON fechado, framing/header, hashes e binding entre rota
e blobs. Não tenta decodificar parcialmente `canonical_state_v3`; o replay real
é a autoridade semântica e rejeita um envelope header-only impossível. Testes
separam whitespace final válido de token trailing inválido.

O emitter captura e serializa todo o lote em memória antes de writes. A
publicação usa stage e backup siblings, flush/close, rename e rollback integral;
recusa destino não vazio sem `--force`. A mesma política protege
`--layout-only`, e um teste de falha entre duas promoções comprova que os dois
goldens anteriores permanecem intactos.

## Cleanup

O buscador de calibração e seu seam de LOS, usados apenas durante a exploração,
foram removidos. O seam `physical_contact` permanece porque sustenta as
asserções físicas de catch e hammer→painéis. O emitter e o parser/replayer
estritos permanecem como ferramentas reprodutíveis. Os testes específicos de
cada rota fazem um replay semântico; a repetição 50× ficou concentrada no
contrato agregado dos sete fixtures. Quatro placeholders que apenas abriam
arquivos foram removidos.

O checkpoint histórico foi preservado em
`docs/gameplay/product-v2-task-18-checkpoint.md` com nota de supersessão; os
fixtures e a projeção de layout são a evidência vigente.

## Verificação

- Parser estrito do fixture e replay determinístico dos sete fixtures.
- Comparação de streams ordenados, estado canônico e hashes congelados.
- Gate físico da Fazenda: projeção/hash, conexidade, inventário, contatos
  permitidos e repouso de 120 ticks.
- O CTest registra o writer atômico como gate próprio; ele passou 1/1 em Debug
  e Release. `vertical_slice_playthrough` e `vertical_slice_determinism`
  passaram 2/2 nos dois builds.

## Release/layout

O modo `--layout-only` do emitter produziu o mesmo documento em Debug e
Release: 81.382 bytes, SHA-256
`EBC89861CD7957D31DA20966F2B8F8B53FABDFC62531D92D5EB633A2C6AC4313`.
O fixture promovido tem exatamente esses bytes. O filtro `product v2 levels`
passou 11/11 em Debug e Release, incluindo inventário, SAT/visual, idle120 e
projeção/hash. O aggregate Debug 7×50 passou em 6.158,22 s; por isso o watchdog
específico do CTest é 14.400 s. Depois do ajuste final de enemy multi-part, uma
nova emissão dos sete documentos foi byte-idêntica entre Debug, Release e os
oficiais, e os replays individuais pós-ajuste passaram Orbital 1/1 e Fazenda
4/4 nos dois builds. O aggregate Release final 7×50 passou em 319,35 s (320,58 s
de CTest total), com
`product v2 determinism replays all seven routes fifty times against common
bytes` em PASS.
