# Ревью изменений после сессии `claude/improve-dialogue-quality-8tAd5`

Дата: 2026-03-20  
Ревьюер: codex

## Что проверено

1. История handoff и roadmap после обновлений по `FSentencePlanner`.
2. Локальный запуск полного набора тестов (`make -C Source/Tests run`).
3. Локальный запуск merge-wrapper (`make -C Source/Tests regression-gate`).
4. Актуальность исторических логов в `Artifacts/` и `Source/Tests/regression_gate.log`.

## Подтверждено

- Сессия по v0.5 NLG корректно зафиксирована в handoff: добавлены `FSentencePlanner`, `EConfidenceLevel`, `StrategyID` и связанный набор тестов.
- Полный раннер в текущем окружении воспроизводит заявленное состояние: `199 PASS / 1 FAIL`.
- `regression-gate` запускается и делает то, что заявлено в Makefile: не падает на общем падении suite, но валит пайплайн только при падении `Neira.RegressionGate.*`.

## Незакрытые моменты (по журналу и фактическому прогону)

### 1) Pre-existing падение по OpenCorpora всё ещё открыто

`Neira.MorphAnalyzer.ExternalDictionary.AutoLoadAndLookup` стабильно падает из-за отсутствия `opencorpora_dict.json` в test environment. Это подтверждается и в handoff/roadmap, и в локальном прогоне.

**Почему важно:** пока тест не стабилизирован (fixture или чёткий contract для CI), общий прогон `make run` остаётся не-"зелёным" по exit code.

### 2) Operational drift между "полный run" и "merge gate"

`make run` падает из-за 1 fail, а `make regression-gate` формально проходит. Это осознанный design, но его легко неправильно интерпретировать без явного policy-комментария для ревьюеров.

**Почему важно:** есть риск ложного ощущения "всё прошло", если смотреть только на код возврата gate-команды.

### 3) Требуется platform-neutral wrapper для gate

Roadmap помечает это как `In Progress`, и вывод ревью это подтверждает: текущий workflow функционален в Unix-like среде, но остаётся зависимым от наличия GNU `make` и bash-обёртки.

**Почему важно:** merge-blocking процесс должен быть одинаково воспроизводим в CI/локально.

## Рекомендованные следующие шаги

1. Для `ExternalDictionary.AutoLoadAndLookup` выбрать одну политику и зафиксировать её в тест-контракте:
   - либо добавить минимальный test fixture JSON в репозиторий,
   - либо сделать тест `SKIP` при отсутствии fixture с отдельной меткой/счётчиком skipped.
2. Добавить короткий policy-блок в `Source/Tests/Makefile`/README: когда запускаем `run`, когда `regression-gate`, и что именно считается merge blocker.
3. Сделать платформенный launcher (`scripts/regression_gate.(sh|ps1)` или CMake target), чтобы убрать жёсткую зависимость от GNU make.

## Критерии приёмки для закрытия хвостов

- `make -C Source/Tests run` возвращает 0 в стандартном dev/CI окружении **или** имеет формально согласованный режим skipped для внешнего словаря.
- `regression-gate` запускается единообразно в Linux/macOS/Windows (через единый wrapper/target).
- В roadmap/handoff есть явная запись о закрытии этих operational хвостов с датой и командой проверки.
