# Module Contracts v1 — Neira_V4

**schema_version:** 1
**Дата:** 2026-03-19
**Статус:** Active
**Следующая ревизия:** при добавлении новых полей в любую из структур

---

## Backward Compatibility

- Минорные изменения (добавление необязательного поля) — `schema_version` не меняется, новое поле документируется в разделе Changelog.
- Ломающие изменения (переименование, удаление поля, смена семантики) — `schema_version` инкрементируется, старые консьюмеры обязаны обновиться до интеграции.
- Поля с дефолтным значением в C++ (`= 0.0f`, `= EFoo::Unknown`) никогда не теряют это значение при эволюции схемы.

---

## 1. FMorphResult — результат морфологического анализа

**Модуль:** `FMorphAnalyzer`
**Файл:** `Source/NeiraCore/Public/FMorphAnalyzer.h`
**Производит:** `FMorphAnalyzer::Analyze()`, `FMorphAnalyzer::AnalyzePhrase()`
**Потребители:** `FSyntaxParser`, `FIntentExtractor` (через FSyntaxParser)

### Поля

| Поле | Тип | Обязательно | Диапазон / Допустимые значения | Инвариант |
|---|---|---|---|---|
| `OriginalWord` | `FString` | Да | Не пустая | Совпадает с входом метода `Analyze()` |
| `Lemma` | `FString` | Да | Не пустая | Начальная форма. Если источник `"dict"` — точное значение из словаря. Если `"suffix"` или `"unknown"` — равна нормализованному `OriginalWord` |
| `PartOfSpeech` | `EPosTag` | Да | Любое значение `EPosTag` | `EPosTag::Unknown` только при `Source == "unknown"` или при `Source == "suffix"` с неизвестным суффиксом |
| `Confidence` | `float` | Да | `[0.0f .. 1.0f]` | `0.95f` для `"dict"`, `0.65f` для `"suffix"`, `0.85f` для `"digit"`, `0.1f` для `"unknown"` |
| `Source` | `FString` | Да | `"dict"` ∣ `"suffix"` ∣ `"digit"` ∣ `"unknown"` | Один из четырёх допустимых тегов |

### Инварианты

- `Confidence > 0.0f` всегда (минимум `0.1f`).
- При `Source == "dict"`: `Confidence == 0.95f`, `PartOfSpeech != EPosTag::Unknown`.
- При `Source == "unknown"`: `Confidence == 0.1f`, `PartOfSpeech == EPosTag::Unknown`.
- `AnalyzePhrase()` возвращает ровно по одному `FMorphResult` на каждый пробел-разделённый токен (после удаления знаков препинания с конца).

### Предусловия вызова

- Вход `Word` — непустая строка, не содержит пробелов.
- Вход `Phrase` — строка, может содержать пробелы; знаки препинания на конце токена удаляются автоматически.

---

## 2. FIntentResult — результат извлечения намерения

**Модуль:** `FIntentExtractor`
**Файл:** `Source/NeiraCore/Public/FIntentExtractor.h`
**Производит:** `FIntentExtractor::Extract()`
**Потребители:** `FActionRegistry`, `FBeliefEngine`

### Поля

| Поле | Тип | Обязательно | Диапазон / Допустимые значения | Инвариант |
|---|---|---|---|---|
| `IntentID` | `EIntentID` | Да | Любое значение `EIntentID` | `EIntentID::Unknown` при FailReason != None |
| `EntityTarget` | `FString` | Нет | Может быть пустой | Пустая строка — допустима для `AnswerAbility`, `CheckMemoryLoad` |
| `Confidence` | `float` | Да | `[0.0f .. 1.0f]` | `0.0f` при `IntentID == Unknown`; `> 0.0f` при успешном извлечении |
| `DecisionTrace` | `FString` | Да | Не пустая | Всегда заполнена: одно из допустимых значений ниже |
| `FailReason` | `EActionFailReason` | Да | Любое значение | `None` только при `IntentID != Unknown`; при неудаче — конкретная причина |
| `DiagnosticNote` | `FString` | Нет | Может быть пустой | Дополнительный контекст для логов и тестов |

### Допустимые значения DecisionTrace

