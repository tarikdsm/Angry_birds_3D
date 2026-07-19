# Ninho Orbital

Vertical slice desktop de **Primeira Orbita - Contrapeso de Aster**, construído
com C++20, Box3D 3D e Godot 4.5.1.

## Pré-requisitos Windows

Antes do primeiro build em uma máquina local:

- instale o Python 3.11 ou superior e deixe o comando `python` disponível no
  `PATH`;
- instale/valide o Visual Studio Build Tools pinado pelo projeto;
- instale as ferramentas portáteis mantidas em `.tools/`.

Os dois últimos passos são automatizados, mas pertencem a conjuntos de parâmetros
separados e devem ser executados em comandos distintos:

```powershell
.\tools\bootstrap.ps1 -InstallVisualStudio
.\tools\bootstrap.ps1 -InstallPortable
```

`-InstallVisualStudio` instala, quando necessário, o Visual Studio/Build Tools
18.7.3 (build 11925.98), o toolset MSVC 14.44 x64 e o Windows SDK 10.0.26100.0.
`-InstallPortable` instala CMake, Ninja, Godot, Blender, FFmpeg/FFprobe e os templates de export
nas versões verificadas pelo projeto; ele não instala Visual Studio nem Python.
As versões, componentes e hashes canônicos estão em
[`tools/toolchain.lock.json`](tools/toolchain.lock.json).

O bootstrap e os wrappers de build rejeitam uma instalação fora desse contrato.
Python 3.11+ é usado diretamente pelos testes PowerShell/Python e pelo smoke JSON
registrado no CTest; como `Invoke-Native.ps1` valida o lock completo antes de
compilar, o comando `python` também deve existir no fluxo de empacotamento.

## Testes

Com os pré-requisitos preparados, execute o gate oficial Debug:

```powershell
.\tools\test.ps1 -Configuration Debug
```

A CI executa o mesmo gate no runner `windows-2025-vs2026`. Nesse ambiente,
Visual Studio e Python são pré-requisitos fornecidos pela máquina; por isso o
workflow instala apenas a parcela portátil antes de chamar `tools/test.ps1`.
Os gates de captura resolvem `ffmpeg.exe` e `ffprobe.exe` diretamente desse
toolchain pinado; não dependem de binários preexistentes no `PATH` do runner.

## Certificação visual em duas fases

Quando a captura precisa ser revisada externamente, gere primeiro um conjunto
imutável de artefatos e só depois certifique exatamente esse conjunto. Para a
fase de captura:

```powershell
.\tools\test_vertical_slice.ps1 -Configuration Debug -CaptureOnly
.\tools\test_vertical_slice.ps1 -Configuration Release -CaptureOnly
```

`CaptureOnly` implica o gate visual, limpa somente o diretório canônico da
configuração, recaptura, valida o manifesto e os artefatos e termina com o
marcador `VERTICAL_SLICE_CAPTURE_READY`. CaptureOnly não cria nem aprova revisões
e não escreve a evidência de certificação. Os revisores devem inspecionar os
artefatos dessa execução e vincular o manifesto de revisões v2 ao SHA-256 do
manifesto de captura, à configuração, ao hash dos inputs testados e aos hashes
de origem/metadados dos goldens.

Depois que as revisões externas estiverem presentes, certifique sem alterar a
captura:

```powershell
.\tools\test_vertical_slice.ps1 -Configuration Debug -UseExistingCapture
.\tools\test_vertical_slice.ps1 -Configuration Release -UseExistingCapture
```

UseExistingCapture nunca limpa nem recaptura. Ele exige o manifesto canônico em
`artifacts/vertical-slice/<configuração>/capture-manifest.json`, valida esquema,
configuração, caminhos seguros, existência e hashes dos artefatos e dos cinco
goldens, e só então valida revisões, clean-room e evidência. Qualquer ausência ou
divergência falha de forma fechada antes da certificação. `-CaptureOnly` e
`-UseExistingCapture` são mutuamente exclusivos; sem eles,
`-IncludeVisualGate` preserva o fluxo legado de recaptura e certificação numa só
execução.

## Pacote Windows

Depois da preparação da máquina, gere o pacote Release:

```powershell
.\tools\package_windows.ps1
```

O pacote é criado em `artifacts/package/windows-release` com executável, PCK
separado, GDExtension x86_64 Release, inventário de fontes, licenças e avisos de
terceiros e manifesto SHA-256 canônico. O empacotador inicia o fluxo mínimo do
executável gerado a partir do próprio diretório de saída e falha se o launch ou qualquer
hash divergir. Para revalidar um pacote existente sem reconstruí-lo:

```powershell
.\tools\package_windows.ps1 -VerifyOnly
```

## Licenciamento e publicação

O projeto Ninho Orbital não possui licença própria declarada neste marco; por
isso o pacote correspondente no SBOM mantém `licenseDeclared` e
`licenseConcluded` como `NOASSERTION`. Os textos em
[`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md) e `third_party/*LICENSE*`
documentam somente os componentes de terceiros correspondentes e não
constituem uma licença para o projeto.

Gerar o pacote Windows é uma verificação técnica e não autoriza sua publicação.
A publicação pública ou comercial permanece bloqueada até uma decisão explícita
do proprietário, registrada como licença própria ou como termos de distribuição
aprovados para o projeto. Essa decisão deve receber a revisão jurídica adequada
ao contexto de publicação.
