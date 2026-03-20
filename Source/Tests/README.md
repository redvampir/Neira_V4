# Source/Tests — локальный и CI запуск

## Что это

Папка содержит native test-runner `neira_tests` и merge-blocking regression gate для drift/threshold тестов.

## Режимы запуска

- `run` — полный прогон всех тестов; падает при любом fail.
- `regression-gate` — merge-blocking gate; падает **только** если есть `FAIL Neira.RegressionGate.*`.

### Матрица режимов (policy)

| Режим | Цель | Ожидаемый результат | Код возврата | Влияние на merge |
|---|---|---|---|---|
| `run` | Полная диагностика качества (весь `neira_tests`) | Нет падений в полном наборе тестов | `0` — все тесты прошли; `!=0` — есть хотя бы один fail/runtime error | **Неблокирующий** сам по себе, используется как инженерная проверка качества перед PR |
| `regression-gate` | Merge-blocking drift/threshold gate (`Neira.RegressionGate.*`) | Нет `FAIL Neira.RegressionGate.*` | `0` — регрессий drift/threshold нет; `1` — найден хотя бы один fail в `Neira.RegressionGate.*` | **Блокирующий** merge (required check в CI) |

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

Policy-формулировки для handoff/roadmap должны совпадать с этой матрицей (единая терминология `run` vs `regression-gate`).
