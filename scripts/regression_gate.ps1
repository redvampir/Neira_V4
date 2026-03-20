#!/usr/bin/env pwsh
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# Compatibility wrapper.
$ScriptDir = Split-Path -Path $MyInvocation.MyCommand.Path -Parent
& (Join-Path $ScriptDir '..\Source\Tests\scripts\regression_gate.ps1') @args
exit $LASTEXITCODE
