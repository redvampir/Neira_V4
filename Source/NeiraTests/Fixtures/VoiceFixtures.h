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
        { TEXT("voice_asr_04"), TEXT("typical_asr_errors"), TEXT("бррр вжух"), EPhraseType::Unknown, EIntentID::Unknown, false, true },

        // new clean commands (20)
        { TEXT("voice_cmd_clean_01"), TEXT("short_commands"), TEXT("найди значение морфологии"), EPhraseType::Command, EIntentID::FindMeaning, true, false },
        { TEXT("voice_cmd_clean_02"), TEXT("short_commands"), TEXT("найди определение синтаксиса"), EPhraseType::Command, EIntentID::FindMeaning, true, false },
        { TEXT("voice_cmd_clean_03"), TEXT("short_commands"), TEXT("найди значение ошибки"), EPhraseType::Command, EIntentID::FindMeaning, true, false },
        { TEXT("voice_cmd_clean_04"), TEXT("short_commands"), TEXT("найди определение отчёта"), EPhraseType::Command, EIntentID::FindMeaning, true, false },
        { TEXT("voice_cmd_clean_05"), TEXT("short_commands"), TEXT("найди значение файла"), EPhraseType::Command, EIntentID::FindMeaning, true, false },
        { TEXT("voice_cmd_clean_06"), TEXT("short_commands"), TEXT("найди определение документа"), EPhraseType::Command, EIntentID::FindMeaning, true, false },
        { TEXT("voice_cmd_clean_07"), TEXT("short_commands"), TEXT("найди значение памяти"), EPhraseType::Command, EIntentID::FindMeaning, true, false },
        { TEXT("voice_cmd_clean_08"), TEXT("short_commands"), TEXT("найди определение даты"), EPhraseType::Command, EIntentID::FindMeaning, true, false },
        { TEXT("voice_cmd_clean_09"), TEXT("short_commands"), TEXT("найди значение времени"), EPhraseType::Command, EIntentID::FindMeaning, true, false },
        { TEXT("voice_cmd_clean_10"), TEXT("short_commands"), TEXT("найди определение лога"), EPhraseType::Command, EIntentID::FindMeaning, true, false },
        { TEXT("voice_cmd_clean_11"), TEXT("short_commands"), TEXT("найди значение текста"), EPhraseType::Command, EIntentID::FindMeaning, true, false },
        { TEXT("voice_cmd_clean_12"), TEXT("short_commands"), TEXT("найди определение окна"), EPhraseType::Command, EIntentID::FindMeaning, true, false },
        { TEXT("voice_cmd_clean_13"), TEXT("short_commands"), TEXT("найди значение слова синтаксис"), EPhraseType::Command, EIntentID::FindMeaning, true, false },
        { TEXT("voice_cmd_clean_14"), TEXT("short_commands"), TEXT("найди определение термина память"), EPhraseType::Command, EIntentID::FindMeaning, true, false },
        { TEXT("voice_cmd_clean_15"), TEXT("short_commands"), TEXT("найди значение слова ошибка"), EPhraseType::Command, EIntentID::FindMeaning, true, false },
        { TEXT("voice_cmd_clean_16"), TEXT("short_commands"), TEXT("найди определение термина морфология"), EPhraseType::Command, EIntentID::FindMeaning, true, false },
        { TEXT("voice_cmd_clean_17"), TEXT("short_commands"), TEXT("найди значение отчета"), EPhraseType::Command, EIntentID::FindMeaning, true, false },
        { TEXT("voice_cmd_clean_18"), TEXT("short_commands"), TEXT("найди определение файла"), EPhraseType::Command, EIntentID::FindMeaning, true, false },
        { TEXT("voice_cmd_clean_19"), TEXT("short_commands"), TEXT("найди значение документа"), EPhraseType::Command, EIntentID::FindMeaning, true, false },
        { TEXT("voice_cmd_clean_20"), TEXT("short_commands"), TEXT("найди определение времени"), EPhraseType::Command, EIntentID::FindMeaning, true, false },

        // new conversational forms (10)
        { TEXT("voice_conv_new_01"), TEXT("conversational_variants"), TEXT("пожалуйста расскажи про синтаксис"), EPhraseType::Request, EIntentID::GetWordFact, false, false },
        { TEXT("voice_conv_new_02"), TEXT("conversational_variants"), TEXT("алло расскажи про морфологию"), EPhraseType::Request, EIntentID::GetWordFact, false, false },
        { TEXT("voice_conv_new_03"), TEXT("conversational_variants"), TEXT("пжлст расскажи про ошибку"), EPhraseType::Request, EIntentID::GetWordFact, false, false },
        { TEXT("voice_conv_new_04"), TEXT("conversational_variants"), TEXT("эээ расскажи про дату"), EPhraseType::Request, EIntentID::GetWordFact, false, false },
        { TEXT("voice_conv_new_05"), TEXT("conversational_variants"), TEXT("пожалуйста объясни память"), EPhraseType::Request, EIntentID::GetWordFact, false, false },
        { TEXT("voice_conv_new_06"), TEXT("conversational_variants"), TEXT("алло объясни ошибку"), EPhraseType::Request, EIntentID::GetWordFact, false, false },
        { TEXT("voice_conv_new_07"), TEXT("conversational_variants"), TEXT("пжлст что такое синтаксис"), EPhraseType::Question, EIntentID::GetDefinition, false, false },
        { TEXT("voice_conv_new_08"), TEXT("conversational_variants"), TEXT("эээ что означает память"), EPhraseType::Question, EIntentID::GetDefinition, false, false },
        { TEXT("voice_conv_new_09"), TEXT("conversational_variants"), TEXT("пожалуйста ты можешь объяснить дату"), EPhraseType::Question, EIntentID::AnswerAbility, false, false },
        { TEXT("voice_conv_new_10"), TEXT("conversational_variants"), TEXT("алло ты можешь рассказать про время"), EPhraseType::Question, EIntentID::AnswerAbility, false, false },

        // new ASR error cases (10)
        { TEXT("voice_asr_new_01"), TEXT("typical_asr_errors"), TEXT("чо означает синтакез"), EPhraseType::Question, EIntentID::GetDefinition, false, false },
        { TEXT("voice_asr_new_02"), TEXT("typical_asr_errors"), TEXT("че значит марфология"), EPhraseType::Question, EIntentID::GetDefinition, false, false },
        { TEXT("voice_asr_new_03"), TEXT("typical_asr_errors"), TEXT("найтить значение паметь"), EPhraseType::Command, EIntentID::FindMeaning, true, false },
        { TEXT("voice_asr_new_04"), TEXT("typical_asr_errors"), TEXT("найтить определение синтакез"), EPhraseType::Command, EIntentID::FindMeaning, true, false },
        { TEXT("voice_asr_new_05"), TEXT("typical_asr_errors"), TEXT("аткрой фаил"), EPhraseType::Command, EIntentID::Unknown, true, true },
        { TEXT("voice_asr_new_06"), TEXT("typical_asr_errors"), TEXT("акрой акно"), EPhraseType::Command, EIntentID::Unknown, true, true },
        { TEXT("voice_asr_new_07"), TEXT("typical_asr_errors"), TEXT("расскажи плиз про синтакез"), EPhraseType::Request, EIntentID::GetWordFact, false, false },
        { TEXT("voice_asr_new_08"), TEXT("typical_asr_errors"), TEXT("объясни пж марфология"), EPhraseType::Request, EIntentID::GetWordFact, false, false },
        { TEXT("voice_asr_new_09"), TEXT("typical_asr_errors"), TEXT("чо такое паметь"), EPhraseType::Question, EIntentID::GetDefinition, false, false },
        { TEXT("voice_asr_new_10"), TEXT("typical_asr_errors"), TEXT("кароч объясни синтакез"), EPhraseType::Request, EIntentID::GetWordFact, false, false }
    };

    return Fixtures;
}

inline const TArray<FString>& GetVoiceLexiconGovernedWords()
{
    static const TArray<FString> Words = {
        TEXT("пожалуйста"),
        TEXT("алло"),
        TEXT("пжлст"),
        TEXT("эээ"),
        TEXT("чо"),
        TEXT("че"),
        TEXT("найтить"),
        TEXT("синтакез"),
        TEXT("марфология"),
        TEXT("паметь"),
        TEXT("аткрой"),
        TEXT("акрой"),
        TEXT("фаил"),
        TEXT("акно"),
        TEXT("плиз"),
        TEXT("кароч")
    };
    return Words;
}


inline const TArray<FString>& GetVoiceDictionaryGrowthGuardWords()
{
    static const TArray<FString> Words = {
        TEXT("чо"),
        TEXT("че"),
        TEXT("найтить"),
        TEXT("синтакез"),
        TEXT("марфология"),
        TEXT("паметь"),
        TEXT("аткрой"),
        TEXT("акрой"),
        TEXT("фаил"),
        TEXT("акно"),
        TEXT("плиз"),
        TEXT("кароч")
    };
    return Words;
}
