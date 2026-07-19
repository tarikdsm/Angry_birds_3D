[CmdletBinding()]
param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
)

$ErrorActionPreference = 'Stop'

function Assert-True {
    param([bool]$Condition, [string]$Message)

    if (-not $Condition) {
        throw $Message
    }
}

function Get-GitAttributes {
    param(
        [Parameter(Mandatory)][string[]]$Paths,
        [Parameter(Mandatory)][string[]]$Attributes
    )

    $output = @(& git -c "safe.directory=$Root" -C $Root `
        check-attr @Attributes -- @Paths)
    if ($LASTEXITCODE -ne 0) {
        throw "git check-attr failed with exit code $LASTEXITCODE"
    }
    $result = @{}
    foreach ($line in $output) {
        $match = [regex]::Match([string]$line, '^(.*): ([^:]+): (.*)$')
        if (-not $match.Success) {
            throw "unexpected git check-attr output: $line"
        }
        $result[$match.Groups[1].Value + "`0" + $match.Groups[2].Value] =
            $match.Groups[3].Value
    }
    return $result
}

$protectedExtensions = @('.blend', '.glb', '.png', '.wav')
$textualExtensions = @('.gd', '.import', '.json', '.ps1', '.tres', '.tscn')
$trackedFiles = @(& git -c "safe.directory=$Root" -C $Root ls-files)
if ($LASTEXITCODE -ne 0) {
    throw "git ls-files failed with exit code $LASTEXITCODE"
}

$binaryFiles = @($trackedFiles | Where-Object {
    $protectedExtensions -ccontains [IO.Path]::GetExtension($_).ToLowerInvariant()
})
foreach ($extension in $protectedExtensions) {
    Assert-True (@($binaryFiles | Where-Object {
                [IO.Path]::GetExtension($_).ToLowerInvariant() -ceq $extension
            }).Count -gt 0) `
        "No tracked file exercises binary attribute rule: *$extension"
}

$binaryAttributes = Get-GitAttributes -Paths $binaryFiles `
    -Attributes @('text', 'diff', 'merge', 'binary')
foreach ($path in $binaryFiles) {
    Assert-True ($binaryAttributes[$path + "`0binary"] -ceq 'set') `
        "Tracked binary does not match an explicit binary rule: $path"
    foreach ($disabled in 'text', 'diff', 'merge') {
        Assert-True ($binaryAttributes[$path + "`0$disabled"] -ceq 'unset') `
            "Tracked binary does not disable ${disabled}: $path"
    }
}

$textFiles = @($trackedFiles | Where-Object {
    $textualExtensions -ccontains [IO.Path]::GetExtension($_).ToLowerInvariant()
})
$textAttributes = Get-GitAttributes -Paths $textFiles -Attributes @('binary')
foreach ($path in $textFiles) {
    Assert-True ($textAttributes[$path + "`0binary"] -ceq 'unspecified') `
        "Textual project format was incorrectly marked binary: $path"
}

Write-Output 'binary-gitattributes-contract-tests: PASS'
