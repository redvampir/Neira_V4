# Отчет о подготовке и runtime-интеграции OpenCorpora для Neira

> Этот документ описывает подготовку, конвертацию и текущее runtime-подключение внешнего словаря OpenCorpora.
>
> Текущий интеграционный статус словарей нужно смотреть в [Data/Dictionaries/README.md](../../Data/Dictionaries/README.md).

Подготовка артефактов: **2026-03-19**  
Статус на **2026-03-20**: **Integrated in runtime**

## Навигация

- [Что именно подтверждает этот отчет](#report-scope)
- [Итоговая сводка](#summary)
- [Результаты конвертации](#conversion-results)
- [Созданные файлы](#created-files)
- [Что еще не закрыто](#not-integrated)
- [Следующие шаги](#next-steps)
- [Словарный status doc](../../Data/Dictionaries/README.md)
- [README проекта](../../README.md)
- [Roadmap проекта](../Roadmap/Agent_Roadmap.md)

---

<a id="report-scope"></a>
## Что именно подтверждает этот отчет

Подтверждены такие факты:

- исходный XML-дамп OpenCorpora был получен;
- данные были сконвертированы в упрощенный JSON-формат;
- в репозитории присутствуют рабочие скрипты конвертации и локальные артефакты.
- `FMorphAnalyzer` умеет загружать `opencorpora_dict.json` во время выполнения;
- runtime lookup использует OpenCorpora как `ext_dict` после встроенного словаря;
- default cold-start policy = `lazy`, то есть JSON не парсится на пустом старте и поднимается только при первом внешнем lookup;
- size guard и режимы `off/lazy/eager` задаются через `NEIRA_EXTERNAL_DICT_MODE`, `NEIRA_EXTERNAL_DICT_PATH`, `NEIRA_EXTERNAL_DICT_MAX_MB`;
- внешняя загрузка подтверждена нативным тестом и живым прогоном через консоль.

Не подтверждены этим отчетом такие вещи:

- perf-budget или бинарный кэш для уменьшения cold-start;
- platform-neutral workflow, который отдельно валидирует словарную загрузку в CI.

---

<a id="summary"></a>
## Итоговая сводка

| Артефакт | Статус | Расположение | Объем |
|---|---|---|---|
| Встроенный словарь Neira | `Integrated` | `Source/NeiraCore/Private/FMorphAnalyzer.cpp` | 1000+ форм |
| OpenCorpora XML | `Prepared` | `Data/Dictionaries/annot.opcorpora.xml` | 532 МБ |
| OpenCorpora archive | `Prepared` | `Data/Dictionaries/annot.opcorpora.xml.zip` | 55 МБ |
| OpenCorpora JSON | `Integrated` | `../../Source/Tests/opencorpora_dict.json` | 387 МБ |
| Конвертер | `Prepared` | `Data/Dictionaries/convert_opencorpora_simple.py` | рабочий скрипт |

Ключевая формулировка: OpenCorpora **подготовлен и встроен в runtime** как внешний lookup-слой `ext_dict` в `FMorphAnalyzer`.

Текущая operational policy:

- default mode: `lazy`;
- manual override: `NEIRA_EXTERNAL_DICT_MODE=off|lazy|eager`;
- path override: `NEIRA_EXTERNAL_DICT_PATH=<path>`;
- size guard: `NEIRA_EXTERNAL_DICT_MAX_MB` (default `512`).

---

<a id="conversion-results"></a>
## Результаты конвертации

### Статистика словаря

```text
Unique words: 93,682
Total forms: 3,219,258
```

### Распределение по частям речи

| POS | Количество | Доля |
|---|---:|---:|
| Noun | 1,139,271 | 35.4% |
| Unknown | 776,317 | 24.1% |
| Adjective | 553,424 | 17.2% |
| Verb | 194,563 | 6.0% |
| Particle | 192,126 | 6.0% |
| Preposition | 188,848 | 5.9% |
| Conjunction | 148,288 | 4.6% |
| Numeral | 18,931 | 0.6% |
| Adverb | 7,490 | 0.2% |

### Что это значит для проекта

- словарь достаточно большой, чтобы резко поднять покрытие морфологии;
- размер JSON-дампа уже требует аккуратной runtime-стратегии;
- lookup `dict -> ext_dict -> suffix -> unknown` уже работает и снимает часть старых пробелов покрытия;
- доля `Unknown` показывает, что даже после интеграции качество не исчерпывается одной только массовой загрузкой словаря.

---

<a id="created-files"></a>
## Созданные файлы

```text
f:\Нейронки\Neira\
├── Source\Tests\opencorpora_dict.json
├── convert_opencorpora_simple.py
└── Data\Dictionaries\
    ├── annot.opcorpora.xml
    ├── annot.opcorpora.xml.zip
    ├── convert_opencorpora_simple.py
    ├── convert_opencorpora_to_neira.py
    ├── download_opencorpora_dict.py
    └── README.md
```

Функциональный смысл этих файлов:

- XML и архив — исходные данные;
- `convert_opencorpora_simple.py` — рабочая линия подготовки;
- `Source\Tests\opencorpora_dict.json` — текущий runtime-артефакт для внешнего lookup в `FMorphAnalyzer`;
- `README.md` в `Data/Dictionaries/` — текущий статусный документ по словарям.

---

<a id="not-integrated"></a>
## Что еще не закрыто

Следующие пункты остаются открытыми:

1. Нет бинарного кэша или облегченного runtime-пакета поверх полного OpenCorpora.
2. Нет отдельного perf-budget для словарной загрузки и lookup.
3. Нет platform-neutral regression-policy, которая отдельно проверяет внешний словарь в CI.

---

<a id="next-steps"></a>
## Следующие шаги

1. Выбрать между полным JSON-дампом и более легким бинарным/отфильтрованным кэшем.
2. Связать интеграцию словаря с отдельной perf/regression-проверкой.
3. Отдельно измерить first-miss latency и memory footprint для полного OpenCorpora.
4. Держать status docs и roadmap синхронизированными с фактическим runtime-состоянием.

---

## Сравнение с другими словарями

| Словарь | Уникальных слов | Форм | Размер | Лицензия |
|---|---:|---:|---:|---|
| Встроенный Neira | ~1000 | ~1000 | ~50 КБ | Internal |
| OpenCorpora | 93,682 | 3.2M | 387 МБ | CC BY-SA |
| pymorphy2-dicts | ~150K | ~5M | ~7 МБ | CC BY-SA |
| Russian National Corpus | ~500K | ~50M | ~5 ГБ | CC BY-SA |

---

## Ссылки

- OpenCorpora: https://opencorpora.org/
- Словарный статус проекта: [Data/Dictionaries/README.md](../../Data/Dictionaries/README.md)
- Roadmap: [Docs/Roadmap/Agent_Roadmap.md](../Roadmap/Agent_Roadmap.md)
