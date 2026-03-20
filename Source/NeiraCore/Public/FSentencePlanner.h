#pragma once

#include "CoreMinimal.h"
#include "NeiraTypes.h"
#include "FResponseGenerator.h"

// ---------------------------------------------------------------------------
// FSyntacticStrategy
//
// Одна синтаксическая стратегия построения предложения.
// PatternFmt содержит плейсхолдеры {Subject} и {Object}.
// Используется только номинатив — морфологическое согласование фаза 2.
// ---------------------------------------------------------------------------
struct FSyntacticStrategy
{
    FString          StrategyId;
    EIntentID        IntentID    = EIntentID::Unknown;
    EConfidenceLevel Confidence  = EConfidenceLevel::Unknown;
    EResponseTone    Tone        = EResponseTone::Calm;
    FString          PatternFmt; // плейсхолдеры: {Subject}, {Object}
};

// ---------------------------------------------------------------------------
// FSentencePlanner
//
// Выбирает синтаксическую стратегию и строит натуральное русское предложение
// из семантического фрейма. Детерминирован: одинаковый RotationHint →
// одинаковая стратегия (нет случайности, только ротация по счётчику).
//
// Фаза 1: стратегии с именительным падежом, без FMorphRealizer.
// Фаза 2: подключение FMorphRealizer для падежного согласования.
// ---------------------------------------------------------------------------
struct NEIRACORE_API FSentencePlanner
{
    FSentencePlanner();

    // Построить предложение.
    // RotationHint — SessionResponseCount из FDialoguePipeline.
    FString Plan(EIntentID        IntentID,
                 EConfidenceLevel Confidence,
                 EResponseTone    Tone,
                 const FString&   Subject,
                 const FString&   Object,
                 int32            RotationHint = 0) const;

    // ID выбранной стратегии (для FormatID и трассировки).
    FString GetStrategyId(EIntentID        IntentID,
                          EConfidenceLevel Confidence,
                          EResponseTone    Tone,
                          int32            RotationHint = 0) const;

    // Число стратегий для данной комбинации (для тестов).
    int32 GetStrategyCount(EIntentID        IntentID,
                           EConfidenceLevel Confidence,
                           EResponseTone    Tone) const;

private:
    TArray<FSyntacticStrategy> Library;
    void BuildLibrary();

    // Вернуть кандидатов: сначала точное совпадение по тону,
    // затем fallback на Calm, затем Unknown.
    TArray<FSyntacticStrategy> GetCandidates(EIntentID        IntentID,
                                              EConfidenceLevel Confidence,
                                              EResponseTone    Tone) const;

    // Заменить {Subject} и {Object} в паттерне.
    static FString Fill(const FString& Pattern,
                        const FString& Subject,
                        const FString& Object);
};
