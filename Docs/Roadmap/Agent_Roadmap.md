# Дорожная карта проекта Neira для агентной работы

> Цель: дать агентам и команде единый ориентир "что уже сделано", "что в работе" и "что дальше", чтобы не терять контекст между сессиями.
>
> Этот документ является основным source of truth по актуальному статусу проекта. Исторические audit-отчеты полезны как контекст, но не должны переопределять текущий статус из этого файла.

Дата фиксации: **2026-03-20**  
Последняя локальная проверка: `Source/Tests/neira_tests.exe` -> **146/146 PASS**  
Ограничение текущего Windows PowerShell-окружения: `bash` доступен, `make` отсутствует; поэтому команда `make regression-gate` пока не является универсально воспроизводимой без дополнительной установки GNU `make`.

## Навигация

- [Быстрый снимок](#snapshot)
- [1) Легенда статусов](#legend)
- [2) Состояние проекта по слоям](#layers)
- [3) Приоритетный маршрут](#priority-route)
- [4) Definition of Done для агентов](#dod)
- [4.2) Локальный/CI запуск regression gate](#regression-gate)
- [6) Что считать "сделано" уже сейчас](#done-now)
- [7) Результат ревизии handoff](#handoff-review)
- [README](../../README.md)
- [Аудит v0.4](V0_4_Audit_2026-03-19.md)
- [Контракты модулей](../Contracts/Module_Contracts_v1.md)
- [Критические архитектурные пробелы](../Architecture/Critical_Open_Architecture_Gaps.md)
- [Индекс знаний](../Knowledge/Knowledge_Index.md)

---

<a id="snapshot"></a>
## Быстрый снимок

- v0.1-v0.4 реализованы в коде и подтверждаются локальным нативным тестовым раннером.
- Встроенный словарь runtime-интегрирован; OpenCorpora тоже встроен в runtime-path `FMorphAnalyzer` через внешний JSON-словарь, shared cache и lazy-load policy с size guard.
- `NeiraDialog` получил P0 meta-dialog routing для greeting/self-description/capability/courtesy/farewell сценариев и более естественный fallback без debug-profile блока в обычном режиме.
- `ThresholdRegressionGateTests.cpp`, `MemoryPressurePolicyTests.cpp` и `DoDIntegrationTests.cpp` подключены в `Source/Tests/Makefile`.
- Merge-blocking workflow `regression-gate` остается операционно незавершенным: обертка есть, но ее канонический запуск зависит от наличия GNU `make`.
- Следующий этап: Privacy/Security baseline -> SLA C++/Blueprint -> migration playbook для v0.4+.

---

<a id="legend"></a>
## 1) Легенда статусов

- `Done` — результат зафиксирован в репозитории и может использоваться как опора.
- `In Progress` — код или документация уже есть, но нет полностью воспроизводимого DoD или рабочего workflow.
- `Next` — следующая очередь после закрытия текущих операционных хвостов.
- `Blocked` — нужна развилка или явное owner-решение.

---

<a id="layers"></a>
## 2) Состояние проекта по слоям

| Область | Статус | Что уже сделано | Что закрывает "готово" |
|---|---|---|---|
| Видение и границы проекта | `Done` | В [README](../../README.md) зафиксированы философия, scope, не-цели, целевая многослойная архитектура и план v0.1-v0.5. | Держать README синхронизированным с roadmap и не допускать drift по статусам. |
| Критические архитектурные пробелы | `Done` | Список ключевых рисков и порядок закрытия зафиксирован в [Critical_Open_Architecture_Gaps.md](../Architecture/Critical_Open_Architecture_Gaps.md). | Переводить каждый риск в отдельный реализуемый пакет спек, тестов и политик. |
| Owner-решения для снятия блокеров | `Done` | Зафиксированы решения по PII, source-of-truth, SLA по железу, online-learning и RU+EN заделу. | Довести решения до машинно-проверяемых политик и operational docs. |
| Реализация v0.1 pipeline | `Done` | `FPhraseClassifier`, `FIntentExtractor`, `FActionRegistry`, `FHypothesisStore` и базовые тесты присутствуют в коде. | Поддерживать обратную совместимость контрактов при расширении pipeline. |
| Реализация v0.2 морфология | `Done` | `FMorphAnalyzer` использует встроенный словарь 1000+ форм, внешний OpenCorpora как `ext_dict`, shared cache загрузки, lazy-load policy (`off/lazy/eager`) и договоренный контракт `FMorphResult`. | Решить, нужен ли более легкий бинарный/отфильтрованный словарный кэш и отдельный perf-budget. |
| Реализация v0.3 синтаксис | `Done` | `FSyntaxParser` интегрирован в intent-path, есть ambiguous trace, memory pressure policy и integration coverage. | При расширении синтаксиса не ломать детерминизм и explainability. |
| Технический долг v0.3 | `Done` | `EventLog`, `DecisionTrace`, fail-reason pipeline и regression fixtures находятся в репозитории. | Поддерживать трассировку как обязательный инвариант при новых изменениях. |
| Реализация v0.4 рассуждения | `Done` | `FBeliefEngine`, веса источников, `Downgrade()`, `FindByClaim()` и negative tests для low-confidence query-intents присутствуют и проходят локально. | Следующий шаг уже не про наличие кода, а про privacy/SLA/migration layer вокруг него. |
| Формальные контракты модулей | `Done` | [Module_Contracts_v1.md](../Contracts/Module_Contracts_v1.md) и `Error_Catalog.md` фиксируют schema, инварианты, диапазоны и recovery expectations. | Машинно-читаемые schema-файлы при переходе к следующему уровню интеграции. |
| Память и транзакционность | `Done` | `FHypothesisEvent` append-only, успешные переходы пишутся в `EventLog`, negative cases покрыты тестами. | Replay/idempotency как следующий уровень зрелости. |
| Explainability и аудит | `Done` | `DecisionTrace` и `EventLog` дают базовую объяснимость по intent и knowledge transitions. | Полный forensic dump и trace-id через весь pipeline. |
| Drift/калибровка порогов | `In Progress` | Threshold fixtures и gate-тесты подключены в `Source/Tests/Makefile`; бинарь с ними локально проходит. | Platform-neutral запуск `regression-gate` и отдельная CI job, которая реально блокирует merge. |
| Security/Privacy baseline | `Next` | Зафиксирована минимальная классификация `PII/NON_PII` и owner-решения по данным. | Retention/deletion policy, правила доступа, минимальные требования к защите данных. |
| C++ vs Blueprint SLA-границы | `Next` | Функциональная граница описана, необходимость нефункциональных контрактов зафиксирована. | Документированные latency/threading/side-effects SLA и ограничения для code review. |
| Эволюция к v0.4+ | `Next` | Потребность в migration playbook и ADR/DoD для следующих слоев зафиксирована. | Набор миграционных сценариев без перепрошивки базовых интерфейсов. |

---

<a id="priority-route"></a>
## 3) Приоритетный маршрут

1. `Done` Контракты модулей и каталог ошибок.
2. `Done` Транзакционная модель памяти.
3. `Done` Explainability / audit trail.
4. `In Progress` Regression gate для порогов и drift-контроль: тесты подключены, но нужен platform-neutral launcher или явный Windows wrapper.
5. `Next` Privacy/Security baseline (`PII/NON_PII`, retention, deletion).
6. `Next` SLA-границы C++/Blueprint и профили производительности.
7. `Next` Migration path для v0.4+ (`ADR`, `DoD`, `playbook`).

---

<a id="dod"></a>
## 4) Definition of Done для агентов

Задача считается завершенной, только если выполнено все:

- есть изменение в репозитории с понятным owner-результатом;
- есть критерии приемки и проверка командой или автоматикой;
- есть следствие для соседних модулей, то есть понятно, чего теперь ожидать от интерфейса;
- есть обратная совместимость или явно описанная стратегия миграции;
- для рискованных изменений есть audit-след: ADR, trace или изменение политики.

## 4.1) Технический checklist закрытия DoD v0.3

Статус на **2026-03-20**: 7/7 пунктов закрыты на уровне кода и тестов.

- [x] Базовый `FSyntaxParser` и unit-тесты.
- [x] `DecisionTrace` в intent-пайплайне и тесты.
- [x] `EventLog` переходов гипотез и тесты.
- [x] Ambiguous-trace на уровне каждого `AmbiguousToken`.
- [x] Memory pressure degradation (`Medium/High/Critical`) с проверяемыми гарантиями.
- [x] Full fail-reason pipeline от синтаксиса до ответа.
- [x] Threshold regression fixtures и gate-тест `ThresholdRegressionGateTests.cpp` подключены; operational wrapper `make regression-gate` считается закрытым только в окружениях с GNU `make` и `bash`.

<a id="regression-gate"></a>
### 4.2) Локальный/CI запуск regression gate

- Unix-like окружение: `cd Source/Tests && make regression-gate`.
- Текущее Windows PowerShell-окружение: `.\neira_tests.exe` дает локальный статус прогона, но не заменяет merge-blocking regression wrapper; на 2026-03-20 локальный результат = `146/146 PASS`.
- Для CI целевая модель остается той же: отдельная job `regression-gate`, которая падает на метриках семейства `Neira.RegressionGate.*`.
- Policy: любое изменение `RegressionThresholds.cfg` требует успешного прогона regression gate на фиксированном RU/EN наборе.

---

## 5) Формат обновления после каждой сессии агента

После каждой существенной задачи обновлять:

1. строку в таблице "Состояние проекта по слоям";
2. пункт в "Приоритетном маршруте", если изменился порядок;
3. раздел DoD или критерии приемки, если появились новые обязательные проверки;
4. дату фиксации в шапке документа;
5. запись в `Docs/Roadmap/Agent_Handoff_Log.md`.

Опорные документы для каждой сессии:

- [README](../../README.md) — общий контекст и актуальный снимок;
- [Agent_Playbook.md](../Agents/Agent_Playbook.md) — workflow агента;
- [ADR index](../ADR/README.md) — реестр архитектурных решений.

Рекомендуемый формат записи факта:

- `YYYY-MM-DD — <что сделано> — <какой риск закрыт> — <ссылка на файл/ADR/тест>`.

---

<a id="done-now"></a>
## 6) Что считать "сделано" уже сейчас

- `Done` Зафиксирован архитектурный каркас, scope и этапность v0.1-v0.5.
- `Done` Зафиксирован список критических архитектурных пробелов с приоритетом закрытия.
- `Done` Добавлен контур координации агентов: playbook, handoff log, ADR-реестр, knowledge index.
- `Done` Транзакционность памяти реализована: `FHypothesisEvent` + `EventLog`.
- `Done` Explainability реализован: `DecisionTrace` в `FIntentResult`.
- `Done` `FSyntaxParser` интегрирован с `FIntentExtractor`.
- `Done` v0.4 закрыт на уровне кода: `FBeliefEngine`, `EHypothesisSource`, `Downgrade()`, `FindByClaim()`, low-confidence guard для query-intents.
- `Done` Формальные контракты модулей v1 опубликованы.
- `Done` Встроенный словарь расширен до 1000+ форм и работает в runtime.
- `Done` OpenCorpora скачан, сконвертирован и интегрирован в runtime-path `FMorphAnalyzer` с lazy-load policy и size guard.
- `Done` Локальный нативный тестовый раннер на 2026-03-20 дает `146/146 PASS`.
- `Next` Следующий этап: Privacy/Security baseline -> SLA C++/Blueprint -> migration playbook v0.4+.

---

<a id="handoff-review"></a>
## 7) Результат ревизии handoff

Историческая ревизия от **2026-03-19** остается полезной как контекст и сохранена в [V0_4_Audit_2026-03-19.md](V0_4_Audit_2026-03-19.md).

Что важно помнить сейчас:

- старые audit-наблюдения про отсутствие `ThresholdRegressionGateTests.cpp` в `Makefile` больше не актуальны;
- актуальным хвостом остается не подключение тестов, а воспроизводимый launcher `regression-gate` в разных окружениях;
- любые новые документы должны ссылаться на этот roadmap как на текущий статус, а не дублировать его в несинхронном виде.
