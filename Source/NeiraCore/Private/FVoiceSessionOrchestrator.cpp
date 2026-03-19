#include "FVoiceSessionOrchestrator.h"

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

FVoiceTurnResult FVoiceSessionOrchestrator::RunTurn(const FVoiceTurnRequest& Request)
{
    FVoiceTurnResult Result;

    if (!TextHandler)
    {
        Result.TextResponse = TEXT("Ошибка: текстовый pipeline недоступен.");
        Result.DiagnosticNote = TEXT("TextHandler is not bound");
        return Result;
    }

    // Feature-flag OFF: оставляем legacy-поведение (только текст).
    if (!Flags.bVoiceEnabled)
    {
        Result.TextResponse = TextHandler(Request.TextInput);
        return Result;
    }

    FString EffectiveText = Request.TextInput;

    if (!Request.AudioInput.TrimStartAndEnd().IsEmpty() && SpeechToText)
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
            Result.DiagnosticNote = AsrResult.DiagnosticNote;

            if (EffectiveText.TrimStartAndEnd().IsEmpty())
            {
                Result.TextResponse = TEXT("Не удалось распознать речь. Повторите голосом или введите текст.");
                return Result;
            }
        }
        else
        {
            Result.bSwitchedToText = true;
            if (Result.DiagnosticNote.IsEmpty())
            {
                Result.DiagnosticNote = AsrResult.DiagnosticNote;
            }
        }
    }

    Result.TextResponse = TextHandler(EffectiveText);

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
            // Явный fail-path: деградация до текста без потери контекста.
            if (!Result.DiagnosticNote.IsEmpty() && !TtsResult.DiagnosticNote.IsEmpty())
            {
                Result.DiagnosticNote += TEXT("; ");
            }
            Result.DiagnosticNote += TtsResult.DiagnosticNote;
            Result.bUsedVoiceOutput = false;
        }
    }

    return Result;
}
