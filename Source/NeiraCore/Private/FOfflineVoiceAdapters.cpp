#include "FOfflineVoiceAdapters.h"

FSpeechToTextResult FOfflineSpeechToTextAdapter::Transcribe(const FString& AudioPayload, int32 TimeoutMs)
{
    (void)TimeoutMs;

    FSpeechToTextResult Result;
    const FString TrimmedPayload = AudioPayload.TrimStartAndEnd();

    if (TrimmedPayload.IsEmpty())
    {
        Result.Status = EAsrStatus::EmptyTranscript;
        Result.DiagnosticNote = TEXT("offline-asr: empty audio payload");
        return Result;
    }

    Result.Status = EAsrStatus::Success;
    Result.Transcript = TrimmedPayload;
    Result.DiagnosticNote = TEXT("offline-asr: transcript restored from payload");
    return Result;
}

FTextToSpeechResult FOfflineTextToSpeechAdapter::Synthesize(const FString& Text)
{
    FTextToSpeechResult Result;
    const FString TrimmedText = Text.TrimStartAndEnd();

    if (TrimmedText.IsEmpty())
    {
        Result.Status = ETtsStatus::Unavailable;
        Result.DiagnosticNote = TEXT("offline-tts: empty text");
        return Result;
    }

    Result.Status = ETtsStatus::Success;
    Result.AudioPayload = FString(TEXT("offline-audio::")) + TrimmedText;
    Result.DiagnosticNote = TEXT("offline-tts: synthesized from text");
    return Result;
}
