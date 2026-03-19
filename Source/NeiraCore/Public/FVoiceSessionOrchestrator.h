#pragma once

#include "CoreMinimal.h"
#include "FVoiceInterfaces.h"

using FTextPipelineHandler = TFunction<FString(const FString&)>;

/**
 * Оркестратор с feature-flag для включения голосового пути.
 *
 * Контракт:
 * - bVoiceEnabled=false: только текстовый путь, как в legacy-режиме.
 * - Голосовой путь: VAD -> ASR -> Intent -> Action -> TTS.
 *   (Intent/Action выполняются внутри TextHandler как шаги текстового pipeline).
 * - На сбоях возвращается DiagnosticCode + fallback в текст.
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
