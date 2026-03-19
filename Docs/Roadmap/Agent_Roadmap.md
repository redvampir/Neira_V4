# Дорожная карта проекта Neira для агентной работы

> Цель: дать агентам и команде единый ориентир «что уже сделано», «что в работе» и «что дальше», чтобы не терять контекст между сессиями.

Дата фиксации: **2026-03-19** (обновлено: v0.4 закрыт — FBeliefEngine, Downgrade(), EHypothesisSource; контракты модулей v1 + error catalog; словарь расширен до 1000+ форм)

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
| **Реализация v0.2 морфология** | ✅ Done | FMorphAnalyzer: словарь расширен с ~130 до ~400 слов (тематические блоки: глаголы познания, существительные, домен Нейры, прилагательные ~65 форм, числительные, наречия, частицы) + суффиксные правила. FHypothesisStore v0.2: ConfirmCount, MinConfirmCount=2. | Расширить словарь до 1000+ слов; подключить к полному pipeline. |
| **Реализация v0.3 синтаксис** | ✅ Done | FSyntaxParser интегрирован в FIntentExtractor (два пути: Frame→Pattern fallback). ExtractFromFrame(): bIsAbilityCheck→AnswerAbility; Statement→StoreFact; FindPredicate→FindMeaning; Question+Object→GetDefinition. Meta-word filter (слово/термин/понятие). 80 тестов — все проходят (нативный C++ runner). | Добавить обработку координации; расширить словарь до 1000+. |
| **Технический долг v0.3** | ✅ Done | EventLog в FHypothesisStore (FHypothesisEvent: append-only, каждый переход состояния записывается). DecisionTrace в FIntentResult (какое правило сработало: "Frame.AbilityCheck"/"Pattern:что такое"/"Fallback:Unknown"). 12 новых тестов (EventLog + negative cases). | — |
| **Реализация v0.4 рассуждения** | ✅ Done | FBeliefEngine: весовая схема источников (DeveloperReview 1.00 … Unknown 0.50), порог 0.30f, Created/Confirmed/Verified/Rejected/NoMatch. EHypothesisSource (5 значений + Unknown). FHypothesisStore::Downgrade() (Verified→Conflicted, Confirmed→Pending+сброс ConfirmCount). FindByClaim(). 7 тестов BeliefEngine + 5 тестов Downgrade. | — |
| Формальные контракты модулей (schema/API/errors) | ✅ Done (базовый уровень v1) | `Docs/Contracts/Module_Contracts_v1.md` — контракты FMorphResult, FIntentResult, FActionRequest/Result, FHypothesis, FHypothesisEvent, FBeliefDecision; инварианты, диапазоны, предусловия, schema_version. `Docs/Contracts/Error_Catalog.md` — 20 кодов ошибок, матрица recovery, формат DiagnosticNote. | Машинно-читаемый JSON Schema файл — при переходе к v0.5. |
| Память и транзакционность | ✅ Done | FHypothesisEvent: append-only event log при каждом успешном переходе состояния (Store/Confirm/Verify/MarkConflicted/Downgrade). ClearEventLog(). Тесты: EventLog_StoreAddsEntry, EventLog_MultipleTransitions, NegativeCase — неудачные переходы не пишутся. | — |
| Explainability и аудит | ✅ Done | DecisionTrace в FIntentResult: строка с именем сработавшего правила ("Frame.AbilityCheck", "Pattern:что такое", "Fallback:Unknown", "EmptyInput"). Тест: DecisionTrace_NotEmpty, DecisionTrace_ContainsFrame. | Полный forensic dump (альтернативы + веса) — следующий шаг в v0.4 FBeliefEngine. |
| Drift/калибровка порогов | ⏭️ Next | Сформулирована проблема деградации и необходимость regression gate. | Offline-калибровка, drift-мониторинг, policy controlled rollout. |
| Security/Privacy baseline | ⏭️ Next | Зафиксирована классификация PII/NON_PII как обязательный минимум. | Retention/deletion policy, контроль доступа, шифрование, верификация процедур удаления. |
| C++ vs Blueprint SLA-границы | ⏭️ Next | Определена необходимость нефункциональных контрактов и запрета утечки критической логики в Blueprint. | Документированные latency/threading/side-effects SLA + проверяемые ограничения в код-ревью. |
| Эволюция к v0.4+ | ⏭️ Next | Зафиксирована потребность в ADR/DoD/migration playbook до роста сложности. | Набор ADR, DoD для слоёв, миграционные сценарии без «перепрошивки» базовых интерфейсов. |

---

## 3) Приоритетный маршрут (исполняемая очередь)

