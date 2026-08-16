# Task 19 — AppShell e input semântico

## Resultado

A aplicação agora inicia em `app_shell.tscn`, com menu principal e opções
roteados por `ScreenRouter`, textos pt-BR vindos de catálogo fechado e
`InputRouter` como fronteira única entre eventos físicos e as 12 intenções
semânticas do produto. A vertical slice legada continua carregável e testada por
caminho explícito.

O input possui contextos `frontend` e `gameplay`: Space aceita no frontend e
ativa habilidade no gameplay. Teclado e mouse cobrem as 12 intenções; gamepad
fica desacoplado pela API semântica pública para o provider de uma tarefa
posterior. Eventos reais passam pelo `Viewport`, são consumidos uma única vez e
o clique mouse só ativa o botão sob sua posição — clicar no fundo não reutiliza
o foco anterior.

## TDD e revisão

Os primeiros REDs foram os recursos inexistentes do `InputRouter` e do
`AppShell`. As ondas de review adicionaram REDs causais para:

- boundary explícita frontend/gameplay e transição observável única;
- mouse real atravessando `InputRouter` sem dupla ativação;
- matriz de intenções suportadas por fonte/contexto;
- clique no background sem ativar o botão Exit previamente focado.

Todos ficaram GREEN nos smokes registrados. A revisão independente final
retornou **Ready to merge: YES**, com 0 Critical, 0 Important e 0 Minor.

## Verificação

- `tools/build.ps1 -Configuration Debug -WithGodot`: PASS, exit 0, usando
  VS2022 17.14/MSVC 19.44.35223.
- `tools/test.ps1 -Configuration Debug`: PASS, exit 0; CTest 69/69 em
  5.426,23 s; import completeness `33/5/13`; smokes AppShell/InputRouter e
  capturas Vulkan/OpenGL válidos.
- `tools/test_vertical_slice.ps1 -Configuration Debug`: núcleo completo
  69/69 em 6.243,10 s, todos os smokes e quatro capturas válidas. O wrapper
  terminou exit 1 apenas porque a fixture de watchdog perdeu uma vez o marcador
  `FAKE_SUCCESS_MARKER` depois da carga longa. A mesma fixture passou 5/5 em
  reprodução focada e todo o sufixo posterior do wrapper passou com
  `VERTICAL_SLICE_SUFFIX_OK`, exit 0. Nenhuma mudança de tooling foi feita para
  ocultar a race.
- `generate_foundation_report.py --check`: PASS após recaptura pública dos
  relatórios Debug/Release motivada pelo novo tested input do smoke registry.
- `git diff --check` e `git diff --cached --check`: executados antes da entrega.

## Escopo operacional

Os 33 `*.import` tocados pelo Godot são somente stat-cache com conteúdo
idêntico ao index e não entram no commit. Os oito `*.gd.uid` novos são únicos e
entram junto aos scripts. Continuar/Mundos e o provider físico de gamepad ficam
para tarefas posteriores, sem hardcode de texto ou rota fictícia nesta tarefa.
