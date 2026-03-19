#pragma once

#include "CoreMinimal.h"

// ---------------------------------------------------------------------------
// Тип фразы — результат первичной классификации входа
// ---------------------------------------------------------------------------
enum class EPhraseType : uint8
{
    Unknown,
    Question,    // "что такое X?", "кто такой Y?"
    Command,     // "открой X", "найди Y"
    Statement,   // "кот — животное"
    Request,     // "скажи мне X", "объясни Y"
};

// ---------------------------------------------------------------------------
// Намерение — что агент должен сделать
// ---------------------------------------------------------------------------
enum class EIntentID : uint8
{
    Unknown,
    GetDefinition,    // GET_DEFINITION
    GetWordFact,      // GET_WORD_FACT
    CheckTextErrors,  // CHECK_TEXT_ERRORS
    FindMeaning,      // FIND_MEANING
    AnswerAbility,    // ANSWER_ABILITY
    StoreFact,        // STORE_FACT
    RetrieveMemory,   // RETRIEVE_FROM_MEMORY
    CheckMemoryLoad,  // CHECK_MEMORY_LOAD
};

// ---------------------------------------------------------------------------
// Действие — исполняемая единица
// ---------------------------------------------------------------------------
enum class EActionID : uint8
{
    Unknown,
    GetDefinition,
    GetWordFact,
    CheckTextErrors,
    FindMeaning,
    AnswerAbility,
    StoreFact,
    RetrieveMemory,
    CheckMemoryLoad,
};

enum class EActionFailReason : uint8
{
    None,           // успех
    EmptyInput,     // входная строка пустая/из пробелов
    PartialParse,   // синтаксис разобран частично, intent не извлечён полностью
    UnknownIntent,  // fallback сработал, но намерение не распознано
    NotFound,       // объект не найден
    NotSupported,   // действие не реализовано
    LowConfidence,  // уверенность ниже порога
    InternalError,  // ошибка выполнения
};

// ---------------------------------------------------------------------------
// Состояние знания
// ---------------------------------------------------------------------------
enum class EKnowledgeState : uint8
{
    Pending,            // гипотеза, ещё не проверена
    Confirmed,          // подтверждена хотя бы раз
    VerifiedKnowledge,  // многократно подтверждена, считается фактом
    Conflicted,         // обнаружен конфликт
    Deprecated,         // устарела / вытеснена
};
