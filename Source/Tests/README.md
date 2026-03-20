# Source/Tests — локальный и CI запуск

## Что это

Папка содержит native test-runner `neira_tests` и merge-blocking regression gate для drift/threshold тестов.

## Режимы запуска

- `run` — полный прогон всех тестов; падает при любом fail.
- `regression-gate` — merge-blocking gate; падает **только** если есть `FAIL Neira.RegressionGate.*`.

## Linux / macOS

```bash
# Полный прогон
make -C Source/Tests run

# Regression gate (предпочтительно для CI merge-blocking)
bash Source/Tests/scripts/regression_gate.sh
# или
make -C Source/Tests regression-gate
```

## Windows (PowerShell)

```powershell
# Собрать бинарь (пример через msys2/bash, как в CI)
C:\tools\msys64\usr\bin\bash -lc "export PATH=/ucrt64/bin:$PATH && cd /d/a/Neira_V4/Neira_V4/Source/Tests && make neira_tests"

# Regression gate
.\Source\Tests\scripts\regression_gate.ps1
```

## CI policy

Workflow: `.github/workflows/regression-gate.yml`.

Merge-blocking job должен использовать launcher из `Source/Tests/scripts/` и трактовать как blocking только падения `Neira.RegressionGate.*`.
