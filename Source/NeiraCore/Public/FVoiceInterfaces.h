#pragma once

#include "CoreMinimal.h"

/**
 * Контракты голосового контура ядра (ASR/TTS + orchestration).
 *
 * Цель: развести инфраструктурные зависимости и основной текстовый pipeline.
 * Важно: при bVoiceEnabled=false orchestrator обязан идти по текстовому пути
 * без обращения к ASR/TTS (поведение как до добавления голоса).
 */

enum class EAsrStatus : uint8
{
    Success,
    Timeout,
    EmptyTranscript,
    Failed,
};

struct NEIRACORE_API FSpeechToTextResult
{
    EAsrStatus Status = EAsrStatus::Failed;
    FString Transcript;
    FString DiagnosticNote;
};

struct NEIRACORE_API ISpeechToText
{
    virtual ~ISpeechToText() = default;
    virtual FSpeechToTextResult Transcribe(const FString& AudioPayload, int32 TimeoutMs) = 0;
};

enum class ETtsStatus : uint8
{
    Success,
    Unavailable,
    Failed,
};

struct NEIRACORE_API FTextToSpeechResult
{
    ETtsStatus Status = ETtsStatus::Failed;
    FString AudioPayload;
    FString DiagnosticNote;
};

struct NEIRACORE_API ITextToSpeech
{
    virtual ~ITextToSpeech() = default;
    virtual FTextToSpeechResult Synthesize(const FString& Text) = 0;
};

struct NEIRACORE_API FVoiceFeatureFlags
{
    bool bVoiceEnabled = false;
};

struct NEIRACORE_API FVoiceTurnRequest
{
    FString TextInput;
    FString AudioInput;
    int32 AsrTimeoutMs = 1500;
};

struct NEIRACORE_API FVoiceTurnResult
{
    FString TextResponse;
    FString AudioResponse;

    bool bUsedVoiceInput = false;
    bool bUsedVoiceOutput = false;
    bool bShouldPromptRepeat = false;
    bool bSwitchedToText = false;

    // Стабильный машинно-читаемый код сбоя оркестратора (если есть).
    FString DiagnosticCode;
    FString DiagnosticNote;
};

struct NEIRACORE_API IVoiceGateway
{
    virtual ~IVoiceGateway() = default;
    virtual void SetVoiceEnabled(bool bEnabled) = 0;
    virtual bool IsVoiceEnabled() const = 0;
    virtual FVoiceTurnResult RunTurn(const FVoiceTurnRequest& Request) = 0;
};

using IVoiceSessionOrchestrator = IVoiceGateway;
