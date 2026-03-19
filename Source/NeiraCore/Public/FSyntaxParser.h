#pragma once

#include "CoreMinimal.h"
#include "NeiraTypes.h"
#include "FMorphAnalyzer.h"

// ---------------------------------------------------------------------------
// Семантический фрейм — результат синтаксического разбора
//
// Структура: кто → действие → что → кому
// ---------------------------------------------------------------------------
struct NEIRACORE_API FSemanticFrame
{
    struct FAmbiguousDecisionTrace
    {
        FString Token;                      // токен, по которому был конфликт
        int32 TokenIndex = INDEX_NONE;      // индекс токена в исходной фразе
        TArray<FString> CandidatePOS;       // кандидаты POS/интерпретаций
        FString SelectedPOS;                // выбранный вариант
        float Confidence = 0.0f;            // уверенность выбора [0..1]
        FString Reason;                     // причина (deterministic tie-break rule)
        FString Anchor;                     // контекстный якорь, повлиявший на выбор
    };

    FString Subject;      // субъект действия ("ты", "кот", "Москва")
    FString Predicate;    // глагол / действие ("открой", "является", "найди")
    FString Object;       // объект действия ("окно", "столицей", "определение")
    FString Recipient;    // адресат / бенефициар ("для меня", "тебе")
    TArray<FAmbiguousDecisionTrace> AmbiguityTrace; // трассировка разрешения POS-конфликтов

    // Флаги
    bool bIsAbilityCheck = false;  // «ты можешь X?» — проверка возможности, не команда
    bool bHasNestedClause= false;  // есть вложенная клауза («что», «чтобы», «если»)
    bool bIsNegated      = false;  // есть отрицание («не», «нельзя»)

    bool IsEmpty() const
    {
        return Subject.IsEmpty() && Predicate.IsEmpty() && Object.IsEmpty();
    }
};

// ---------------------------------------------------------------------------
// FSyntaxParser
//
// v0.3 — базовые синтаксические связи без полноценного парсера зависимостей.
//
// Алгоритм (одним проходом по токенам):
//   1. AnalyzePhrase() через FMorphAnalyzer — получить теги.
//   2. Первый глагол (Verb) → Predicate.
//   3. Токены до Predicate (Noun/Pronoun) → Subject (берём первый).
//   4. Токены после Predicate (Noun) → Object (берём первый).
//   5. Предлог «для»/«к» + следующий Noun → Recipient.
//   6. Местоимения «ты»/«вы» в Subject + «можешь»/«можете» в Predicate
//      → bIsAbilityCheck = true.
//   7. Союзы «что»/«чтобы»/«если» в любой позиции → bHasNestedClause = true.
//   8. Частица «не» или «нельзя» → bIsNegated = true.
//
// Омонимия v0.3: при неоднозначном POS токена — предпочитаем Verb в позиции
// Predicate (начало фразы), Noun — в позиции Object.
//
// Ограничения v0.3:
//   - Нет учёта падежей: «кот видит мышь» и «мышь видит кот» → разбираются
//     одинаково (порядок слов как главный сигнал).
//   - Вложенные клаузы помечаются флагом, но не рекурсивно разбираются.
//   - Координация («и X, и Y») → берётся первый найденный токен.
// ---------------------------------------------------------------------------
struct NEIRACORE_API FSyntaxParser
{
    /**
     * Разобрать фразу, получив семантический фрейм.
     * @param Phrase      исходная фраза
     * @param PhraseType  тип из FPhraseClassifier (влияет на интерпретацию)
     */
    FSemanticFrame Parse(const FString& Phrase, EPhraseType PhraseType) const;

private:
    FMorphAnalyzer MorphAnalyzer;
};
