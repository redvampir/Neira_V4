#pragma once

#include "CoreMinimal.h"
#include "NeiraTypes.h"

struct FRegressionFixture
{
    FString CaseID;
    FString Category;
    FString Phrase;
    EPhraseType PhraseType = EPhraseType::Unknown;

    EIntentID BaselineIntent = EIntentID::Unknown;
    FString BaselineSubject;
    FString BaselinePredicate;
    FString BaselineObject;
    bool BaselineAbilityCheck = false;
    bool BaselineNestedClause = false;
    bool BaselineNegation = false;
};

inline const TArray<FRegressionFixture>& GetRUENRegressionFixtures()
{
    static const TArray<FRegressionFixture> Fixtures = {
        // ability check
        { TEXT("ru_ability_01"), TEXT("ability_check"), TEXT("ты можешь объяснять слова?"), EPhraseType::Question,
          EIntentID::AnswerAbility, TEXT("ты"), TEXT("мочь"), TEXT("слово"), true, false, false },
        { TEXT("ru_ability_02"), TEXT("ability_check"), TEXT("вы можете рассказать про синтаксис?"), EPhraseType::Question,
          EIntentID::AnswerAbility, TEXT("вы"), TEXT("мочь"), TEXT("синтаксис"), true, false, false },

        // nested clauses
        { TEXT("ru_nested_01"), TEXT("nested_clauses"), TEXT("расскажи что такое морфология"), EPhraseType::Request,
          EIntentID::GetWordFact, TEXT(""), TEXT("рассказать"), TEXT("морфология"), false, true, false },
        { TEXT("ru_nested_02"), TEXT("nested_clauses"), TEXT("скажи если кот не спит"), EPhraseType::Request,
          EIntentID::GetWordFact, TEXT(""), TEXT("сказать"), TEXT("кот"), false, true, true },

        // negation
        { TEXT("ru_negation_01"), TEXT("negation"), TEXT("не открой окно"), EPhraseType::Command,
          EIntentID::Unknown, TEXT(""), TEXT("открыть"), TEXT("окно"), false, false, true },
        { TEXT("ru_negation_02"), TEXT("negation"), TEXT("нельзя найти определение"), EPhraseType::Command,
          EIntentID::Unknown, TEXT(""), TEXT("найти"), TEXT("определение"), false, false, true },

        // ambiguous tokens
        { TEXT("ru_ambiguous_01"), TEXT("ambiguous_tokens"), TEXT("расскажи что такое синтаксис"), EPhraseType::Request,
          EIntentID::GetWordFact, TEXT(""), TEXT("рассказать"), TEXT("синтаксис"), false, true, false },
        { TEXT("ru_ambiguous_02"), TEXT("ambiguous_tokens"), TEXT("объясни как работает память"), EPhraseType::Request,
          EIntentID::GetWordFact, TEXT(""), TEXT("объяснить"), TEXT("память"), false, false, false },

        // domain package: action_commands (lemma + frequent forms + boundary)
        { TEXT("ru_action_lemma_01"), TEXT("action_commands"), TEXT("проверь окно"), EPhraseType::Command,
          EIntentID::Unknown, TEXT(""), TEXT("проверить"), TEXT("окно"), false, false, false },
        { TEXT("ru_action_freq_01"), TEXT("action_commands"), TEXT("можете открыть окно"), EPhraseType::Question,
          EIntentID::GetDefinition, TEXT(""), TEXT("мочь"), TEXT("окно"), false, false, false },
        { TEXT("ru_action_boundary_01"), TEXT("action_commands"), TEXT("как открыть окно"), EPhraseType::Question,
          EIntentID::GetDefinition, TEXT(""), TEXT("открыть"), TEXT("окно"), false, false, false },

        // domain package: text_diagnostics (lemma + frequent forms + boundary)
        { TEXT("ru_diag_lemma_01"), TEXT("text_diagnostics"), TEXT("объясни синтаксис"), EPhraseType::Request,
          EIntentID::GetWordFact, TEXT(""), TEXT("объяснить"), TEXT("синтаксис"), false, false, false },
        { TEXT("ru_diag_freq_01"), TEXT("text_diagnostics"), TEXT("найди значение слова текста"), EPhraseType::Command,
          EIntentID::FindMeaning, TEXT(""), TEXT("найти"), TEXT("значение"), false, false, false },
        { TEXT("ru_diag_boundary_01"), TEXT("text_diagnostics"), TEXT("расскажи что такое морфология"), EPhraseType::Request,
          EIntentID::GetWordFact, TEXT(""), TEXT("рассказать"), TEXT("морфология"), false, true, false },

        // domain package: memory_knowledge (lemma + frequent forms + boundary)
        { TEXT("ru_memory_lemma_01"), TEXT("memory_knowledge"), TEXT("найди определение термина память"), EPhraseType::Command,
          EIntentID::FindMeaning, TEXT(""), TEXT("найти"), TEXT("определение"), false, false, false },
        { TEXT("ru_memory_freq_01"), TEXT("memory_knowledge"), TEXT("расскажи про память"), EPhraseType::Request,
          EIntentID::GetWordFact, TEXT(""), TEXT("рассказать"), TEXT("память"), false, false, false },
        { TEXT("ru_memory_boundary_01"), TEXT("memory_knowledge"), TEXT("что означает слово память"), EPhraseType::Question,
          EIntentID::GetDefinition, TEXT(""), TEXT("означать"), TEXT("слово"), false, true, false },

        // package: commands_actions (20 fixtures)
        { TEXT("ru_cmd_pkg_01"), TEXT("commands_actions"), TEXT("открой файл"), EPhraseType::Command,
          EIntentID::Unknown, TEXT(""), TEXT("открыть"), TEXT("файл"), false, false, false },
        { TEXT("ru_cmd_pkg_02"), TEXT("commands_actions"), TEXT("закрой папку"), EPhraseType::Command,
          EIntentID::Unknown, TEXT(""), TEXT("закрыть"), TEXT("папка"), false, false, false },
        { TEXT("ru_cmd_pkg_03"), TEXT("commands_actions"), TEXT("проверь документ"), EPhraseType::Command,
          EIntentID::Unknown, TEXT(""), TEXT("проверить"), TEXT("документ"), false, false, false },
        { TEXT("ru_cmd_pkg_04"), TEXT("commands_actions"), TEXT("запомни задачу"), EPhraseType::Command,
          EIntentID::Unknown, TEXT(""), TEXT("запомнить"), TEXT("задача"), false, false, false },
        { TEXT("ru_cmd_pkg_05"), TEXT("commands_actions"), TEXT("обнови список"), EPhraseType::Command,
          EIntentID::Unknown, TEXT(""), TEXT("обновить"), TEXT("список"), false, false, false },
        { TEXT("ru_cmd_pkg_06"), TEXT("commands_actions"), TEXT("создай отчёт"), EPhraseType::Command,
          EIntentID::Unknown, TEXT(""), TEXT("создать"), TEXT("отчёт"), false, false, false },
        { TEXT("ru_cmd_pkg_07"), TEXT("commands_actions"), TEXT("удали файл"), EPhraseType::Command,
          EIntentID::Unknown, TEXT(""), TEXT("удалить"), TEXT("файл"), false, false, false },
        { TEXT("ru_cmd_pkg_08"), TEXT("commands_actions"), TEXT("сохрани документ"), EPhraseType::Command,
          EIntentID::Unknown, TEXT(""), TEXT("сохранить"), TEXT("документ"), false, false, false },
        { TEXT("ru_cmd_pkg_09"), TEXT("commands_actions"), TEXT("загрузи логи"), EPhraseType::Command,
          EIntentID::Unknown, TEXT(""), TEXT("загрузить"), TEXT("лог"), false, false, false },
        { TEXT("ru_cmd_pkg_10"), TEXT("commands_actions"), TEXT("скачай файл"), EPhraseType::Command,
          EIntentID::Unknown, TEXT(""), TEXT("скачать"), TEXT("файл"), false, false, false },
        { TEXT("ru_cmd_pkg_11"), TEXT("commands_actions"), TEXT("переименуй папку"), EPhraseType::Command,
          EIntentID::Unknown, TEXT(""), TEXT("переименовать"), TEXT("папка"), false, false, false },
        { TEXT("ru_cmd_pkg_12"), TEXT("commands_actions"), TEXT("очисти лог"), EPhraseType::Command,
          EIntentID::Unknown, TEXT(""), TEXT("очистить"), TEXT("лог"), false, false, false },
        { TEXT("ru_cmd_pkg_13"), TEXT("commands_actions"), TEXT("скопируй файл"), EPhraseType::Command,
          EIntentID::Unknown, TEXT(""), TEXT("скопировать"), TEXT("файл"), false, false, false },
        { TEXT("ru_cmd_pkg_14"), TEXT("commands_actions"), TEXT("вставь документ"), EPhraseType::Command,
          EIntentID::Unknown, TEXT(""), TEXT("вставить"), TEXT("документ"), false, false, false },
        { TEXT("ru_cmd_pkg_15"), TEXT("commands_actions"), TEXT("выдели текст"), EPhraseType::Command,
          EIntentID::Unknown, TEXT(""), TEXT("выделить"), TEXT("текст"), false, false, false },
        { TEXT("ru_cmd_pkg_16"), TEXT("commands_actions"), TEXT("отмени задачу"), EPhraseType::Command,
          EIntentID::Unknown, TEXT(""), TEXT("отменить"), TEXT("задача"), false, false, false },
        { TEXT("ru_cmd_pkg_17"), TEXT("commands_actions"), TEXT("повтори текст"), EPhraseType::Command,
          EIntentID::Unknown, TEXT(""), TEXT("повторить"), TEXT("текст"), false, false, false },
        { TEXT("ru_cmd_pkg_18"), TEXT("commands_actions"), TEXT("исправь ошибку"), EPhraseType::Command,
          EIntentID::Unknown, TEXT(""), TEXT("исправить"), TEXT("ошибка"), false, false, false },
        { TEXT("ru_cmd_pkg_19"), TEXT("commands_actions"), TEXT("подтверди задачу"), EPhraseType::Command,
          EIntentID::Unknown, TEXT(""), TEXT("подтвердить"), TEXT("задача"), false, false, false },
        { TEXT("ru_cmd_pkg_20"), TEXT("commands_actions"), TEXT("найди значение файла"), EPhraseType::Command,
          EIntentID::FindMeaning, TEXT(""), TEXT("найти"), TEXT("значение"), false, false, false },

        // package: household_dialog (20 fixtures)
        { TEXT("ru_dialog_pkg_01"), TEXT("household_dialog"), TEXT("что такое время"), EPhraseType::Question,
          EIntentID::GetDefinition, TEXT(""), TEXT(""), TEXT("время"), false, true, false },
        { TEXT("ru_dialog_pkg_02"), TEXT("household_dialog"), TEXT("что такое дата"), EPhraseType::Question,
          EIntentID::GetDefinition, TEXT(""), TEXT(""), TEXT("дата"), false, true, false },
        { TEXT("ru_dialog_pkg_03"), TEXT("household_dialog"), TEXT("что значит время"), EPhraseType::Question,
          EIntentID::GetDefinition, TEXT(""), TEXT("значить"), TEXT("время"), false, true, false },
        { TEXT("ru_dialog_pkg_04"), TEXT("household_dialog"), TEXT("что означает дата"), EPhraseType::Question,
          EIntentID::GetDefinition, TEXT(""), TEXT("означать"), TEXT("дата"), false, true, false },
        { TEXT("ru_dialog_pkg_05"), TEXT("household_dialog"), TEXT("объясни время"), EPhraseType::Request,
          EIntentID::GetWordFact, TEXT(""), TEXT("объяснить"), TEXT("время"), false, false, false },
        { TEXT("ru_dialog_pkg_06"), TEXT("household_dialog"), TEXT("расскажи время"), EPhraseType::Request,
          EIntentID::GetWordFact, TEXT(""), TEXT("рассказать"), TEXT("время"), false, false, false },
        { TEXT("ru_dialog_pkg_07"), TEXT("household_dialog"), TEXT("расскажи про время"), EPhraseType::Request,
          EIntentID::GetWordFact, TEXT(""), TEXT("рассказать"), TEXT("время"), false, false, false },
        { TEXT("ru_dialog_pkg_08"), TEXT("household_dialog"), TEXT("расскажи о дате"), EPhraseType::Request,
          EIntentID::GetWordFact, TEXT(""), TEXT("рассказать"), TEXT("дата"), false, false, false },
        { TEXT("ru_dialog_pkg_09"), TEXT("household_dialog"), TEXT("ты можешь объяснить время"), EPhraseType::Question,
          EIntentID::AnswerAbility, TEXT("ты"), TEXT("мочь"), TEXT("время"), true, false, false },
        { TEXT("ru_dialog_pkg_10"), TEXT("household_dialog"), TEXT("вы можете рассказать дату"), EPhraseType::Question,
          EIntentID::AnswerAbility, TEXT("вы"), TEXT("мочь"), TEXT("дата"), true, false, false },
        { TEXT("ru_dialog_pkg_11"), TEXT("household_dialog"), TEXT("скажи время"), EPhraseType::Request,
          EIntentID::GetWordFact, TEXT(""), TEXT("сказать"), TEXT("время"), false, false, false },
        { TEXT("ru_dialog_pkg_12"), TEXT("household_dialog"), TEXT("объясни почему ошибка"), EPhraseType::Request,
          EIntentID::GetWordFact, TEXT(""), TEXT("объяснить"), TEXT("ошибка"), false, false, false },
        { TEXT("ru_dialog_pkg_13"), TEXT("household_dialog"), TEXT("когда дата"), EPhraseType::Question,
          EIntentID::GetDefinition, TEXT(""), TEXT(""), TEXT("дата"), false, false, false },
        { TEXT("ru_dialog_pkg_14"), TEXT("household_dialog"), TEXT("сколько время"), EPhraseType::Question,
          EIntentID::GetDefinition, TEXT(""), TEXT(""), TEXT("время"), false, false, false },
        { TEXT("ru_dialog_pkg_15"), TEXT("household_dialog"), TEXT("почему ошибка"), EPhraseType::Question,
          EIntentID::GetDefinition, TEXT(""), TEXT(""), TEXT("ошибка"), false, false, false },
        { TEXT("ru_dialog_pkg_16"), TEXT("household_dialog"), TEXT("как дата"), EPhraseType::Question,
          EIntentID::GetDefinition, TEXT(""), TEXT(""), TEXT("дата"), false, false, false },
        { TEXT("ru_dialog_pkg_17"), TEXT("household_dialog"), TEXT("что такое ошибка"), EPhraseType::Question,
          EIntentID::GetDefinition, TEXT(""), TEXT(""), TEXT("ошибка"), false, true, false },
        { TEXT("ru_dialog_pkg_18"), TEXT("household_dialog"), TEXT("что такое отчёт"), EPhraseType::Question,
          EIntentID::GetDefinition, TEXT(""), TEXT(""), TEXT("отчёт"), false, true, false },
        { TEXT("ru_dialog_pkg_19"), TEXT("household_dialog"), TEXT("объясни дату"), EPhraseType::Request,
          EIntentID::GetWordFact, TEXT(""), TEXT("объяснить"), TEXT("дата"), false, false, false },
        { TEXT("ru_dialog_pkg_20"), TEXT("household_dialog"), TEXT("скажи дату"), EPhraseType::Request,
          EIntentID::GetWordFact, TEXT(""), TEXT("сказать"), TEXT("дата"), false, false, false },

        // package: asr_artifacts (20 fixtures)
        { TEXT("ru_asr_pkg_01"), TEXT("asr_artifacts"), TEXT("аткрой акно"), EPhraseType::Command,
          EIntentID::Unknown, TEXT(""), TEXT("открыть"), TEXT("окно"), false, false, false },
        { TEXT("ru_asr_pkg_02"), TEXT("asr_artifacts"), TEXT("акрой фаил"), EPhraseType::Command,
          EIntentID::Unknown, TEXT(""), TEXT("открыть"), TEXT("файл"), false, false, false },
        { TEXT("ru_asr_pkg_03"), TEXT("asr_artifacts"), TEXT("найтить значение синтакез"), EPhraseType::Command,
          EIntentID::FindMeaning, TEXT(""), TEXT("найти"), TEXT("значение"), false, false, false },
        { TEXT("ru_asr_pkg_04"), TEXT("asr_artifacts"), TEXT("чо значит синтакез"), EPhraseType::Question,
          EIntentID::GetDefinition, TEXT(""), TEXT("значить"), TEXT("синтаксис"), false, true, false },
        { TEXT("ru_asr_pkg_05"), TEXT("asr_artifacts"), TEXT("че означает марфология"), EPhraseType::Question,
          EIntentID::GetDefinition, TEXT(""), TEXT("означать"), TEXT("морфология"), false, true, false },
        { TEXT("ru_asr_pkg_06"), TEXT("asr_artifacts"), TEXT("расскажи пж про паметь"), EPhraseType::Request,
          EIntentID::GetWordFact, TEXT(""), TEXT("рассказать"), TEXT("память"), false, false, false },
        { TEXT("ru_asr_pkg_07"), TEXT("asr_artifacts"), TEXT("объясни плиз марфология"), EPhraseType::Request,
          EIntentID::GetWordFact, TEXT(""), TEXT("объяснить"), TEXT("морфология"), false, false, false },
        { TEXT("ru_asr_pkg_08"), TEXT("asr_artifacts"), TEXT("щас открой фаил"), EPhraseType::Command,
          EIntentID::Unknown, TEXT(""), TEXT("открыть"), TEXT("файл"), false, false, false },
        { TEXT("ru_asr_pkg_09"), TEXT("asr_artifacts"), TEXT("ща закрой акно"), EPhraseType::Command,
          EIntentID::Unknown, TEXT(""), TEXT("закрыть"), TEXT("окно"), false, false, false },
        { TEXT("ru_asr_pkg_10"), TEXT("asr_artifacts"), TEXT("кароч объясни синтакез"), EPhraseType::Request,
          EIntentID::GetWordFact, TEXT(""), TEXT("объяснить"), TEXT("синтаксис"), false, false, false },
        { TEXT("ru_asr_pkg_11"), TEXT("asr_artifacts"), TEXT("кароче расскажи про паметь"), EPhraseType::Request,
          EIntentID::GetWordFact, TEXT(""), TEXT("рассказать"), TEXT("память"), false, false, false },
        { TEXT("ru_asr_pkg_12"), TEXT("asr_artifacts"), TEXT("проверька фаил"), EPhraseType::Command,
          EIntentID::Unknown, TEXT(""), TEXT("проверить"), TEXT("файл"), false, false, false },
        { TEXT("ru_asr_pkg_13"), TEXT("asr_artifacts"), TEXT("запомена паметь"), EPhraseType::Command,
          EIntentID::Unknown, TEXT(""), TEXT("запомнить"), TEXT("память"), false, false, false },
        { TEXT("ru_asr_pkg_14"), TEXT("asr_artifacts"), TEXT("чо такое паметь"), EPhraseType::Question,
          EIntentID::GetDefinition, TEXT(""), TEXT(""), TEXT("память"), false, true, false },
        { TEXT("ru_asr_pkg_15"), TEXT("asr_artifacts"), TEXT("че такое синтакез"), EPhraseType::Question,
          EIntentID::GetDefinition, TEXT(""), TEXT(""), TEXT("синтаксис"), false, true, false },
        { TEXT("ru_asr_pkg_16"), TEXT("asr_artifacts"), TEXT("аткрой документ"), EPhraseType::Command,
          EIntentID::Unknown, TEXT(""), TEXT("открыть"), TEXT("документ"), false, false, false },
        { TEXT("ru_asr_pkg_17"), TEXT("asr_artifacts"), TEXT("найтить определение марфология"), EPhraseType::Command,
          EIntentID::FindMeaning, TEXT(""), TEXT("найти"), TEXT("определение"), false, false, false },
        { TEXT("ru_asr_pkg_18"), TEXT("asr_artifacts"), TEXT("чо означает фаил"), EPhraseType::Question,
          EIntentID::GetDefinition, TEXT(""), TEXT("означать"), TEXT("файл"), false, true, false },
        { TEXT("ru_asr_pkg_19"), TEXT("asr_artifacts"), TEXT("че значит паметь"), EPhraseType::Question,
          EIntentID::GetDefinition, TEXT(""), TEXT("значить"), TEXT("память"), false, true, false },
        { TEXT("ru_asr_pkg_20"), TEXT("asr_artifacts"), TEXT("расскажи пж о синтакез"), EPhraseType::Request,
          EIntentID::GetWordFact, TEXT(""), TEXT("рассказать"), TEXT("синтаксис"), false, false, false },

        // fallback
        { TEXT("en_fallback_01"), TEXT("fallback"), TEXT("what is syntax"), EPhraseType::Question,
          EIntentID::Unknown, TEXT(""), TEXT(""), TEXT(""), false, false, false },
        { TEXT("en_fallback_02"), TEXT("fallback"), TEXT("can you explain morphology"), EPhraseType::Question,
          EIntentID::Unknown, TEXT(""), TEXT(""), TEXT(""), false, false, false },
        { TEXT("ru_fallback_01"), TEXT("fallback"), TEXT("бррр вжух"), EPhraseType::Unknown,
          EIntentID::Unknown, TEXT(""), TEXT(""), TEXT(""), false, false, false }
    };
    return Fixtures;
}
