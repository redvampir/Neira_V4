# Дорожная карта проекта Neira для агентной работы

> Цель: дать агентам и команде единый ориентир «что уже сделано», «что в работе» и «что дальше», чтобы не терять контекст между сессиями.

Дата фиксации: **2026-03-19** (обновлено: разделён статус v0.3 «частично реализовано» vs «DoD закрыт полностью», добавлен tech-checklist DoD и ссылки на целевые файлы/тесты)

---

## 1) Легенда статусов

- ✅ **Done** — результат зафиксирован в репозитории и может быть использован как опора.
- 🔄 **In Progress** — работа начата или частично оформлена, но нет полного DoD.
- ⏭️ **Next** — следующая очередь после закрытия блокеров.
- 🧊 **Blocked** — нужна развилка/решение владельца или команды.

---

## 2) Состояние проекта по слоям

| Область | Статус | Что уже сделано | Что закрывает «готово» |
|---|---|---|---|
| Видение и границы проекта | ✅ Done | Зафиксированы философия, scope, не-цели, целевая многослойная архитектура и поэтапный план v0.1–v0.5 в README. Добавлены критерии готовности v0.1–v0.3. | Поддерживать актуальность при смене стратегии. |
| Критические архитектурные пробелы | ✅ Done (диагностика) | Сформирован перечень ключевых рисков (контракты, память, explainability, SLA, privacy, migration path) и приоритет закрытия. | Превратить каждый пункт в реализуемый пакет спецификаций/артефактов. |
| Owner-решения для снятия блокеров | ✅ Done (фиксация) | Зафиксированы решения по PII, source-of-truth (через командный ADR-процесс), SLA по железу, рамкам online-learning, RU+EN-заделу. | Перевести решения в политики, схемы и тестируемые правила. |
| **Реализация v0.1 pipeline** | ✅ Done | FPhraseClassifier, FIntentExtractor, FActionRegistry, FHypothesisStore (.cpp). 35 тестов в NeiraTests. | Запустить тесты в UE Automation Tool; подключить Build.cs. |
| **Реализация v0.2 морфология** | ✅ Done | FMorphAnalyzer: словарь ~130 слов + суффиксные правила (глаголы/существительные/прилагательные). FHypothesisStore v0.2: ConfirmCount, MinConfirmCount=2. 17 тестов. | Расширить словарь до 1000+ слов; подключить к FIntentExtractor. |
| **v0.3 реализовано частично** | 🔄 In Progress | В коде есть базовый `FSyntaxParser`, `DecisionTrace`, `EventLog`; есть unit-тесты на синтаксис/trace/event-log. | Закрыть оставшиеся технические пункты DoD: ambiguous-trace, memory pressure degradation, full fail-reason pipeline. |
| **v0.3 DoD выполнен полностью** | 🔄 In Progress | DoD формализован, но закрыт не полностью: часть критериев пока только в требованиях. | Перевести все пункты DoD в код + автопроверки и перевести статус в ✅ Done. |
| Формальные контракты модулей (schema/API/errors) | 🔄 In Progress | Заголовки модулей содержат контракты в комментариях. Формальных JSON Schema ещё нет. | JSON Schema v1, versioning policy, error catalog, negative tests для каждого шага pipeline. |
| Память и транзакционность | 🔄 In Progress | FHypothesisStore: инварианты переходов состояний, счётчик подтверждений. Полная транзакционная модель ещё не описана. | Формальная модель write-path, rollback/replay сценарии, тесты на отсутствие partial state. |
| Explainability и аудит | 🔄 In Progress | Зафиксирована необходимость trace-id, decision trace и forensic dump. | Единый trace-формат + экспортируемый forensic-режим + регрессионные проверки разбора причин. |
| Drift/калибровка порогов | ⏭️ Next | Сформулирована проблема деградации и необходимость regression gate. | Offline-калибровка, drift-мониторинг, policy controlled rollout. |
| Security/Privacy baseline | ⏭️ Next | Зафиксирована классификация PII/NON_PII как обязательный минимум. | Retention/deletion policy, контроль доступа, шифрование, верификация процедур удаления. |
| C++ vs Blueprint SLA-границы | ⏭️ Next | Определена необходимость нефункциональных контрактов и запрета утечки критической логики в Blueprint. | Документированные latency/threading/side-effects SLA + проверяемые ограничения в код-ревью. |
| Эволюция к v0.4+ | ⏭️ Next | Зафиксирована потребность в ADR/DoD/migration playbook до роста сложности. | Набор ADR, DoD для слоёв, миграционные сценарии без «перепрошивки» базовых интерфейсов. |

---

## 3) Приоритетный маршрут (исполняемая очередь)

1. **Контракты модулей + каталог ошибок** (база для параллельной разработки).
2. **Транзакционная модель памяти** (устранение риска тихой порчи знаний).
3. **Explainability/Audit trail** (разбор ошибок и доверие к выводам).
4. **Regression gate для порогов и drift-контроль**.
5. **Privacy/Security baseline (PII/NON_PII + retention/deletion)**.
6. **SLA-границы C++/Blueprint и профили производительности**.
7. **Migration path для v0.3+ (ADR + DoD + playbook)**.

---

## 4) Definition of Done для агентов (единый чек-лист)

Задача считается завершённой, только если выполнено всё:

- Есть **изменение в репозитории** (спека/код/тест/док) с понятным owner-результатом.
- Есть **критерии приёмки** и проверка командой или автоматикой.
- Есть **следствие для соседних модулей** (что им теперь можно ожидать от интерфейса).
- Есть **обратная совместимость** или явно описанная стратегия миграции.
- Для рискованных изменений есть **audit-след** (ADR/trace/изменение политики).

