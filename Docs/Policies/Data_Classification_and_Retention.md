# Data Policy v0.6 — минимальная классификация и retention

Дата: **2026-03-20**  
Owner: **Core Platform / Knowledge Pipeline**

## 1) Классификация данных

| Класс | Определение | Где встречается |
|---|---|---|
| `NON_PII` | Данные, не содержащие персональных идентификаторов пользователя. | `Claim` и `Reason` в общих знаниях, технические trace-метки. |
| `PII` | Персональные данные (телефон, email, имя+контекст и т.п.). | `Claim`/`Source` при сохранении пользовательских фактов. |

## 2) Базовые правила хранения

1. По умолчанию любая гипотеза считается `NON_PII`.
2. `PII` запрещено сохранять без явного разрешения (`bPIIAllowed=true`).
3. При нарушении правила запись отклоняется до попадания в хранилище.

## 3) Retention и удаление

Политика задаётся в `FDataRetentionPolicy`:

- `NonPIIRetentionOps` — окно хранения `NON_PII` в условных шагах (operations sequence).
- `PIIRetentionOps` — окно хранения `PII` (обычно меньше, чем для `NON_PII`).

Очистка выполняется через `PurgeExpired()`:

- переводит запись в `Deprecated`;
- очищает полезную нагрузку (`Claim`, `Source`, confidence/confirm_count);
- пишет событие `RetentionPurge` в event-log.

## 4) Точки применения policy в pipeline

1. **Гипотезы/факты (`FHypothesisStore::Store`)**
   - guard `CanStoreHypothesis`: блокировка `PII` без разрешения.
2. **Event log (`FHypothesisEvent`)**
   - каждое событие имеет `DataClass`, чтобы downstream-аудит различал PII/NON_PII путь.
3. **Логирование/объяснимость (`FBeliefEngine`)**
   - при попытке записи, отклонённой policy, возвращается `Rejected` с privacy-reason.

## 5) Минимальные критерии контроля

- [x] Есть тест на блокировку `PII` без явного разрешения.
- [x] Есть тест на retention purge и появление `RetentionPurge` в event-log.
- [x] Policy задокументирована как отдельный документ и привязана к roadmap.
