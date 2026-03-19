#pragma once

#include "CoreMinimal.h"
#include "NeiraTypes.h"

/**
 * FActionRequest — вход для любого действия.
 */
struct NEIRACORE_API FActionRequest
{
    EActionID  ActionID      = EActionID::Unknown;
    FString    EntityTarget;          // объект действия
    float      Confidence    = 0.0f;  // уверенность интерпретации [0..1]
};

/**
 * FActionResult — выход любого действия.
 * Пустой ResultText допустим только для Internal-действий.
 * Пустой ответ при bSuccess=false недопустим: генератор обязан сформировать фallback.
 */
struct NEIRACORE_API FActionResult
{
    bool              bSuccess       = false;
    FString           ResultText;         // для External-действий; пусто для Internal
    EActionFailReason FailReason     = EActionFailReason::InternalError; // при bSuccess=false не должен быть None
    FString           DiagnosticNote;     // причина сбоя для логов
};