---

## 4.1) Технический checklist закрытия DoD v0.3

Статус на **2026-03-19**: 6/7 пунктов закрыты, 1/7 открыт.

- [x] Базовый `FSyntaxParser` + unit-тесты (`Source/NeiraCore/Public/FSyntaxParser.h`, `Source/NeiraCore/Private/FSyntaxParser.cpp`, `Source/NeiraTests/Private/SyntaxParserTests.cpp`).
- [x] `DecisionTrace` в intent-пайплайне + тесты (`Source/NeiraCore/Public/FIntentExtractor.h`, `Source/NeiraCore/Private/FIntentExtractor.cpp`, `Source/NeiraTests/Private/IntentExtractorTests.cpp`).
- [x] `EventLog` переходов гипотез + тесты (`Source/NeiraCore/Public/FHypothesisStore.h`, `Source/NeiraCore/Private/FHypothesisStore.cpp`, `Source/NeiraTests/Private/HypothesisStoreTests.cpp`).
- [x] Ambiguous-trace на уровне каждого `AmbiguousToken` (реализован `FAmbiguousDecisionTrace` + deterministic tie-break + unit-тесты; целевые точки: `Source/NeiraCore/Public/FSyntaxParser.h`, `Source/NeiraCore/Private/FSyntaxParser.cpp`, `Source/NeiraTests/Private/SyntaxParserTests.cpp`).
- [x] Memory pressure degradation (`Medium/High/Critical`) с тестируемыми гарантиями HOT/WARM/COLD+anchor (реализован `FMemoryPressurePolicy` с API статуса/причины деградации и unit-тестами переходов/anchor-recovery; целевые точки: `Source/NeiraCore/Public/FMemoryPressurePolicy.h`, `Source/NeiraCore/Private/FMemoryPressurePolicy.cpp`, `Source/NeiraTests/Private/MemoryPressurePolicyTests.cpp`).
- [ ] Full fail-reason pipeline от синтаксиса до ответа (целевые точки: `Source/NeiraCore/Public/FActionTypes.h`, `Source/NeiraCore/Public/NeiraTypes.h`, `Source/NeiraCore/Private/FActionRegistry.cpp`, `Source/NeiraCore/Private/FIntentExtractor.cpp`, `Source/NeiraTests/Private/*`).
- [x] Threshold regression gate по `topic_change_threshold` и confidence thresholds на фиксированном RU/EN-наборе (добавлены fixtures `Source/NeiraTests/Fixtures/RegressionIntentFrameFixtures.h`, source-of-truth конфиг `Source/NeiraTests/Fixtures/RegressionThresholds.cfg`, gate-тест `Source/NeiraTests/Private/ThresholdRegressionGateTests.cpp`, цель запуска `make regression-gate`).


### 4.2) Локальный/CI запуск regression gate

- Обязательная команда локально: `cd Source/Tests && make regression-gate`.
- Если подключается CI, эта же команда должна быть отдельной job (`regression-gate`) и блокировать merge при падении.
- Policy: **любое изменение threshold-конфига (`RegressionThresholds.cfg`) требует успешного прогона regression gate** на фиксированном RU/EN наборе.

---

## 5) Формат обновления после каждой сессии агента

Чтобы карта всегда отвечала на вопрос «что сделано», после каждой существенной задачи обновлять:

1. Строку в таблице «Состояние проекта по слоям» (статус + короткий факт).
2. Один пункт в «Приоритетном маршруте» (если изменился порядок).
3. Раздел DoD/критерии приёмки (если появились новые обязательные проверки).
4. Дату фиксации в шапке документа.
5. Запись в `Docs/Roadmap/Agent_Handoff_Log.md`.

Опорные документы для каждой сессии:
- `Docs/Agents/Agent_Playbook.md` — стандартный workflow агента.
- `Docs/ADR/README.md` — где фиксировать архитектурные решения.

Рекомендуемый формат записи факта:
- `YYYY-MM-DD — <что сделано> — <какой риск закрыт> — <ссылка на файл/ADR/тест>`.

---

## 6) Что считать «сделано» уже сейчас (снимок на 2026-03-19, обновлено)

- ✅ Зафиксирован архитектурный каркас, scope и этапность v0.1–v0.5.
- ✅ Зафиксирован список критических архитектурных пробелов с приоритетом закрытия.
- ✅ Зафиксированы ключевые owner-решения, снимающие часть стратегических блокеров.
- ✅ Добавлен минимальный контур координации агентов: playbook, handoff log, ADR-реестр, knowledge index.
- 🔄 v0.3 зафиксирована как частично реализованная: база синтаксиса/trace/event-log есть, но полный DoD ещё не закрыт технически.
- 🔄 Не хватает формальных контрактов и full fail-reason pipeline; threshold regression gate вынесен в обязательную проверку `make regression-gate`.

---

## 7) Результат ревизии handoff (2026-03-19)

Проверено, что предыдущий агент корректно зафиксировал архитектурные разделы в README. По ревизии выявлены и закрыты два пропуска:

- добавлены **критерии готовности v0.3** (DoD на уровне наблюдаемых проверок);
- добавлен **алгоритм retrieval по `FAnchorSystem`** с порогами `AUTO_RESTORE_TOPIC / ASK_USER_CONFIRMATION / NO_MATCH`.

Что остаётся открытым после ревизии:
- формальные schema-контракты и error catalog;
- транзакционная модель памяти (event log/replay);
- единый trace/audit формат.
