// FPhraseClassifier.cpp
// v0.1 — классификация на паттернах, без морфологии.
//
// Алгоритм: два прохода.
//   1. Маркеры вопроса (вопросительные слова, '?') — наивысший приоритет.
//   2. Маркеры просьбы («скажи», «объясни», «расскажи»).
//   3. Маркеры команды (глагол в повелительном наклонении в начале).
//   4. Маркеры утверждения («—», «является», «это», «есть»).
//   5. Unknown — ни один маркер не сработал.
//
// Ограничения v0.1:
//   - Список маркеров жёстко прошит. Расширяется в v0.2 через словарь.
//   - Регистронезависимость обеспечивается ToLower() перед проверкой.
//   - Порядок проверки имеет значение: вопрос проверяется первым.

#include "FPhraseClassifier.h"

// ---------------------------------------------------------------------------
// Вспомогательные данные — маркеры для каждого типа
// ---------------------------------------------------------------------------

namespace
{
    // Вопросительные слова (начало фразы или любая позиция)
    const TArray<FString> QuestionMarkers = {
        TEXT("что такое"),
        TEXT("кто такой"),
        TEXT("кто такая"),
        TEXT("что означает"),
        TEXT("что значит"),
        TEXT("как называется"),
        TEXT("где находится"),
        TEXT("когда"),
        TEXT("почему"),
        TEXT("зачем"),
        TEXT("сколько"),
        TEXT("какой"),
        TEXT("какая"),
        TEXT("какие"),
    };

    // Просьба: глаголы-маркеры в начале фразы
    const TArray<FString> RequestStartMarkers = {
        TEXT("скажи"),
        TEXT("объясни"),
        TEXT("расскажи"),
        TEXT("помоги"),
        TEXT("покажи мне"),
        TEXT("скажи мне"),
        TEXT("объясни мне"),
        TEXT("расскажи мне"),
    };

    // Команда: глаголы в повелительном наклонении в начале фразы
    const TArray<FString> CommandStartMarkers = {
        TEXT("открой"),
        TEXT("закрой"),
        TEXT("найди"),
        TEXT("запусти"),
        TEXT("останови"),
        TEXT("проверь"),
        TEXT("сохрани"),
        TEXT("удали"),
        TEXT("покажи"),
        TEXT("запомни"),
    };

    // Утверждение: связочные маркеры в любой позиции
    const TArray<FString> StatementMarkers = {
        TEXT(" — это "),
        TEXT(" — "),
        TEXT(" является "),
        TEXT(" представляет собой "),
        TEXT(" это "),
        TEXT(" есть "),
    };

    bool StartsWithAny(const FString& Lower, const TArray<FString>& Markers)
    {
        for (const FString& M : Markers)
        {
            if (Lower.StartsWith(M))
                return true;
        }
        return false;
    }

    bool ContainsAny(const FString& Lower, const TArray<FString>& Markers)
    {
        for (const FString& M : Markers)
        {
            if (Lower.Contains(M))
                return true;
        }
        return false;
    }
}

// ---------------------------------------------------------------------------
// Реализация
// ---------------------------------------------------------------------------

EPhraseType FPhraseClassifier::Classify(const FString& Phrase) const
{
    const FString Trimmed = Phrase.TrimStartAndEnd();
    if (Trimmed.IsEmpty())
        return EPhraseType::Unknown;

    const FString Lower = Trimmed.ToLower();

    // 1. Вопрос: явный '?' или вопросительное слово.
    //    Исключение: если фраза начинается с маркера просьбы («скажи мне что такое X»),
    //    то приоритет отдаётся просьбе, а не вопросу.
    const bool bStartsWithRequest = StartsWithAny(Lower, RequestStartMarkers);
    if (Lower.EndsWith(TEXT("?")) || (!bStartsWithRequest && ContainsAny(Lower, QuestionMarkers)))
        return EPhraseType::Question;

    // 2. Просьба: «скажи мне», «объясни», «расскажи» в начале
    if (bStartsWithRequest)
        return EPhraseType::Request;

    // 3. Команда: глагол повелительного наклонения в начале
    if (StartsWithAny(Lower, CommandStartMarkers))
        return EPhraseType::Command;

    // 4. Утверждение: связочные маркеры
    if (ContainsAny(Lower, StatementMarkers))
        return EPhraseType::Statement;

    return EPhraseType::Unknown;
}