| Значение | Когда ставится |
|---|---|
| `"EmptyInput"` | Входная фраза пустая или только пробелы |
| `"Frame.AbilityCheck"` | `FSemanticFrame.bIsAbilityCheck == true` |
| `"Frame.StatementStoreFact"` | `PhraseType == Statement` |
| `"Frame.QuestionWithObject"` | Вопросная фраза с заполненным `Frame.Object` |
| `"Frame.Predicate:<глагол>"` | Командная/запросная фраза с известным предикатом |
| `"Pattern:<маркер>"` | Сработал строковый паттерн fallback (например `"Pattern:что такое"`) |
| `"Fallback:Unknown"` | Ни один путь не дал результата |

### Инварианты

- `DecisionTrace` никогда не пустая.
- Если `FailReason != None`, то `IntentID == EIntentID::Unknown` и `Confidence == 0.0f`.
- Если `IntentID != Unknown`, то `FailReason == None`.
- `EntityTarget` пустая строка — допустима; `null` — недопустима (всегда инициализирована).

### Предусловия вызова

- `Phrase` и `PhraseType` передаются от `FPhraseClassifier`. Пустая `Phrase` — допустима (вернёт `EmptyInput`).

---

## 3. FActionRequest — запрос на выполнение действия

**Модуль:** `FActionRegistry`
**Файл:** `Source/NeiraCore/Public/FActionTypes.h`
**Производит:** pipeline-координатор (конвертирует `FIntentResult`)
**Потребители:** `FActionRegistry::Execute()`

### Поля

| Поле | Тип | Обязательно | Диапазон / Допустимые значения | Инвариант |
|---|---|---|---|---|
| `ActionID` | `EActionID` | Да | Любое значение `EActionID` | `EActionID::Unknown` — допустим, вернёт `NotSupported` |
| `EntityTarget` | `FString` | Нет | Может быть пустой | Пустая строка — допустима |
| `Confidence` | `float` | Да | `[0.0f .. 1.0f]` | Если `< 0.5f` (LowConfidenceThreshold), `FActionRegistry` вернёт `LowConfidence` |

---

## 4. FActionResult — результат выполнения действия

**Модуль:** `FActionRegistry`
**Файл:** `Source/NeiraCore/Public/FActionTypes.h`
**Производит:** `FActionRegistry::Execute()` (и зарегистрированные обработчики)
**Потребители:** pipeline-координатор, генератор ответа

### Поля

| Поле | Тип | Обязательно | Диапазон / Допустимые значения | Инвариант |
|---|---|---|---|---|
| `bSuccess` | `bool` | Да | `true` / `false` | — |
| `ResultText` | `FString` | Условно | Может быть пустой | Для External-действий — непустая при `bSuccess == true`; для Internal — может быть пустой |
| `FailReason` | `EActionFailReason` | Да | Любое значение | `None` только при `bSuccess == true`; при `bSuccess == false` — не должен быть `None` |
| `DiagnosticNote` | `FString` | Нет | Может быть пустой | Причина сбоя для логов |

### Инварианты

- `bSuccess == false` → `FailReason != None`.
- `bSuccess == true` → `FailReason == None`.
- Генератор ответа обязан сформировать fallback-текст если `bSuccess == false` и `ResultText` пустой.

---

## 5. FHypothesis — единица знания

**Модуль:** `FHypothesisStore`
**Файл:** `Source/NeiraCore/Public/FHypothesisStore.h`
**Производит:** `FHypothesisStore::Store()`
**Потребители:** `FHypothesisStore::Find()`, `FBeliefEngine`

### Поля

| Поле | Тип | Обязательно | Диапазон / Допустимые значения | Инвариант |
|---|---|---|---|---|
| `Claim` | `FString` | Да | Не пустая | Утверждение в свободной форме |
| `Source` | `FString` | Нет | Может быть пустой | Комментарий об источнике (human-readable) |
| `Confidence` | `float` | Да | `[0.0f .. 1.0f]` | Не изменяется после `Store()` (кроме случаев явного переопределения) |
| `State` | `EKnowledgeState` | Да | Любое значение | Всегда `Pending` после `Store()` независимо от переданного значения |
| `Reason` | `FString` | Нет | Может быть пустой | Причина текущего статуса |
| `ConfirmCount` | `int32` | Да | `[0..∞)` | Всегда `0` после `Store()` |
| `SourceType` | `EHypothesisSource` | Да | Любое значение `EHypothesisSource` | Передаётся при `Store()`, далее не изменяется |

