#include "FVoiceSessionOrchestrator.h"

namespace
{
static constexpr int32 MinAudioPayloadLenForVoice = 4;

void SetDiagnostic(FVoiceTurnResult& Result, const FString& Code, const FString& Note)
{
    Result.DiagnosticCode = Code;
    Result.DiagnosticNote = Note;
}
}

FVoiceSessionOrchestrator::FVoiceSessionOrchestrator(const FVoiceFeatureFlags& InFlags,
                                                     FTextPipelineHandler      InTextHandler,
                                                     ISpeechToText*            InSpeechToText,
                                                     ITextToSpeech*            InTextToSpeech)
    : Flags(InFlags)
    , TextHandler(MoveTemp(InTextHandler))
    , SpeechToText(InSpeechToText)
    , TextToSpeech(InTextToSpeech)
{
}

void FVoiceSessionOrchestrator::SetVoiceEnabled(bool bEnabled)
{
    Flags.bVoiceEnabled = bEnabled;
}

bool FVoiceSessionOrchestrator::IsVoiceEnabled() const
{
    return Flags.bVoiceEnabled;
}

FVoiceTurnResult FVoiceSessionOrchestrator::RunTurn(const FVoiceTurnRequest& Request)
{
    FVoiceTurnResult Result;

    if (!TextHandler)
    {
        Result.TextResponse = TEXT("Ошибка: текстовый pipeline недоступен.");
        SetDiagnostic(Result, TEXT("FVoiceSessionOrchestrator:TEXT_PIPELINE_UNAVAILABLE"), TEXT("TextHandler is not bound"));
        return Result;
    }

    // Feature-flag OFF: оставляем legacy-поведение (только текст).
    if (!Flags.bVoiceEnabled)
    {
        Result.TextResponse = TextHandler(Request.TextInput);
        return Result;
    }

    // Step 1: VAD (silence / interrupted audio check).
    FString EffectiveText = Request.TextInput;
    const FString TrimmedAudio = Request.AudioInput.TrimStartAndEnd();
    const bool bHasAudioPayload = !TrimmedAudio.IsEmpty();

    if (bHasAudioPayload && TrimmedAudio.Len() < MinAudioPayloadLenForVoice)
    {
        Result.bShouldPromptRepeat = true;
        Result.bSwitchedToText = true;
        SetDiagnostic(Result,
                      TEXT("FVoiceSessionOrchestrator:VAD_AUDIO_INTERRUPTED"),
                      TEXT("Audio payload is too short and treated as interrupted"));
        if (EffectiveText.TrimStartAndEnd().IsEmpty())
        {
            Result.TextResponse = TEXT("Обрыв аудио. Повторите голосом или введите текст.");
            return Result;
        }
    }

    // Step 2: ASR.
    if (bHasAudioPayload && TrimmedAudio.Len() >= MinAudioPayloadLenForVoice && SpeechToText)
    {
        const FSpeechToTextResult AsrResult = SpeechToText->Transcribe(Request.AudioInput, Request.AsrTimeoutMs);

        if (AsrResult.Status == EAsrStatus::Success && !AsrResult.Transcript.TrimStartAndEnd().IsEmpty())
        {
            EffectiveText = AsrResult.Transcript;
            Result.bUsedVoiceInput = true;
        }
        else if (AsrResult.Status == EAsrStatus::Timeout || AsrResult.Status == EAsrStatus::EmptyTranscript)
        {
            Result.bShouldPromptRepeat = true;
            Result.bSwitchedToText = true;
            if (AsrResult.Status == EAsrStatus::Timeout)
            {
                SetDiagnostic(Result, TEXT("FVoiceSessionOrchestrator:ASR_TIMEOUT"), AsrResult.DiagnosticNote);
            }
            else
            {
                SetDiagnostic(Result, TEXT("FVoiceSessionOrchestrator:ASR_EMPTY_TRANSCRIPT"), AsrResult.DiagnosticNote);
            }

            if (EffectiveText.TrimStartAndEnd().IsEmpty())
            {
                Result.TextResponse = TEXT("Не удалось распознать речь. Повторите голосом или введите текст.");
                return Result;
            }
        }
        else
        {
            Result.bSwitchedToText = true;
            SetDiagnostic(Result, TEXT("FVoiceSessionOrchestrator:ASR_FAILED"), AsrResult.DiagnosticNote);
        }
    }
    else if (!bHasAudioPayload && EffectiveText.TrimStartAndEnd().IsEmpty())
    {
        Result.bShouldPromptRepeat = true;
        Result.bSwitchedToText = true;
        SetDiagnostic(Result,
                      TEXT("FVoiceSessionOrchestrator:VAD_SILENCE"),
                      TEXT("No audio payload and no text input"));
        Result.TextResponse = TEXT("Тишина на входе. Скажите фразу ещё раз или введите текст.");
        return Result;
    }

    // Step 3->4: Intent + Action hidden behind text pipeline handler.
    if (EffectiveText.TrimStartAndEnd().IsEmpty())
    {
        Result.bShouldPromptRepeat = true;
        Result.bSwitchedToText = true;
        SetDiagnostic(Result,
                      TEXT("FVoiceSessionOrchestrator:EMPTY_EFFECTIVE_TEXT"),
                      TEXT("Effective text is empty after VAD/ASR processing"));
        Result.TextResponse = TEXT("Не удалось получить текст запроса. Повторите голосом или введите текст.");
        return Result;
    }

    Result.TextResponse = TextHandler(EffectiveText);

    // Step 5: TTS.
    if (TextToSpeech && !Result.TextResponse.TrimStartAndEnd().IsEmpty())
    {
        const FTextToSpeechResult TtsResult = TextToSpeech->Synthesize(Result.TextResponse);
        if (TtsResult.Status == ETtsStatus::Success)
        {
            Result.AudioResponse = TtsResult.AudioPayload;
            Result.bUsedVoiceOutput = true;
        }
        else
        {
            const FString ExistingCode = Result.DiagnosticCode;
            const FString ExistingNote = Result.DiagnosticNote;
            SetDiagnostic(Result,
                          TEXT("FVoiceSessionOrchestrator:TTS_FAILED"),
                          TtsResult.DiagnosticNote);
            if (!ExistingCode.IsEmpty())
            {
                Result.DiagnosticCode = ExistingCode + TEXT(";") + Result.DiagnosticCode;
            }
            if (!ExistingNote.IsEmpty() && !TtsResult.DiagnosticNote.IsEmpty())
            {
                Result.DiagnosticNote = ExistingNote + TEXT("; ") + TtsResult.DiagnosticNote;
            }
            else if (!ExistingNote.IsEmpty())
            {
                Result.DiagnosticNote = ExistingNote;
            }
            // Явный fail-path: деградация до текста без потери контекста.
            Result.bUsedVoiceOutput = false;
        }
    }

    return Result;
}
