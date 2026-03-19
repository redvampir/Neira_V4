# Error Catalog — Neira_V4

**schema_version:** 1
**Дата:** 2026-03-19
**Статус:** Active

Единый каталог кодов ошибок и состояний деградации по всему pipeline.

---

## Секция A: EActionFailReason

**Файл:** `Source/NeiraCore/Public/NeiraTypes.h`
**Область применения:** `FIntentResult::FailReason`, `FActionResult::FailReason`

| Код | Значение | Условие возникновения | Какой модуль ставит | Recovery |
|---|---|---|---|---|
| `None` | Успех | Операция завершилась успешно | Все модули | — |
| `EmptyInput` | Пустой вход | Входная строка пустая или состоит только из пробелов | `FIntentExtractor::Extract()` | Запросить у пользователя повторный ввод |
| `PartialParse` | Частичный разбор | Морфологический/синтаксический разбор завершился частично; намерение не извлечено полностью | `FIntentExtractor::Extract()` | Запросить уточнение или применить fallback-ответ |
| `UnknownIntent` | Неизвестное намерение | Все пути разрешения намерения (фреймовый + паттерновый) вернули Unknown | `FIntentExtractor::Extract()` | Сообщить пользователю о непонимании; предложить перефразировать |
| `NotFound` | Объект не найден | Запрошенный `EntityTarget` отсутствует в памяти / словаре | Обработчики `GetDefinition`, `GetWordFact`, `FindMeaning`, `RetrieveMemory` | Сообщить об отсутствии; предложить проверить написание |
| `NotSupported` | Действие не реализовано | `ActionID == Unknown` или обработчик не зарегистрирован | `FActionRegistry::Execute()` | Сообщить о нереализованной функции |
| `LowConfidence` | Низкая уверенность | `FActionRequest::Confidence < 0.5f` (LowConfidenceThreshold) | `FActionRegistry::Execute()` | Попросить пользователя уточнить; повторить с более явной формулировкой |
| `InternalError` | Внутренняя ошибка | Исключение или непредвиденное состояние в обработчике | Любой модуль | Логировать с `DiagnosticNote`; вернуть generic fallback-ответ |

### Инварианты использования EActionFailReason

- `FIntentResult::FailReason == None` тогда и только тогда, когда `IntentID != Unknown`.
- `FActionResult::FailReason == None` тогда и только тогда, когда `bSuccess == true`.
- При `bSuccess == false` значение `FailReason` не должно быть `None`.
- `InternalError` — последний резерв; предпочтительны более специфичные коды.

### Матрица: FailReason → тип ответа генератора

| FailReason | Поведение генератора |
|---|---|
| `None` | Использовать `ResultText` |
| `EmptyInput` | «Пожалуйста, введи что-нибудь» |
| `PartialParse` | «Не совсем понял, попробуй переформулировать» |
| `UnknownIntent` | «Не понимаю, что ты имеешь в виду» |
| `NotFound` | «[EntityTarget] мне не известен(а)» |
| `NotSupported` | «Это я пока не умею» |
| `LowConfidence` | «Уточни, пожалуйста — недостаточно уверен(а)» |
| `InternalError` | «Что-то пошло не так, попробуй ещё раз» |

---

## Секция B: EMemoryPolicyDegradationReason

**Файл:** `Source/NeiraCore/Public/FMemoryPressurePolicy.h`
**Область применения:** `FMemoryPolicyApplyResult::Reason`

| Код | Условие возникновения | Recovery |
|---|---|---|
| `None` | Политика применена без деградации | — |
| `WarmCompacted` | Warm-память сжата (объединение близких записей) | Приемлемо; целостность данных сохранена |
| `WarmSummarized` | Warm-память суммирована (потеря детали) | Приемлемо при Medium-давлении; логировать |
| `HotReducedToMinimal` | Hot-память сокращена до минимального набора | Критично; логировать, уведомить если возможно |
| `MissingRequiredAnchors` | Отсутствуют обязательные анкерные элементы | Блокирующее состояние; политика не применена |
| `MissingHotForCritical` | Нет Hot-записей при Critical-давлении | Политика `Critical` не может быть применена без Hot-данных |
| `AnchorContextMissing` | Анкерный контекст (`AnchorContextPairs`) отсутствует | Блокирующее состояние; политика не применена |

---

## Секция C: EBeliefAction — результаты движка убеждений

**Файл:** `Source/NeiraCore/Public/FBeliefEngine.h`
**Область применения:** `FBeliefDecision::Action`

| Код | Условие | Что означает для вызывающего |
|---|---|---|
| `Created` | StoreFact, `AppliedConfidence >= 0.30f` | Новая гипотеза добавлена в Store |
| `Confirmed` | Запросный IntentID, Claim найден в Store | Гипотеза получила ещё одно подтверждение |
| `Verified` | Запросный IntentID, Claim найден, ConfirmCount достиг MinConfirmCount | Гипотеза стала VerifiedKnowledge |
| `Rejected` | `AppliedConfidence < 0.30f` | Уверенность слишком мала; Store не изменён |
| `NoMatch` | IntentID не связан со знаниями / Claim не найден | Store не изменён; обработка продолжается по другому пути |

---

## Секция D: Коды DiagnosticNote — рекомендуемые форматы

`DiagnosticNote` — свободная строка, но рекомендуется следовать формату:

```
<ModuleName>:<code> <detail>
```

Примеры:
- `"FActionRegistry:LowConfidence 0.35 < threshold 0.50"`
- `"FIntentExtractor:UnknownIntent all paths returned Unknown"`
- `"FHypothesisStore:Downgrade forbidden state=Pending"`
- `"FBeliefEngine:Rejected AppliedConfidence=0.21 < 0.30"`

---

## Changelog

| Версия | Дата | Изменение |
|---|---|---|
| v1 | 2026-03-19 | Первая версия. EActionFailReason (8 кодов), EMemoryPolicyDegradationReason (7 кодов), EBeliefAction (5 кодов), DiagnosticNote формат |
