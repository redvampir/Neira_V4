#pragma once

#include "CoreMinimal.h"
#include "NeiraTypes.h"
#include "FResponseGenerator.h"
#include "FMorphAgreement.h"

// ---------------------------------------------------------------------------
// FSyntacticStrategy
//
// Одна синтаксическая стратегия построения предложения.
// PatternFmt содержит плейсхолдеры:
// {SubjectNom}, {SubjectPrep}, {SubjectIns}, {Object}.
// ---------------------------------------------------------------------------
struct FSyntacticStrategy
{
    FString          StrategyId;
    EIntentID        IntentID    = EIntentID::Unknown;
    EConfidenceLevel Confidence  = EConfidenceLevel::Unknown;
    EResponseTone    Tone        = EResponseTone::Calm;
    FString          PatternFmt; // плейсхолдеры: {SubjectNom}, {SubjectPrep}, {SubjectIns}, {Object}
};

// ---------------------------------------------------------------------------
// FSentencePlanner
//
// Выбирает синтаксическую стратегию и строит натуральное русское предложение
// из семантического фрейма. Детерминирован: одинаковый RotationHint →
// одинаковая стратегия (нет случайности, только ротация по счётчику).
//
// Morph agreement (падеж/род/число) применяется через FMorphAgreement.
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

    // Заменить формы субъекта и {Object} в паттерне.
    static FString Fill(const FString& Pattern,
                        const FEntityTargetForms& SubjectForms,
                        const FString& Object);
};
