#pragma once

#include "CoreMinimal.h"
#include "NeiraTypes.h"
#include "FSyntaxParser.h"

/**
 * FIntentResult
 *
 * Результат извлечения намерения: само намерение + объект действия
 * + уверенность + трассировка принятого решения.
 * Уверенность нужна ActionRegistry для проверки порога.
 *
 * DecisionTrace — строка-описание того, какой путь сработал при извлечении.
 * Формат: "Frame.AbilityCheck" | "Frame.Question+Object" | "Frame.Predicate:найти"
 *         | "PhraseType:Statement" | "Pattern:что такое" | "Fallback:Unknown"
 * Используется для отладки и regression-анализа. В v0.4 станет основой
 * для объяснения решений самообучения.
 */
struct NEIRACORE_API FIntentResult
{
    EIntentID  IntentID      = EIntentID::Unknown;
    FString    EntityTarget;              // "кот", "окно", "текст"
    float      Confidence    = 0.0f;     // [0..1]
    FString    DecisionTrace;            // путь принятия решения
    EActionFailReason FailReason = EActionFailReason::None; // причина неуспеха pipeline
    FString    DiagnosticNote;           // диагностическая причина для логов/тестов
};

/**
 * FIntentExtractor
 *
 * Второй шаг pipeline: из классифицированной фразы извлекает намерение
 * и объект действия.
 *
 * v0.1: работал только по строковым шаблонам (маркер-подстрока).
 * v0.3: интегрирован с FSyntaxParser. Основной путь — анализ FSemanticFrame.
 *       Fallback на строковые шаблоны сохранён для случаев, когда фрейм
 *       не даёт достаточно информации (пустой Object, неизвестный Predicate).
 *
 * Порядок разрешения намерения:
 *   1. Frame.bIsAbilityCheck → AnswerAbility
 *   2. PhraseType == Statement → StoreFact
 *   3. Frame.Predicate + Frame.Object → FindMeaning / GetWordFact / GetDefinition
 *   4. Fallback: строковые шаблоны v0.1
 *   5. Unknown
 *
 * Реализация: v0.3
 */
struct NEIRACORE_API FIntentExtractor
{
    /**
     * Извлечь намерение из фразы.
     * @param Phrase      исходный текст
     * @param PhraseType  результат FPhraseClassifier
     * @return FIntentResult с заполненным DecisionTrace
     */
    FIntentResult Extract(const FString& Phrase, EPhraseType PhraseType) const;

private:
    FSyntaxParser SyntaxParser;

    /** Извлечь намерение из семантического фрейма. Возвращает Unknown если фрейм недостаточен. */
    FIntentResult ExtractFromFrame(const FSemanticFrame& Frame,
                                   const FString&        OriginalPhrase,
                                   EPhraseType           PhraseType) const;

    /** Fallback: строковые шаблоны v0.1. */
    FIntentResult ExtractFromPatterns(const FString& Phrase) const;
};
