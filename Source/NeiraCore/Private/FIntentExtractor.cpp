// FIntentExtractor.cpp
// v0.1 — извлечение намерения и объекта действия по шаблонам.
//
// Алгоритм:
//   Для каждого шаблона задана тройка:
//     { маркер-строка, позиция (начало/любая), EIntentID }
//   Первый совпавший шаблон даёт намерение.
//   Объект действия (EntityTarget) извлекается как токен после маркера.
//
//   Confidence:
//     0.9 — маркер точный («что такое», «что означает»)
//     0.7 — маркер менее специфичный («объясни», «расскажи»)
//     0.4 — намерение не определено (Unknown)
//
// Ограничения v0.1:
//   - Извлечение объекта: берём всё после маркера, убираем '?'.
//     Без морфологического разбора — до v0.2.
//   - Шаблоны жёстко прошиты. В v0.2 будут расширены через словарь.

#include "FIntentExtractor.h"

namespace
{
    struct FIntentPattern
    {
        FString     Marker;
        EIntentID   IntentID;
        float       Confidence;
        bool        bExtractAfterMarker;  // true — объект стоит после маркера
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

        // STORE_FACT — утверждение (StatementType)
        // Обрабатывается отдельно по PhraseType, см. ниже
    };

    // Убрать '?' и обрезать пробелы
    FString CleanEntity(const FString& Raw)
    {
        FString Result = Raw.TrimStartAndEnd();
        if (Result.EndsWith(TEXT("?")))
            Result = Result.LeftChop(1).TrimEnd();
        return Result;
    }
}

FIntentResult FIntentExtractor::Extract(const FString& Phrase, EPhraseType PhraseType) const
{
    FIntentResult Result;

    // Утверждение → STORE_FACT без поиска по шаблонам
    if (PhraseType == EPhraseType::Statement)
    {
        Result.IntentID    = EIntentID::StoreFact;
        Result.EntityTarget = Phrase.TrimStartAndEnd();
        Result.Confidence  = 0.8f;
        return Result;
    }

    const FString Lower = Phrase.ToLower().TrimStartAndEnd();

    for (const FIntentPattern& P : Patterns)
    {
        int32 MarkerPos = Lower.Find(P.Marker);
        if (MarkerPos == INDEX_NONE)
            continue;

        Result.IntentID   = P.IntentID;
        Result.Confidence = P.Confidence;

        if (P.bExtractAfterMarker)
        {
            // Берём оригинальную строку (не Lower) чтобы сохранить регистр сущности
            int32 EntityStart = MarkerPos + P.Marker.Len();
            FString Raw = Phrase.Mid(EntityStart);
            Result.EntityTarget = CleanEntity(Raw);
        }

        return Result;
    }

    // Ни один шаблон не сработал
    Result.IntentID    = EIntentID::Unknown;
    Result.Confidence  = 0.4f;
    Result.EntityTarget = TEXT("");
    return Result;
}
