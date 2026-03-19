// FIntentExtractor.cpp
// v0.3 — интеграция с FSyntaxParser, DecisionTrace.
//
// Алгоритм (два пути):
//
//   Путь 1: Frame-анализ (приоритет).
//     FSyntaxParser.Parse() строит FSemanticFrame. Затем ExtractFromFrame()
//     пробует разрешить Intent по структурным признакам фрейма:
//       1. Frame.bIsAbilityCheck → AnswerAbility
//       2. PhraseType == Statement → StoreFact
//       3. Predicate ∈ {найти} + Object ∈ {значение, определение, смысл} → FindMeaning
//       4. Predicate ∈ {рассказать, объяснить, сказать} + Object непустой → GetWordFact
//       5. PhraseType == Question + Object непустой → GetDefinition
//     Если всё выше не сработало (Object пустой, Predicate неизвестный) → Unknown из фрейма.
//
//   Путь 2: Fallback — строковые шаблоны v0.1.
//     Если путь 1 вернул Unknown → ExtractFromPatterns() ищет маркер-подстроку.
//     Это backward-compatibility: старые тесты продолжают проходить.
//
//   DecisionTrace заполняется всегда — описывает какой именно путь/правило сработало.
//
// EntityTarget:
//   - Frame-путь: берётся из Frame.Object (сохраняет оригинальный регистр).
//   - Pattern-путь: берётся как текст после маркера (поведение v0.1).
//   - Statement: берётся полная фраза.
//
// Ограничения v0.3:
//   - Координация («найди X и Y») не поддерживается — берётся первый объект.
//   - Лемматизация предикатов: сравниваются леммы (рассказать/расскажи → одна лемма).

#include "FIntentExtractor.h"

namespace
{
    // -----------------------------------------------------------------------
    // Маркеры для Fallback — строковые шаблоны (v0.1, без изменений)
    // -----------------------------------------------------------------------

    struct FIntentPattern
    {
        FString   Marker;
        EIntentID IntentID;
        float     Confidence;
        bool      bExtractAfterMarker;
    };

    // Порядок важен: более специфичные маркеры — выше
    const TArray<FIntentPattern> Patterns = {
        // GET_DEFINITION
        { TEXT("что такое "),          EIntentID::GetDefinition, 0.9f, true  },
        { TEXT("кто такой "),          EIntentID::GetDefinition, 0.9f, true  },
        { TEXT("кто такая "),          EIntentID::GetDefinition, 0.9f, true  },
        { TEXT("что означает слово "), EIntentID::GetDefinition, 0.9f, true  },
        { TEXT("что означает "),       EIntentID::GetDefinition, 0.9f, true  },
        { TEXT("что значит "),         EIntentID::GetDefinition, 0.9f, true  },

        // FIND_MEANING
        { TEXT("найди значение "),     EIntentID::FindMeaning,   0.9f, true  },
        { TEXT("найди определение "),  EIntentID::FindMeaning,   0.9f, true  },

        // GET_WORD_FACT
        { TEXT("расскажи про "),       EIntentID::GetWordFact,   0.7f, true  },
        { TEXT("расскажи о "),         EIntentID::GetWordFact,   0.7f, true  },

        // ANSWER_ABILITY
        { TEXT("ты можешь "),          EIntentID::AnswerAbility, 0.8f, false },
        { TEXT("можешь ли ты "),       EIntentID::AnswerAbility, 0.8f, false },
        { TEXT("умеешь ли "),          EIntentID::AnswerAbility, 0.8f, false },

        // STORE_FACT — обрабатывается в ExtractFromFrame по PhraseType
    };

    // Убрать '?' и обрезать пробелы
    FString CleanEntity(const FString& Raw)
    {
        FString Result = Raw.TrimStartAndEnd();
        if (Result.EndsWith(TEXT("?")))
            Result = Result.LeftChop(1).TrimEnd();
        return Result;
    }

    // -----------------------------------------------------------------------
    // Вспомогательные предикаты для Frame-анализа
    // -----------------------------------------------------------------------

    bool IsFindPredicate(const FString& LemmaLower)
    {
        return LemmaLower == TEXT("найти")
            || LemmaLower == TEXT("найди")
            || LemmaLower == TEXT("искать");
    }

    bool IsTellPredicate(const FString& LemmaLower)
    {
        return LemmaLower == TEXT("рассказать")
            || LemmaLower == TEXT("объяснить")
            || LemmaLower == TEXT("сказать")
            || LemmaLower == TEXT("расскажи")
            || LemmaLower == TEXT("объясни")
            || LemmaLower == TEXT("скажи");
    }

    bool IsDefinitionObject(const FString& ObjectLower)
    {
        return ObjectLower == TEXT("значение")
            || ObjectLower == TEXT("определение")
            || ObjectLower == TEXT("смысл")
            || ObjectLower == TEXT("значения")
            || ObjectLower == TEXT("определения");
    }

    // Мета-слова — функциональные слова, не являющиеся целью запроса.
    // Например: «что означает слово X» → Object="слово", но целевой объект — X.
    // В таких случаях Frame не может извлечь правильный EntityTarget, и нужен Fallback.
    bool IsMetaWord(const FString& ObjectLower)
    {
        return ObjectLower == TEXT("слово")
            || ObjectLower == TEXT("слова")
            || ObjectLower == TEXT("термин")
            || ObjectLower == TEXT("понятие")
            || ObjectLower == TEXT("выражение");
    }
}

