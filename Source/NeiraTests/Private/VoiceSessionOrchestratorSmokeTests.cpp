#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

#include "FVoiceSessionOrchestrator.h"

#define NEIRA_TEST_FLAGS (EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

namespace
{
struct FMockSpeechToText final : public ISpeechToText
{
    FSpeechToTextResult NextResult;

    FSpeechToTextResult Transcribe(const FString& AudioPayload, int32 TimeoutMs) override
    {
        (void)AudioPayload;
        (void)TimeoutMs;
        return NextResult;
    }
};

struct FMockTextToSpeech final : public ITextToSpeech
{
    FTextToSpeechResult NextResult;

    FTextToSpeechResult Synthesize(const FString& Text) override
    {
        if (NextResult.Status == ETtsStatus::Success && NextResult.AudioPayload.IsEmpty())
        {
            FTextToSpeechResult Dynamic = NextResult;
            Dynamic.AudioPayload = FString(TEXT("audio::")) + Text;
            return Dynamic;
        }
        return NextResult;
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVoiceSmoke_NormalVoicePass,
    "Neira.Voice.Smoke.NormalVoicePass",
    NEIRA_TEST_FLAGS)
bool FVoiceSmoke_NormalVoicePass::RunTest(const FString& Parameters)
{
    FMockSpeechToText Asr;
    Asr.NextResult.Status = EAsrStatus::Success;
    Asr.NextResult.Transcript = TEXT("найди значение слова синтаксис");

    FMockTextToSpeech Tts;
    Tts.NextResult.Status = ETtsStatus::Success;

    FVoiceFeatureFlags Flags;
    Flags.bVoiceEnabled = true;

    FVoiceSessionOrchestrator Orchestrator(
        Flags,
        [](const FString& Input) -> FString { return FString(TEXT("text::")) + Input; },
        &Asr,
        &Tts);

    FVoiceTurnRequest Request;
    Request.AudioInput = TEXT("pcm-bytes");

    const FVoiceTurnResult Result = Orchestrator.RunTurn(Request);
    TestEqual(TEXT("Текстовый ответ должен быть собран из transcript"),
              Result.TextResponse,
              FString(TEXT("text::найди значение слова синтаксис")));
    TestTrue(TEXT("ASR путь должен быть использован"), Result.bUsedVoiceInput);
    TestTrue(TEXT("TTS путь должен быть использован"), Result.bUsedVoiceOutput);
    TestTrue(TEXT("Audio response должен быть сформирован"), !Result.AudioResponse.IsEmpty());

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVoiceSmoke_AsrEmptyPromptRepeat,
    "Neira.Voice.Smoke.AsrEmptyPromptRepeat",
    NEIRA_TEST_FLAGS)
bool FVoiceSmoke_AsrEmptyPromptRepeat::RunTest(const FString& Parameters)
{
    FMockSpeechToText Asr;
    Asr.NextResult.Status = EAsrStatus::EmptyTranscript;
    Asr.NextResult.DiagnosticNote = TEXT("empty transcript");

    FVoiceFeatureFlags Flags;
    Flags.bVoiceEnabled = true;

    FVoiceSessionOrchestrator Orchestrator(
        Flags,
        [](const FString& Input) -> FString { return FString(TEXT("text::")) + Input; },
        &Asr,
        nullptr);

    FVoiceTurnRequest Request;
    Request.AudioInput = TEXT("pcm-bytes");
    Request.TextInput = TEXT("");

    const FVoiceTurnResult Result = Orchestrator.RunTurn(Request);
    TestTrue(TEXT("При пустом ASR нужно просить повтор"), Result.bShouldPromptRepeat);
    TestTrue(TEXT("При пустом ASR нужно переключиться на текст"), Result.bSwitchedToText);
    TestTrue(TEXT("Ответ должен содержать просьбу повторить"), Result.TextResponse.Contains(TEXT("Повторите"), false));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVoiceSmoke_TtsUnavailableFallbackToText,
    "Neira.Voice.Smoke.TtsUnavailableFallbackToText",
    NEIRA_TEST_FLAGS)
bool FVoiceSmoke_TtsUnavailableFallbackToText::RunTest(const FString& Parameters)
{
    FMockSpeechToText Asr;
    Asr.NextResult.Status = EAsrStatus::Success;
    Asr.NextResult.Transcript = TEXT("что такое память");

    FMockTextToSpeech Tts;
    Tts.NextResult.Status = ETtsStatus::Unavailable;
    Tts.NextResult.DiagnosticNote = TEXT("tts unavailable");

    FVoiceFeatureFlags Flags;
    Flags.bVoiceEnabled = true;

    FVoiceSessionOrchestrator Orchestrator(
        Flags,
        [](const FString& Input) -> FString { return FString(TEXT("text::")) + Input; },
        &Asr,
        &Tts);

    FVoiceTurnRequest Request;
    Request.AudioInput = TEXT("pcm-bytes");

    const FVoiceTurnResult Result = Orchestrator.RunTurn(Request);
    TestEqual(TEXT("При недоступном TTS текстовый ответ сохраняется"),
              Result.TextResponse,
              FString(TEXT("text::что такое память")));
    TestFalse(TEXT("TTS output не должен быть активен"), Result.bUsedVoiceOutput);
    TestTrue(TEXT("Диагностика должна содержать причину TTS"), Result.DiagnosticNote.Contains(TEXT("tts unavailable"), false));

    return true;
}
