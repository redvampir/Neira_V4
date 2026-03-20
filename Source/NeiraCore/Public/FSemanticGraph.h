#pragma once

#include "CoreMinimal.h"

// ---------------------------------------------------------------------------
// Тип семантической связи между словами
// ---------------------------------------------------------------------------
enum class ERelationType : uint8
{
    Unknown,
    Synonym,      // кот → кошка (то же значение)
    Antonym,      // горячий → холодный (противоположное)
    Hypernym,     // кот → животное (is-a, родовое понятие)
    Hyponym,      // животное → кот (has-subtype, видовое понятие)
    Meronym,      // кот → лапа (has-part, часть целого)
    Collocation,  // часто встречается рядом в текстах
};

// ---------------------------------------------------------------------------
// Семантическая связь
// ---------------------------------------------------------------------------
struct NEIRACORE_API FSemanticRelation
{
    FString       TargetLemma;                       // к какому слову
    ERelationType Type    = ERelationType::Unknown;  // тип связи
    float         Weight  = 1.0f;                    // уверенность [0..1]
    FString       Source;                            // "wiktionary"|"ruwordnet"|"manual"
};

// ---------------------------------------------------------------------------
// FSemanticGraph
//
// v0.1 — Семантический граф слов на основе SQLite.
//
// Загружает граф из файла semantics.db, сгенерированного
// build_semantic_graph.py.  Поддерживает:
//   — синонимы / антонимы
//   — гиперонимы (is-a) / гипонимы (has-subtype)
//   — меронимы (has-part)
//   — коллокации
//
// После Load() все запросы выполняются in-memory (граф загружается целиком).
// Для тестов и ручного заполнения доступны AddWord() / AddRelation().
//
// Пример:
//   FSemanticGraph Graph;
//   Graph.Load(TEXT("Data/Semantics/semantics.db"));
//   TArray<FString> Synonyms = Graph.GetSynonyms(TEXT("кот"));
// ---------------------------------------------------------------------------
struct NEIRACORE_API FSemanticGraph
{
    FSemanticGraph();
    ~FSemanticGraph();

    // Запрещаем копирование (владеет внутренним состоянием)
    FSemanticGraph(const FSemanticGraph&)            = delete;
    FSemanticGraph& operator=(const FSemanticGraph&) = delete;

    // ---------------------------------------------------------------------------
    // Загрузка
    // ---------------------------------------------------------------------------

    /**
     * Загрузить граф из SQLite файла.
     * Путь ":memory:" создаёт пустой in-memory граф (для тестов).
     * @return true при успехе (или если граф уже загружен).
     */
    bool Load(const FString& DbPath);

    /**
     * Проверить, загружен ли граф.
     */
    bool IsLoaded() const;

    // ---------------------------------------------------------------------------
    // Семантические запросы
    // ---------------------------------------------------------------------------

    /** Синонимы: слова с близким значением. */
    TArray<FString> GetSynonyms(const FString& Lemma) const;

    /** Антонимы: слова с противоположным значением. */
    TArray<FString> GetAntonyms(const FString& Lemma) const;

    /** Гиперонимы: родовые понятия (кот → животное). */
    TArray<FString> GetHypernyms(const FString& Lemma) const;

    /** Гипонимы: видовые понятия (животное → кот). */
    TArray<FString> GetHyponyms(const FString& Lemma) const;

    /** Меронимы: части целого (кот → лапа). */
    TArray<FString> GetMeronyms(const FString& Lemma) const;

    /** Все связи слова, любого типа. */
    TArray<FSemanticRelation> GetRelations(const FString& Lemma) const;

    /**
     * Проверить наличие прямой связи любого типа между двумя словами.
     * @param A первое слово
     * @param B второе слово
     * @param OutType (out) тип найденной связи
     */
    bool AreRelated(const FString& A, const FString& B,
                    ERelationType& OutType) const;

    // ---------------------------------------------------------------------------
    // Статистика
    // ---------------------------------------------------------------------------

    /** Количество уникальных слов в графе. */
    int32 GetWordCount() const;

    /** Количество связей в графе. */
    int32 GetRelationCount() const;

    // ---------------------------------------------------------------------------
    // Ручное заполнение (для тестов и расширений)
    // ---------------------------------------------------------------------------

    /**
     * Добавить слово вручную. Если уже есть — игнорируется.
     * @return true если слово добавлено, false если уже было.
     */
    bool AddWord(const FString& Lemma, const FString& Pos = TEXT(""));

    /**
     * Добавить связь вручную (дубли игнорируются).
     * Требует предварительного вызова Load() или наличия in-memory графа.
     * @return true если связь добавлена.
     */
    bool AddRelation(const FString& FromLemma, const FString& ToLemma,
                     ERelationType Type,
                     float Weight   = 1.0f,
                     const FString& Source = TEXT("manual"));

private:
    // Pimpl-подобный подход: скрываем sqlite3* за void*
    void* DbHandle  = nullptr;   // sqlite3*
    bool  bLoaded   = false;

    bool EnsureSchema();
    int32 GetOrCreateWordId(const FString& Lemma, const FString& Pos = TEXT(""));

    TArray<FString> GetRelationsByType(
        const FString& Lemma, ERelationType Type) const;

    static FString RelationTypeName(ERelationType Type);
    static ERelationType RelationTypeFromName(const FString& Name);
};