// ---------------------------------------------------------------------------
// Путь 1: Frame-анализ
// ---------------------------------------------------------------------------

FIntentResult FIntentExtractor::ExtractFromFrame(const FSemanticFrame& Frame,
                                                  const FString&        OriginalPhrase,
                                                  EPhraseType           PhraseType) const
{
    FIntentResult Result;

    // 1. Проверка возможности → AnswerAbility
    if (Frame.bIsAbilityCheck)
    {
        Result.IntentID      = EIntentID::AnswerAbility;
        Result.EntityTarget  = Frame.Object;
        Result.Confidence    = 0.85f;
        Result.DecisionTrace = TEXT("Frame.AbilityCheck");
        return Result;
    }

    // 2. Утверждение → StoreFact
    if (PhraseType == EPhraseType::Statement)
    {
        Result.IntentID      = EIntentID::StoreFact;
        Result.EntityTarget  = OriginalPhrase.TrimStartAndEnd();
        Result.Confidence    = 0.8f;
        Result.DecisionTrace = TEXT("PhraseType:Statement");
        return Result;
    }

    const FString PredLower   = Frame.Predicate.ToLower();
    const FString ObjectLower = Frame.Object.ToLower();

    // 3. Predicate ∈ {найти} + Object ∈ {значение, определение, смысл} → FindMeaning
    if (IsFindPredicate(PredLower) && IsDefinitionObject(ObjectLower))
    {
        Result.IntentID      = EIntentID::FindMeaning;
        Result.EntityTarget  = Frame.Object;
        Result.Confidence    = 0.9f;
        Result.DecisionTrace = TEXT("Frame.Predicate:найти+DefinitionObject");
        return Result;
    }

    // 4. Predicate ∈ {рассказать, объяснить, сказать} + Object непустой → GetWordFact
    if (IsTellPredicate(PredLower) && !Frame.Object.IsEmpty())
    {
        Result.IntentID      = EIntentID::GetWordFact;
        Result.EntityTarget  = Frame.Object;
        Result.Confidence    = 0.75f;
        Result.DecisionTrace = TEXT("Frame.Predicate:рассказать/объяснить");
        return Result;
    }

    // 5. Question + Object непустой + Object не мета-слово → GetDefinition
    //    Мета-слова («слово», «термин» и т.п.) не являются целевым объектом:
    //    «что означает слово X» — Object="слово", цель — X.
    //    В таких случаях Frame недостаточен → Fallback на паттерны.
    if (PhraseType == EPhraseType::Question && !Frame.Object.IsEmpty()
        && !IsMetaWord(ObjectLower))
    {
        Result.IntentID      = EIntentID::GetDefinition;
        Result.EntityTarget  = Frame.Object;
        Result.Confidence    = 0.85f;
        Result.DecisionTrace = TEXT("Frame.Question+Object");
        return Result;
    }

    // Фрейм не дал достаточно информации
    Result.IntentID      = EIntentID::Unknown;
    Result.Confidence    = 0.0f;
    Result.DecisionTrace = TEXT("");
    return Result;
}

// ---------------------------------------------------------------------------
// Путь 2: Fallback — строковые шаблоны v0.1
// ---------------------------------------------------------------------------

FIntentResult FIntentExtractor::ExtractFromPatterns(const FString& Phrase) const
{
    FIntentResult Result;
    const FString Lower = Phrase.ToLower().TrimStartAndEnd();

    for (const FIntentPattern& P : Patterns)
    {
        int32 MarkerPos = Lower.Find(P.Marker);
        if (MarkerPos == INDEX_NONE)
            continue;

        Result.IntentID      = P.IntentID;
        Result.Confidence    = P.Confidence;
        Result.DecisionTrace = FString::Printf(TEXT("Pattern:%s"), *P.Marker.TrimStartAndEnd());

        if (P.bExtractAfterMarker)
        {
            // Берём оригинальную строку (не Lower) чтобы сохранить регистр сущности
            int32 EntityStart   = MarkerPos + P.Marker.Len();
            FString Raw         = Phrase.Mid(EntityStart);
            Result.EntityTarget = CleanEntity(Raw);
        }

        return Result;
    }

    // Ни один шаблон не сработал
    Result.IntentID      = EIntentID::Unknown;
    Result.Confidence    = 0.4f;
    Result.EntityTarget  = TEXT("");
    Result.DecisionTrace = TEXT("Fallback:Unknown");
    return Result;
}

// ---------------------------------------------------------------------------
// Публичный метод
// ---------------------------------------------------------------------------

FIntentResult FIntentExtractor::Extract(const FString& Phrase, EPhraseType PhraseType) const
{
    const FString Trimmed = Phrase.TrimStartAndEnd();

    if (Trimmed.IsEmpty())
    {
        FIntentResult Empty;
        Empty.IntentID      = EIntentID::Unknown;
        Empty.Confidence    = 0.0f;
        Empty.DecisionTrace = TEXT("EmptyInput");
        return Empty;
    }

    // Путь 1: Frame-анализ
    const FSemanticFrame Frame = SyntaxParser.Parse(Trimmed, PhraseType);
    FIntentResult FrameResult  = ExtractFromFrame(Frame, Trimmed, PhraseType);

    if (FrameResult.IntentID != EIntentID::Unknown)
        return FrameResult;

    // Путь 2: Fallback на строковые шаблоны
    return ExtractFromPatterns(Trimmed);
}
