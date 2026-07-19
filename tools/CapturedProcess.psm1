Set-StrictMode -Version Latest
Import-Module (Join-Path $PSScriptRoot 'SafePath.psm1')

function Invoke-NinhoCapturedProcess {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][string]$FilePath,
        [Parameter(Mandatory)][AllowEmptyCollection()][string[]]$ArgumentList,
        [Parameter(Mandatory)][string]$LogDirectory,
        [Parameter(Mandatory)][string]$AllowedRoot,
        [Parameter(Mandatory)][string]$LogPrefix,
        [Parameter(Mandatory)][string]$FailureLabel,
        [string]$WorkingDirectory
    )

    $logId = [Guid]::NewGuid().ToString('N')
    $stdoutPath = Join-Path $LogDirectory ($LogPrefix + '-' + $logId + '.stdout.log')
    $stderrPath = Join-Path $LogDirectory ($LogPrefix + '-' + $logId + '.stderr.log')
    Assert-NinhoNoReparseAncestors -Path $stdoutPath -AllowedRoot $AllowedRoot | Out-Null
    Assert-NinhoNoReparseAncestors -Path $stderrPath -AllowedRoot $AllowedRoot | Out-Null

    $process = $null
    try {
        $startParameters = @{
            FilePath = $FilePath
            ArgumentList = $ArgumentList
            Wait = $true
            PassThru = $true
            WindowStyle = 'Hidden'
            RedirectStandardOutput = $stdoutPath
            RedirectStandardError = $stderrPath
        }
        if (-not [string]::IsNullOrWhiteSpace($WorkingDirectory)) {
            $startParameters.WorkingDirectory = $WorkingDirectory
        }
        $process = Start-Process @startParameters
        $exitCode = $process.ExitCode
        if ($exitCode -ne 0) {
            $stdout = (Get-Content -Raw -LiteralPath $stdoutPath -ErrorAction SilentlyContinue).Trim()
            $stderr = (Get-Content -Raw -LiteralPath $stderrPath -ErrorAction SilentlyContinue).Trim()
            if ([string]::IsNullOrEmpty($stdout)) { $stdout = '<empty>' }
            if ([string]::IsNullOrEmpty($stderr)) { $stderr = '<empty>' }
            throw "$FailureLabel failed with exit code ${exitCode}:`nstdout:`n$stdout`nstderr:`n$stderr"
        }
    }
    finally {
        if ($null -ne $process) { $process.Dispose() }
        foreach ($logPath in @($stdoutPath, $stderrPath)) {
            if (Test-Path -LiteralPath $logPath) {
                Assert-NinhoNoReparseAncestors -Path $logPath -AllowedRoot $AllowedRoot | Out-Null
                Remove-Item -LiteralPath $logPath -Force
            }
        }
    }
}

Export-ModuleMember -Function Invoke-NinhoCapturedProcess
