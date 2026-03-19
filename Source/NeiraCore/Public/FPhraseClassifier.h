#pragma once

#include "CoreMinimal.h"
#include "NeiraTypes.h"

/**
 * FPhraseClassifier
 *
 * Первый шаг pipeline: определяет тип фразы по поверхностным признакам.
 * Не занимается морфологией — только паттерны и маркеры.
 *
 * Реализация: v0.1
 */
struct NEIRACORE_API FPhraseClassifier
{
    /**
     * Классифицировать фразу.
     * @param Phrase  исходный текст (может содержать знаки препинания)
     * @return тип фразы
     */
    EPhraseType Classify(const FString& Phrase) const;
};
