#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

#include "FOfflineVoiceAdapters.h"
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVoiceSmoke_RuntimeTogglePreservesTextPipelineInvariant,
    "Neira.Voice.Smoke.RuntimeTogglePreservesTextPipelineInvariant",
    NEIRA_TEST_FLAGS)
bool FVoiceSmoke_RuntimeTogglePreservesTextPipelineInvariant::RunTest(const FString& Parameters)
{
    FMockSpeechToText Asr;
    Asr.NextResult.Status = EAsrStatus::Success;
    Asr.NextResult.Transcript = TEXT("voice in");

    FMockTextToSpeech Tts;
    Tts.NextResult.Status = ETtsStatus::Success;

    FVoiceFeatureFlags Flags; // default: voice disabled
    FVoiceSessionOrchestrator Orchestrator(
        Flags,
        [](const FString& Input) -> FString { return FString(TEXT("text::")) + Input; },
        &Asr,
        &Tts);

    FVoiceTurnRequest Request;
    Request.TextInput = TEXT("typed in");
    Request.AudioInput = TEXT("pcm-bytes");

    const FVoiceTurnResult VoiceOffResult = Orchestrator.RunTurn(Request);
    TestEqual(TEXT("При voice=false используется текущий текстовый pipeline"),
              VoiceOffResult.TextResponse,
              FString(TEXT("text::typed in")));
    TestFalse(TEXT("При voice=false голосовой вход не используется"), VoiceOffResult.bUsedVoiceInput);
    TestFalse(TEXT("При voice=false голосовой выход не используется"), VoiceOffResult.bUsedVoiceOutput);

    Orchestrator.SetVoiceEnabled(true);
    const FVoiceTurnResult VoiceOnResult = Orchestrator.RunTurn(Request);
    TestEqual(TEXT("После runtime-включения используется ASR transcript"),
              VoiceOnResult.TextResponse,
              FString(TEXT("text::voice in")));
    TestTrue(TEXT("После runtime-включения voice input используется"), VoiceOnResult.bUsedVoiceInput);
    TestTrue(TEXT("После runtime-включения voice output используется"), VoiceOnResult.bUsedVoiceOutput);

    Orchestrator.SetVoiceEnabled(false);
    const FVoiceTurnResult VoiceOffAgainResult = Orchestrator.RunTurn(Request);
    TestEqual(TEXT("После повторного выключения инвариант текстового pipeline сохраняется"),
              VoiceOffAgainResult.TextResponse,
              FString(TEXT("text::typed in")));
    TestFalse(TEXT("После повторного выключения voice input не используется"), VoiceOffAgainResult.bUsedVoiceInput);
    TestFalse(TEXT("После повторного выключения voice output не используется"), VoiceOffAgainResult.bUsedVoiceOutput);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVoiceSmoke_OfflineAdaptersProvideDeterministicPath,
    "Neira.Voice.Smoke.OfflineAdaptersProvideDeterministicPath",
    NEIRA_TEST_FLAGS)
bool FVoiceSmoke_OfflineAdaptersProvideDeterministicPath::RunTest(const FString& Parameters)
{
    FOfflineSpeechToTextAdapter OfflineAsr;
    FOfflineTextToSpeechAdapter OfflineTts;

    FVoiceFeatureFlags Flags;
    Flags.bVoiceEnabled = true;

    FVoiceSessionOrchestrator Orchestrator(
        Flags,
        [](const FString& Input) -> FString { return FString(TEXT("text::")) + Input; },
        &OfflineAsr,
        &OfflineTts);

    FVoiceTurnRequest Request;
    Request.AudioInput = TEXT("локальный запрос");

    const FVoiceTurnResult Result = Orchestrator.RunTurn(Request);
    TestEqual(TEXT("Offline ASR должен отдавать transcript из payload"),
              Result.TextResponse,
              FString(TEXT("text::локальный запрос")));
    TestTrue(TEXT("Offline voice input должен быть использован"), Result.bUsedVoiceInput);
    TestTrue(TEXT("Offline voice output должен быть использован"), Result.bUsedVoiceOutput);
    TestTrue(TEXT("Offline TTS должен формировать audio payload"),
             Result.AudioResponse.Contains(TEXT("offline-audio::"), false));

    return true;
}
