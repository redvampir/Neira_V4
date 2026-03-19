#pragma once

#include "CoreMinimal.h"
#include "FVoiceInterfaces.h"

using FTextPipelineHandler = TFunction<FString(const FString&)>;

/**
 * Оркестратор с feature-flag для включения голосового пути.
 *
 * Контракт:
 * - bVoiceEnabled=false: только текстовый путь, как в legacy-режиме.
 * - ASR timeout/empty: явный fail-path (prompt repeat + switch to text).
 * - TTS failure: текстовый ответ возвращается всегда, контекст не теряется.
 */
struct NEIRACORE_API FVoiceSessionOrchestrator : public IVoiceGateway
{
    FVoiceSessionOrchestrator(const FVoiceFeatureFlags& InFlags,
                              FTextPipelineHandler      InTextHandler,
                              ISpeechToText*            InSpeechToText,
                              ITextToSpeech*            InTextToSpeech);

    void SetVoiceEnabled(bool bEnabled) override;
    bool IsVoiceEnabled() const override;
    FVoiceTurnResult RunTurn(const FVoiceTurnRequest& Request) override;

private:
    FVoiceFeatureFlags Flags;
    FTextPipelineHandler TextHandler;
    ISpeechToText* SpeechToText = nullptr;
    ITextToSpeech* TextToSpeech = nullptr;
};
