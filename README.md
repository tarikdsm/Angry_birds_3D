# Ninho Orbital

Vertical slice desktop de **Primeira Orbita - Contrapeso de Aster**, construído
com C++20, Box3D 3D e Godot 4.5.1.

## Pacote Windows

Instale/verifique as ferramentas portáteis e gere o pacote Release:

```powershell
.\tools\bootstrap.ps1 -InstallPortable
.\tools\package_windows.ps1
```

O pacote é criado em `artifacts/package/windows-release` com executável, PCK
separado, GDExtension x86_64 Release, inventário de conteúdo, licenças e um
manifesto SHA-256 canônico. O empacotador inicia o fluxo mínimo do executável
gerado a partir do próprio diretório de saída e falha se o launch ou qualquer
hash divergir. Para revalidar um pacote existente sem reconstruí-lo:

```powershell
.\tools\package_windows.ps1 -VerifyOnly
```
