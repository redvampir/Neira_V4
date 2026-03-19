#pragma once

#include "CoreMinimal.h"
#include "NeiraTypes.h"

struct FVoiceFixture
{
    FString CaseID;
    FString Category;
    FString Phrase;
    EPhraseType PhraseType = EPhraseType::Unknown;
    EIntentID ExpectedIntent = EIntentID::Unknown;
    bool bIsCommandCase = false;
    bool bRequiresFallback = false;
};

inline const TArray<FVoiceFixture>& GetVoiceFixtures()
{
    static const TArray<FVoiceFixture> Fixtures = {
        // short commands
        { TEXT("voice_cmd_short_01"), TEXT("short_commands"), TEXT("найди значение синтаксиса"), EPhraseType::Command, EIntentID::FindMeaning, true, false },
        { TEXT("voice_cmd_short_02"), TEXT("short_commands"), TEXT("найди определение памяти"), EPhraseType::Command, EIntentID::FindMeaning, true, false },
        { TEXT("voice_cmd_short_03"), TEXT("short_commands"), TEXT("объясни синтаксис"), EPhraseType::Request, EIntentID::GetWordFact, false, false },
        { TEXT("voice_cmd_short_04"), TEXT("short_commands"), TEXT("расскажи память"), EPhraseType::Request, EIntentID::GetWordFact, false, false },

        // questions
        { TEXT("voice_q_01"), TEXT("questions"), TEXT("что такое память"), EPhraseType::Question, EIntentID::GetDefinition, false, false },
        { TEXT("voice_q_02"), TEXT("questions"), TEXT("что означает синтаксис"), EPhraseType::Question, EIntentID::GetDefinition, false, false },
        { TEXT("voice_q_03"), TEXT("questions"), TEXT("что значит ошибка"), EPhraseType::Question, EIntentID::GetDefinition, false, false },
        { TEXT("voice_q_04"), TEXT("questions"), TEXT("ты можешь объяснить морфологию"), EPhraseType::Question, EIntentID::AnswerAbility, false, false },

        // conversational variants
        { TEXT("voice_conv_01"), TEXT("conversational_variants"), TEXT("пожалуйста расскажи про память"), EPhraseType::Request, EIntentID::GetWordFact, false, false },
        { TEXT("voice_conv_02"), TEXT("conversational_variants"), TEXT("алло объясни синтаксис"), EPhraseType::Request, EIntentID::GetWordFact, false, false },
        { TEXT("voice_conv_03"), TEXT("conversational_variants"), TEXT("пжлст объясни ошибку"), EPhraseType::Request, EIntentID::GetWordFact, false, false },
        { TEXT("voice_conv_04"), TEXT("conversational_variants"), TEXT("эээ что такое дата"), EPhraseType::Question, EIntentID::GetDefinition, false, false },

        // typical ASR errors
        { TEXT("voice_asr_01"), TEXT("typical_asr_errors"), TEXT("чо значит синтакез"), EPhraseType::Question, EIntentID::GetDefinition, false, false },
        { TEXT("voice_asr_02"), TEXT("typical_asr_errors"), TEXT("найтить определение марфология"), EPhraseType::Command, EIntentID::FindMeaning, true, false },
        { TEXT("voice_asr_03"), TEXT("typical_asr_errors"), TEXT("расскажи пж про паметь"), EPhraseType::Request, EIntentID::GetWordFact, false, false },
        { TEXT("voice_asr_04"), TEXT("typical_asr_errors"), TEXT("бррр вжух"), EPhraseType::Unknown, EIntentID::Unknown, false, true }
    };

    return Fixtures;
}

inline const TArray<FString>& GetVoiceLexiconGovernedWords()
{
    static const TArray<FString> Words = {
        TEXT("пожалуйста"),
        TEXT("алло"),
        TEXT("пжлст"),
        TEXT("эээ")
    };
    return Words;
}
