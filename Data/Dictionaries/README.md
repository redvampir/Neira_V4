# Словари для Neira

> Этот документ является основным source of truth по текущему состоянию словарей и их интеграции в runtime.
>
> Важное различие:
> - `Integrated` — словарь реально используется кодом во время выполнения;
> - `Prepared` — артефакт скачан или сконвертирован, но в runtime еще не подключен;
> - `Planned` — формат или точка интеграции описаны, но самого рабочего артефакта в проекте пока нет.

Дата актуализации: **2026-03-20**

## Навигация

- [Текущий статус](#dictionary-status)
- [Каталог артефактов](#artifact-catalog)
- [Как морфология работает сейчас](#runtime-flow)
- [Как подключать внешние словари](#external-integration)
- [Следующие шаги](#next-steps)
- [README проекта](../../README.md)
- [Roadmap проекта](../../Docs/Roadmap/Agent_Roadmap.md)
- [Отчет по OpenCorpora](../../Docs/Knowledge/OpenCorpora_Integration_Success.md)
- [Контракт `FMorphResult`](../../Docs/Contracts/Module_Contracts_v1.md)

---

<a id="dictionary-status"></a>
## Текущий статус

| Источник | Статус | Фактическое состояние | Следующий шаг |
|---|---|---|---|
| Встроенный Core Dictionary | `Integrated` | Словарь зашит в `Source/NeiraCore/Private/FMorphAnalyzer.cpp` и участвует в runtime-анализе. | Расширять без ломки контракта `FMorphResult`. |
| Проектный JSON-словарь `neira_dict.json` | `Planned` | Формат описан, но файла `Data/Dictionaries/neira_dict.json` в текущем репозитории нет. Runtime-loader тоже не реализован. | Добавить loader и положить либо реальный словарь, либо минимальный sample-файл. |
| OpenCorpora XML/JSON артефакты | `Integrated` | `FMorphAnalyzer` умеет автоматически находить `opencorpora_dict.json`, держит его в shared cache и использует как `ext_dict` после встроенного словаря. По умолчанию загрузка идет в `lazy`-режиме, с size guard и env-policy. | Решить, нужен ли более легкий бинарный кэш и отдельный perf-budget для cold-start. |

Коротко: сейчас в runtime работают и встроенный словарь, и OpenCorpora. Приоритет lookup такой: `dict -> ext_dict -> suffix -> unknown`.

---

<a id="artifact-catalog"></a>
## Каталог артефактов

| Артефакт | Путь | Назначение | Статус |
|---|---|---|---|
| Core dictionary | `Source/NeiraCore/Private/FMorphAnalyzer.cpp` | Встроенные словарные формы для runtime | `Integrated` |
| OpenCorpora XML | `Data/Dictionaries/annot.opcorpora.xml` | Исходный XML-дамп | `Prepared` |
| OpenCorpora archive | `Data/Dictionaries/annot.opcorpora.xml.zip` | Архив исходного дампа | `Prepared` |
| Рабочий конвертер | `Data/Dictionaries/convert_opencorpora_simple.py` | Конвертация OpenCorpora в упрощенный формат | `Prepared` |
| Исторический конвертер | `Data/Dictionaries/convert_opencorpora_to_neira.py` | Более ранняя версия конвертера | `Prepared` |
| Загрузчик/получение исходника | `Data/Dictionaries/download_opencorpora_dict.py` | Скрипт скачивания/подготовки исходных данных | `Prepared` |
| Сконвертированный JSON (fixture в репозитории) | `Data/Dictionaries/opencorpora_dict.json` | Минимальный runtime JSON для автотеста внешнего словаря; путь может быть переопределен через `NEIRA_EXTERNAL_DICT_PATH` | `Integrated` |
| Проектный JSON-словарь | `Data/Dictionaries/neira_dict.json` | Предполагаемый легкий внешний словарь проекта | `Planned` |

---

<a id="runtime-flow"></a>
## Как морфология работает сейчас

Текущий runtime-path в `FMorphAnalyzer` такой:

1. Поиск во встроенном словаре.
2. Если слово не найдено, поиск во внешнем словаре OpenCorpora.
3. Если и там нет совпадения, применяются суффиксные правила.
4. Если ничего не сработало, возвращается `Unknown`.

Практический результат:

- `dict` -> `confidence = 0.95`
- `ext_dict` -> `confidence = 0.90`
- `suffix` -> `confidence = 0.65`
- `unknown` -> `confidence = 0.1`

Это соответствует контракту `FMorphResult`, зафиксированному в [Module_Contracts_v1.md](../../Docs/Contracts/Module_Contracts_v1.md).

---

## Словари по типам

### 1. Встроенный словарь

- Расположение: `Source/NeiraCore/Private/FMorphAnalyzer.cpp`
- Объем: 1000+ форм
- Статус: `Integrated`

Содержит наиболее частотные слова и доменную лексику Neira:

- местоимения;
- служебные части речи;
- частотные глаголы;
- существительные;
- прилагательные;
- наречия;
- числительные и кванторы.

### 2. Проектный JSON-словарь

- Целевой путь: `Data/Dictionaries/neira_dict.json`
- Статус: `Planned`

Формат остается актуальным и может использоваться как контракт для будущей загрузки:

```json
{
  "entries": [
    {"word": "нейросеть", "lemma": "нейросеть", "pos": "Noun"},
    {"word": "нейросети", "lemma": "нейросеть", "pos": "Noun"},
    {"word": "искусственный", "lemma": "искусственный", "pos": "Adjective"}
  ]
}
```

Поддерживаемые POS:

- `Noun`
- `Verb`
- `Adjective`
- `Adverb`
- `Pronoun`
- `Preposition`
- `Conjunction`
- `Particle`
- `Numeral`

Важно: это описание формата, а не подтверждение наличия файла в текущем checkout.

### 3. OpenCorpora как внешний источник

- Артефакты: XML и сконвертированный JSON присутствуют локально
- Статус: `Integrated`

Что уже сделано:

- исходный XML получен;
- рабочий скрипт конвертации присутствует;
- крупный JSON-дамп создан.
- `FMorphAnalyzer` поддерживает `LoadExternalDictionary(const FString& Path)`;
- конструктор `FMorphAnalyzer` автоматически ищет `opencorpora_dict.json` по project-relative путям;
- внешний словарь загружается один раз в общий кэш и переиспользуется между экземплярами анализатора;
- cold-start policy по умолчанию = `lazy`: словарь не парсится на пустом старте и поднимается при первом реальном внешнем lookup;
- memory/cold-start guard задаются через `NEIRA_EXTERNAL_DICT_MODE`, `NEIRA_EXTERNAL_DICT_PATH`, `NEIRA_EXTERNAL_DICT_MAX_MB`.
- внешний lookup работает в runtime и покрыт тестом `Neira.MorphAnalyzer.ExternalDictionary.AutoLoadAndLookup`.
- для стабильности CI в репозитории хранится минимальный fixture `Data/Dictionaries/opencorpora_dict.json`, чтобы тест внешнего словаря не зависел от локального окружения разработчика.

Что еще не сделано:

- нет бинарного кэша или отфильтрованного lightweight-пакета;
- нет отдельного perf-budget для словарной загрузки в CI.

---

<a id="external-integration"></a>
## Как подключать внешние словари

Текущий внешний loader уже есть в `FMorphAnalyzer`:

```cpp
bool LoadExternalDictionary(const FString& Path);
```

Текущее поведение runtime:

- при создании `FMorphAnalyzer` делается авто-поиск `opencorpora_dict.json`, но без eager-parse по умолчанию;
- поддерживаются project-relative пути, чтобы словарь поднимался из корня репозитория и из подпапок вроде `Source/ConsoleApp` и `Source/Tests`;
- при успешной загрузке словарь живет в общем кэше, а не копируется в каждый экземпляр анализатора;
- default policy: `NEIRA_EXTERNAL_DICT_MODE=lazy`, `NEIRA_EXTERNAL_DICT_MAX_MB=512`;
- поддерживаются режимы `off`, `lazy`, `eager`, а `NEIRA_EXTERNAL_DICT_PATH` позволяет переопределить путь.

### Шаг 1. Выбрать тип внешнего источника

Есть два практических пути:

- легкий проектный JSON в `Data/Dictionaries/neira_dict.json`;
- большой OpenCorpora-дамп из `opencorpora_dict.json` или явного `NEIRA_EXTERNAL_DICT_PATH`.

### Шаг 2. Поддерживать порядок приоритетов

Текущая последовательность такая:

```cpp
// 1. Встроенный словарь
// 2. Внешний словарь
// 3. Суффиксные правила
// 4. Unknown
```

### Шаг 3. Поддерживать policy-ограничения

Текущее policy-поведение уже зафиксировано в коде:

- full JSON не грузится на idle/startup, если режим = `lazy`;
- auto-load ограничен size guard через `NEIRA_EXTERNAL_DICT_MAX_MB`;
- shared cache живет на процесс и не дублируется в каждом экземпляре анализатора.

Следующий уровень зрелости:

- бинарный или предварительно отфильтрованный кэш вместо полного JSON;
- отдельный perf-budget по времени cold-start и first-miss latency;
- CI-проверка для словарной policy, а не только для качества intent/frame.

---

<a id="next-steps"></a>
## Следующие шаги

1. Решить, нужен ли runtime-load полного OpenCorpora, или нужен более легкий бинарный/отфильтрованный кэш.
2. Зафиксировать perf-budget для cold-start и first external lookup.
3. Положить в `Data/Dictionaries/` либо реальный `neira_dict.json`, либо минимальный sample-файл для документационного контракта.
4. Привязать любые изменения словарной интеграции к regression-набору, perf-проверке и обновлению roadmap.

---

## Внешние источники

- OpenCorpora: https://opencorpora.org/
- pymorphy2 docs: https://pymorphy2.readthedocs.io/
- Russian National Corpus: https://ruscorpora.ru/
