#pragma once

#include "CoreMinimal.h"
#include "NeiraTypes.h"

// ---------------------------------------------------------------------------
// Часть речи — используется в морфологическом анализе
// ---------------------------------------------------------------------------
enum class EPosTag : uint8
{
    Unknown,
    Noun,         // кот, синтаксис, Москва
    Verb,         // открой, является, искать
    Adjective,    // синтаксический, большой
    Adverb,       // быстро, очень
    Pronoun,      // ты, я, он
    Preposition,  // в, на, для, к
    Conjunction,  // и, но, что, чтобы
    Particle,     // не, же, ли
    Numeral,      // один, два, первый
};

// ---------------------------------------------------------------------------
// Результат морфологического анализа одного токена
// ---------------------------------------------------------------------------
struct NEIRACORE_API FMorphResult
{
    FString  OriginalWord;                      // слово как пришло
    FString  Lemma;                             // начальная форма
    EPosTag  PartOfSpeech = EPosTag::Unknown;
    float    Confidence   = 0.0f;               // [0..1]
    FString  Source;                            // "dict" | "suffix" | "unknown"
};

// ---------------------------------------------------------------------------
// Тип записи в словаре
// ---------------------------------------------------------------------------
struct FDictEntry
{
    FString  Word;   // нижний регистр, нормализованная форма
    FString  Lemma;
    EPosTag  POS;
};

// ---------------------------------------------------------------------------
// FMorphAnalyzer
//
// v0.2 — лёгкая морфология без внешних зависимостей.
// v0.6 — добавлена поддержка внешнего словаря OpenCorpora (JSON).
//
// Алгоритм:
//   1. Привести к нижнему регистру.
//   2. Поиск в словаре-ядре → confidence 0.95, source "dict".
//   3. Поиск во внешнем словаре (OpenCorpora) → confidence 0.90, source "ext_dict".
//   4. Если не найдено: суффиксные правила → confidence 0.65, source "suffix".
//   5. Не найдено ничего → EPosTag::Unknown, confidence 0.1, source "unknown".
//
// Омонимия v0.2: если слово встречается в словаре с несколькими POS,
// возвращается запись с наибольшим приоритетом (Verb > Noun > Adj).
// Контекстное разрешение — v0.3.
//
// Ограничения v0.2:
//   - Словарь содержит ~200 частотных слов-зёрен. Расширяется вручную.
//   - Суффиксные правила охватывают наиболее продуктивные окончания РЯ.
//   - Числа и имена собственные → Noun с confidence 0.5.
// ---------------------------------------------------------------------------
struct NEIRACORE_API FMorphAnalyzer
{
    /**
     * Конструктор. Инициализирует policy внешнего словаря и подготавливает
     * project-relative lookup для OpenCorpora.
     * По умолчанию full JSON грузится лениво, при первом внешнем lookup.
     */
    FMorphAnalyzer();

    /**
     * Проанализировать одно слово.
     * @param Word  слово (любой регистр, без пробелов)
     */
    FMorphResult Analyze(const FString& Word) const;

    /**
     * Разбить фразу на токены и проанализировать каждый.
     * Разделитель — пробел. Знаки препинания удаляются из конца токена.
     */
    TArray<FMorphResult> AnalyzePhrase(const FString& Phrase) const;

    /**
     * Загрузить внешний словарь из JSON файла.
     * @param Path  путь к JSON файлу (например, "opencorpora_dict.json")
     * @return true если словарь успешно загружен
     */
    bool LoadExternalDictionary(const FString& Path);

    /**
     * Проверить, загружен ли внешний словарь.
     */
    bool HasExternalDictionary() const;

    /**
     * Получить количество записей во внешнем словаре.
     */
    int32 GetExternalDictionarySize() const;

private:
    // Поиск во встроенном словаре
    const FDictEntry* FindInCoreDictionary(const FString& LowerWord) const;
    
    // Поиск во внешнем словаре
    const FDictEntry* FindInExternalDictionary(const FString& LowerWord) const;
    
    // Применение суффиксных правил
    FMorphResult ApplySuffixRules(const FString& Word) const;
    
    // Обработка чисел
    FMorphResult AnalyzeNumber(const FString& Word) const;
};
