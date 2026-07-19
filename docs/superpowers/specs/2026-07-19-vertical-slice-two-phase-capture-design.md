# Fluxo de captura e certificação do vertical slice em duas fases

## Problema

O manifest `ninho.vertical-slice.reviews.v2` vincula a revisão de arte ao SHA-256
do manifest de captura e aos hashes dos cinco goldens canônicos. O fluxo visual
legado apaga e recaptura os artefatos imediatamente antes de validar as reviews.
Uma revisão externa, portanto, não consegue conhecer com segurança a identidade
da captura que o mesmo comando acabará certificando.

## Interface aprovada

`tools/test_vertical_slice.ps1` mantém o comportamento legado quando nenhum modo
novo é informado. `-CaptureOnly` e `-UseExistingCapture` pertencem a parameter
sets mutuamente exclusivos e implicam o gate visual mesmo sem
`-IncludeVisualGate`.

- `-CaptureOnly`: executa os preflights existentes, limpa somente o diretório
  canônico `artifacts/vertical-slice/<configuration>`, captura, valida o manifest
  e todos os artefatos declarados e encerra sem ler reviews, empacotar ou escrever
  evidência de certificação.
- `-UseExistingCapture`: executa os preflights existentes, não limpa nem captura,
  exige o manifest no diretório canônico da configuração, valida sua identidade e
  seus artefatos e somente então valida reviews, clean-room, pacote e evidência.
- Legado com `-IncludeVisualGate`: recaptura e certifica em uma única execução,
  para compatibilidade; reviews desatualizadas continuam falhando fechadas.
- Legado sem gate visual: preserva o gate não visual atual.

## Limite do helper

Um helper de workflow recebe raiz, configuração, diretório de artefatos, modo e a
operação de captura. Ele verifica que o diretório é exatamente o diretório
canônico da configuração e que não há ancestral reparse fora da raiz. Nos modos
Legacy e CaptureOnly, remove e recria somente esse diretório antes de chamar a
operação de captura. Em UseExistingCapture, a operação não é chamada e nenhum
arquivo é removido.

O helper resolve `capture-manifest.json`, exige schema
`ninho.vertical-slice.capture.v1` e configuração exata, calcula seu SHA-256 e
valida de forma fail-closed:

- `source_artifacts` não vazio, sem caminhos duplicados, absolutos, escapando da
  raiz ou passando por reparse points;
- existência e SHA-256 de cada artefato declarado;
- exatamente os goldens `overview`, `aim`, `virela`, `vulnerable_impact` e
  `result`, cada um com hash válido;
- correspondência exata entre cada golden e o artefato de origem declarado.

O retorno contém somente o path canônico do manifest, documento já validado e
SHA-256 calculado. Reviews nunca são criadas, aprovadas ou modificadas por esse
helper ou pelo runner.

## Saídas e falhas

CaptureOnly emite um marker estável com configuração, path do manifest e SHA-256.
UseExistingCapture só emite o marker final de certificação já existente depois de
todas as validações. Manifest ausente, configuração trocada, path inseguro, arquivo
ausente, hash divergente, golden incompleto ou reviews não vinculadas encerram o
processo antes de qualquer evidência ser escrita.

## Verificação

Uma fixture temporária chama o helper com uma operação de captura leve e prova:

1. CaptureOnly/Legacy removem um sentinel, chamam captura uma vez e aceitam um
   manifest íntegro.
2. UseExistingCapture preserva o sentinel e não chama captura.
3. Configuração, hash, arquivo, path e golden inválidos falham fechados.
4. Os modos de CLI são exclusivos, implicam visual e CaptureOnly sai antes de
   reviews/evidência.

Os comandos operacionais documentados são: captura e validação; revisão externa
que vincula o hash impresso; certificação com captura existente. O gate completo
não é necessário para testar a fixture de workflow.
