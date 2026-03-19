#pragma once

#include "CoreMinimal.h"
#include "FVoiceInterfaces.h"

/**
 * Offline-адаптер ASR: принимает заранее сериализованный текст в AudioPayload
 * для локальных сценариев и smoke-тестов без внешних зависимостей.
 */
struct NEIRACORE_API FOfflineSpeechToTextAdapter final : public ISpeechToText
{
    FSpeechToTextResult Transcribe(const FString& AudioPayload, int32 TimeoutMs) override;
};

/**
 * Offline-адаптер TTS: синтезирует "аудио" в виде префиксированной строки.
 * Нужен для инварианта "voice optional" без интеграции с облачными сервисами.
 */
struct NEIRACORE_API FOfflineTextToSpeechAdapter final : public ITextToSpeech
{
    FTextToSpeechResult Synthesize(const FString& Text) override;
};
