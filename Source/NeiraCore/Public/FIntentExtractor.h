#pragma once

#include "CoreMinimal.h"
#include "NeiraTypes.h"

/**
 * FIntentResult
 *
 * Результат извлечения намерения: само намерение + объект действия
 * + уверенность. Уверенность нужна ActionRegistry для проверки порога.
 */
struct NEIRACORE_API FIntentResult
{
    EIntentID  IntentID    = EIntentID::Unknown;
    FString    EntityTarget;          // "кот", "окно", "текст"
    float      Confidence  = 0.0f;    // [0..1]
};

/**
 * FIntentExtractor
 *
 * Второй шаг pipeline: из классифицированной фразы извлекает намерение
 * и объект действия. Работает по шаблонам v0.1.
 *
 * Реализация: v0.1
 */
struct NEIRACORE_API FIntentExtractor
{
    /**
     * Извлечь намерение из фразы.
     * @param Phrase      исходный текст
     * @param PhraseType  результат FPhraseClassifier
     * @return FIntentResult
     */
    FIntentResult Extract(const FString& Phrase, EPhraseType PhraseType) const;
};