### Переходы состояний

```
Pending ──Confirm()──► Confirmed ──Confirm() × N──► Confirmed (ConfirmCount++)
                           │
                       Verify() при ConfirmCount >= 2
                           │
                           ▼
                     VerifiedKnowledge
                           │
                       Downgrade()
                           │
                           ▼
                        Conflicted
                           │
                       (любой) Downgrade из Confirmed → Pending (ConfirmCount=0)

MarkConflicted() → Conflicted из любого состояния
```

### Инварианты

- `State == Pending` всегда сразу после `Store()`.
- `ConfirmCount == 0` всегда сразу после `Store()` или после `Downgrade()` из `Confirmed`.
- `Verify()` возвращает `false` из `Pending` (требуется `Confirmed` + `ConfirmCount >= 2`).
- `Downgrade()` из `Pending`, `Conflicted`, `Deprecated` возвращает `false`.
- `Confirm()` из `Conflicted` или `Deprecated` возвращает `false`.

---

## 6. FHypothesisEvent — запись журнала переходов

**Модуль:** `FHypothesisStore`
**Файл:** `Source/NeiraCore/Public/FHypothesisStore.h`
**Производит:** каждый успешный переход в `FHypothesisStore`
**Потребители:** аудит, replay, тесты

### Поля

| Поле | Тип | Обязательно | Диапазон / Допустимые значения | Инвариант |
|---|---|---|---|---|
| `HypothesisID` | `int32` | Да | `>= 0` | Соответствует индексу гипотезы в `Hypotheses[]` |
| `FromState` | `EKnowledgeState` | Да | Любое значение | Состояние до перехода |
| `ToState` | `EKnowledgeState` | Да | Любое значение | Состояние после перехода; `FromState != ToState` |
| `MethodName` | `FString` | Да | `"Store"` ∣ `"Confirm"` ∣ `"Verify"` ∣ `"MarkConflicted"` ∣ `"Downgrade"` | Имя вызванного метода |
| `Reason` | `FString` | Нет | Может быть пустой | Причина перехода, переданная вызывающим |

### Инварианты

- В лог попадают только **успешные** переходы. Неудачные вызовы не пишутся.
- `FromState != ToState` всегда.
- Журнал append-only: записи не редактируются, только добавляются и очищаются (`ClearEventLog()`).

---

## 7. FBeliefDecision — решение движка убеждений

**Модуль:** `FBeliefEngine`
**Файл:** `Source/NeiraCore/Public/FBeliefEngine.h`
**Производит:** `FBeliefEngine::Process()`
**Потребители:** pipeline-координатор

### Поля

| Поле | Тип | Обязательно | Диапазон / Допустимые значения | Инвариант |
|---|---|---|---|---|
| `Action` | `EBeliefAction` | Да | Любое значение `EBeliefAction` | — |
| `HypothesisID` | `int32` | Да | `-1` или `>= 0` | `-1` при `Action == Rejected` или `Action == NoMatch` |
| `AppliedConfidence` | `float` | Да | `[0.0f .. 1.0f]` | `Confidence * SourceWeight`; `0.0f` при `Action == NoMatch` |
| `Reason` | `FString` | Да | Не пустая | Описание принятого решения |

### Весовая схема EHypothesisSource

| Source | Вес |
|---|---|
| `DeveloperReview` | `1.00f` |
| `ExternalValidation` | `0.95f` |
| `Dictionary` | `0.90f` |
| `UserConfirm` | `0.85f` |
| `AutoInference` | `0.60f` |
| `Unknown` | `0.50f` |

### Инварианты

- `AppliedConfidence >= 0.30f` → действие выполняется (`Created`, `Confirmed`, или `Verified`).
- `AppliedConfidence < 0.30f` → `Action == Rejected`, `HypothesisID == -1`.
- `IntentID != StoreFact` и `IntentID` не является запросным → `Action == NoMatch`.
- `Reason` никогда не пустая.

---

## Changelog

| Версия | Дата | Изменение |
|---|---|---|
| v1 | 2026-03-19 | Первая версия. Покрывает FMorphResult, FIntentResult, FActionRequest, FActionResult, FHypothesis, FHypothesisEvent, FBeliefDecision |
