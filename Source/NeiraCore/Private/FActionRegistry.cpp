// FActionRegistry.cpp
// v0.1 — реестр действий агента.
//
// Порядок проверок в Execute():
//   1. Confidence < LowConfidenceThreshold → LowConfidence (до поиска обработчика)
//   2. Обработчик не найден → NotSupported + DiagnosticNote
//   3. Вызов обработчика → результат пробрасывается как есть

#include "FActionRegistry.h"

void FActionRegistry::Register(EActionID ActionID, FActionHandler Handler)
{
    Handlers.Add(ActionID, MoveTemp(Handler));
}

bool FActionRegistry::IsSupported(EActionID ActionID) const
{
    return Handlers.Contains(ActionID);
}

FActionResult FActionRegistry::Execute(const FActionRequest& Request) const
{
    FActionResult Result;

    // 1. Проверка уверенности — раньше поиска обработчика
    if (Request.Confidence < LowConfidenceThreshold)
    {
        Result.bSuccess       = false;
        Result.FailReason     = EActionFailReason::LowConfidence;
        Result.DiagnosticNote = FString::Printf(
            TEXT("Confidence %.2f ниже порога %.2f"),
            Request.Confidence, LowConfidenceThreshold);
        return Result;
    }

    // 2. Поиск обработчика
    const FActionHandler* Handler = Handlers.Find(Request.ActionID);
    if (!Handler)
    {
        Result.bSuccess       = false;
        Result.FailReason     = EActionFailReason::NotSupported;
        Result.DiagnosticNote = FString::Printf(
            TEXT("Действие %d не зарегистрировано в реестре"),
            static_cast<int32>(Request.ActionID));
        return Result;
    }

    // 3. Выполнение
    Result = (*Handler)(Request);
    return Result;
}