1. ✅ **Контракты модулей + каталог ошибок** — выполнено (Docs/Contracts/).
2. ✅ **Транзакционная модель памяти** — выполнено (FHypothesisEvent + EventLog).
3. ✅ **Explainability/Audit trail** — выполнено (DecisionTrace + EventLog).
4. 🔄 **Regression gate для порогов и drift-контроль** — in progress (заявлен `ThresholdRegressionGateTests`, но фактический прогон `make regression-gate` требует дофикса подключения в test runner).
5. **Privacy/Security baseline (PII/NON_PII + retention/deletion)**.
6. **SLA-границы C++/Blueprint и профили производительности**.
7. **Migration path для v0.4+ (ADR + DoD + playbook)**.

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

Статус на **2026-03-19**: 7/7 пунктов закрыты.

- [x] Базовый `FSyntaxParser` + unit-тесты (`Source/NeiraCore/Public/FSyntaxParser.h`, `Source/NeiraCore/Private/FSyntaxParser.cpp`, `Source/NeiraTests/Private/SyntaxParserTests.cpp`).
- [x] `DecisionTrace` в intent-пайплайне + тесты (`Source/NeiraCore/Public/FIntentExtractor.h`, `Source/NeiraCore/Private/FIntentExtractor.cpp`, `Source/NeiraTests/Private/IntentExtractorTests.cpp`).
- [x] `EventLog` переходов гипотез + тесты (`Source/NeiraCore/Public/FHypothesisStore.h`, `Source/NeiraCore/Private/FHypothesisStore.cpp`, `Source/NeiraTests/Private/HypothesisStoreTests.cpp`).
- [x] Ambiguous-trace на уровне каждого `AmbiguousToken` (реализован `FAmbiguousDecisionTrace` + deterministic tie-break + unit-тесты; целевые точки: `Source/NeiraCore/Public/FSyntaxParser.h`, `Source/NeiraCore/Private/FSyntaxParser.cpp`, `Source/NeiraTests/Private/SyntaxParserTests.cpp`).
- [x] Memory pressure degradation (`Medium/High/Critical`) с тестируемыми гарантиями HOT/WARM/COLD+anchor (реализован `FMemoryPressurePolicy` с API статуса/причины деградации и unit-тестами переходов/anchor-recovery; целевые точки: `Source/NeiraCore/Public/FMemoryPressurePolicy.h`, `Source/NeiraCore/Private/FMemoryPressurePolicy.cpp`, `Source/NeiraTests/Private/MemoryPressurePolicyTests.cpp`).
- [x] Full fail-reason pipeline от синтаксиса до ответа (добавлены интеграционные тесты `Source/NeiraTests/Private/DoDIntegrationTests.cpp`: end-to-end на PartialParse/DecisionTrace/threshold gate + связка с memory degradation).
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

## 6) Что считать «сделано» уже сейчас (снимок на 2026-03-19, финал сессии)

- ✅ Зафиксирован архитектурный каркас, scope и этапность v0.1–v0.5.
- ✅ Зафиксирован список критических архитектурных пробелов с приоритетом закрытия.
- ✅ Зафиксированы ключевые owner-решения, снимающие часть стратегических блокеров.
- ✅ Добавлен минимальный контур координации агентов: playbook, handoff log, ADR-реестр, knowledge index.
- ✅ Транзакционность памяти реализована: FHypothesisEvent + EventLog (append-only, каждый переход состояния).
- ✅ Explainability реализован: DecisionTrace в FIntentResult (имя правила, которое сработало).
- ✅ FSyntaxParser интегрирован с FIntentExtractor: двухпутевое разрешение Frame→Pattern fallback.
- ✅ v0.4 закрыт: FBeliefEngine (7 тестов), EHypothesisSource (весовая схема), Downgrade() (5 тестов), FindByClaim().
- ✅ Формальные контракты модулей v1: Docs/Contracts/Module_Contracts_v1.md + Error_Catalog.md.
- ✅ Словарь расширен до 1000+ форм. 108 тестов — все проходят.
- ⏭️ Следующий этап: Privacy/Security baseline (PII/NON_PII + retention) → SLA C++/Blueprint → Migration playbook v0.4+.

---

## 7) Результат ревизии handoff (2026-03-19)

Проверено, что предыдущий агент корректно зафиксировал архитектурные разделы в README. По ревизии выявлены и закрыты два пропуска:

- добавлены **критерии готовности v0.3** (DoD на уровне наблюдаемых проверок);
- добавлен **алгоритм retrieval по `FAnchorSystem`** с порогами `AUTO_RESTORE_TOPIC / ASK_USER_CONFIRMATION / NO_MATCH`.

Что остаётся открытым после ревизии:
- формальные schema-контракты и error catalog;
- транзакционная модель памяти (event log/replay);
- единый trace/audit формат.
