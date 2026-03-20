#!/usr/bin/env pwsh
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# Purpose: Merge-blocking regression gate runner for Windows PowerShell.
# Invariant: overall test suite failures must not fail this gate unless
#            Neira.RegressionGate.* explicitly fails.

$ScriptDir = Split-Path -Path $MyInvocation.MyCommand.Path -Parent
$TestDir = Resolve-Path (Join-Path $ScriptDir '..')
$LogPath = Join-Path $TestDir 'regression_gate.log'

Push-Location $TestDir
try {
    $binary = Join-Path $TestDir 'neira_tests.exe'
    if (-not (Test-Path $binary)) {
        $binary = Join-Path $TestDir 'neira_tests'
    }

    if (-not (Test-Path $binary)) {
        throw '[regression-gate] neira_tests(.exe) not found. Build it first (e.g. make -C Source/Tests neira_tests).'
    }

    & $binary 2>&1 | Tee-Object -FilePath $LogPath
    $suiteExit = $LASTEXITCODE

    $logText = Get-Content -Path $LogPath -Raw

    if ($logText -notmatch '(?m)^  (PASS|FAIL)  Neira\.RegressionGate\.') {
        Write-Host '[regression-gate] WARNING: no executable Neira.RegressionGate.* tests found in binary'
    }

    if ($logText -match '(?m)^  FAIL  Neira\.RegressionGate\.') {
        Write-Host '[regression-gate] FAIL: detected failing Neira.RegressionGate.* tests'
        exit 1
    }

    if ($suiteExit -ne 0) {
        Write-Host "[regression-gate] INFO: suite exit code was $suiteExit, ignored by gate policy"
    }

    Write-Host '[regression-gate] PASS: no Neira.RegressionGate.* failures'
    exit 0
}
finally {
    Pop-Location
}
