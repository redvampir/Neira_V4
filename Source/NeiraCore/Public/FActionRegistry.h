#pragma once

#include "CoreMinimal.h"
#include "NeiraTypes.h"
#include "FActionTypes.h"

/** Тип обработчика действия. */
using FActionHandler = TFunction<FActionResult(const FActionRequest&)>;

/**
 * FActionRegistry
 *
 * Реестр действий агента. Статическая таблица EActionID → FActionHandler.
 * Заполняется при старте. Добавление нового действия не меняет реестр —
 * только Register() + новый обработчик.
 *
 * Минимальный порог уверенности: 0.5f (константа LowConfidenceThreshold).
 *
 * Реализация: v0.1
 */
struct NEIRACORE_API FActionRegistry
{
    static constexpr float LowConfidenceThreshold = 0.5f;

    /** Зарегистрировать обработчик для действия. */
    void Register(EActionID ActionID, FActionHandler Handler);

    /** Проверить, зарегистрировано ли действие. */
    bool IsSupported(EActionID ActionID) const;

    /**
     * Выполнить действие.
     * Если действие не зарегистрировано → NotSupported.
     * Если Confidence < LowConfidenceThreshold → LowConfidence.
     */
    FActionResult Execute(const FActionRequest& Request) const;

private:
    TMap<EActionID, FActionHandler> Handlers;
};
