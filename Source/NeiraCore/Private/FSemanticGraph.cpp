// FSemanticGraph.cpp
// v0.1 — Семантический граф на основе SQLite.
//
// Хранит связи между словами (синонимы, антонимы, гиперонимы и др.),
// загруженные из semantics.db, сгенерированного build_semantic_graph.py.
//
// Зависимость: sqlite3 (libsqlite3-dev, -lsqlite3 в линкере).

#include "FSemanticGraph.h"
#include <sqlite3.h>
#include <cstdio>

// ---------------------------------------------------------------------------
// Утилиты преобразования типа связи
// ---------------------------------------------------------------------------
FString FSemanticGraph::RelationTypeName(ERelationType Type)
{
    switch (Type)
    {
        case ERelationType::Synonym:     return TEXT("synonym");
        case ERelationType::Antonym:     return TEXT("antonym");
        case ERelationType::Hypernym:    return TEXT("hypernym");
        case ERelationType::Hyponym:     return TEXT("hyponym");
        case ERelationType::Meronym:     return TEXT("meronym");
        case ERelationType::Collocation: return TEXT("collocation");
        default:                         return TEXT("unknown");
    }
}

ERelationType FSemanticGraph::RelationTypeFromName(const FString& Name)
{
    if (Name == TEXT("synonym"))     return ERelationType::Synonym;
    if (Name == TEXT("antonym"))     return ERelationType::Antonym;
    if (Name == TEXT("hypernym"))    return ERelationType::Hypernym;
    if (Name == TEXT("hyponym"))     return ERelationType::Hyponym;
    if (Name == TEXT("meronym"))     return ERelationType::Meronym;
    if (Name == TEXT("collocation")) return ERelationType::Collocation;
    return ERelationType::Unknown;
}

// ---------------------------------------------------------------------------
// Конструктор / деструктор
// ---------------------------------------------------------------------------
FSemanticGraph::FSemanticGraph()
    : DbHandle(nullptr)
    , bLoaded(false)
{
}

FSemanticGraph::~FSemanticGraph()
{
    if (DbHandle)
    {
        sqlite3_close(static_cast<sqlite3*>(DbHandle));
        DbHandle = nullptr;
    }
}

// ---------------------------------------------------------------------------
// Загрузка
// ---------------------------------------------------------------------------
bool FSemanticGraph::Load(const FString& DbPath)
{
    if (bLoaded)
        return true;

    sqlite3* Db = nullptr;
    int Rc = sqlite3_open(*DbPath, &Db);
    if (Rc != SQLITE_OK)
    {
        fprintf(stderr, "[SemanticGraph] Не удалось открыть БД '%s': %s\n",
                *DbPath, sqlite3_errmsg(Db));
        sqlite3_close(Db);
        return false;
    }

    DbHandle = Db;

    if (!EnsureSchema())
    {
        sqlite3_close(Db);
        DbHandle = nullptr;
        return false;
    }

    bLoaded = true;
    return true;
}

bool FSemanticGraph::IsLoaded() const
{
    return bLoaded;
}

// ---------------------------------------------------------------------------
// Схема БД
// ---------------------------------------------------------------------------
bool FSemanticGraph::EnsureSchema()
{
    const char* Sql =
        "CREATE TABLE IF NOT EXISTS words ("
        "  id         INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  lemma      TEXT NOT NULL UNIQUE,"
        "  pos        TEXT DEFAULT ''"
        ");"
        "CREATE TABLE IF NOT EXISTS relations ("
        "  id            INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  word_id       INTEGER NOT NULL,"
        "  target_lemma  TEXT    NOT NULL,"
        "  relation_type TEXT    NOT NULL,"
        "  weight        REAL    DEFAULT 1.0,"
        "  source        TEXT    DEFAULT 'manual',"
        "  FOREIGN KEY(word_id) REFERENCES words(id) ON DELETE CASCADE"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_words_lemma    ON words(lemma);"
        "CREATE INDEX IF NOT EXISTS idx_relations_word ON relations(word_id);"
        "CREATE INDEX IF NOT EXISTS idx_relations_type ON relations(relation_type);";

    char* ErrMsg = nullptr;
    int Rc = sqlite3_exec(static_cast<sqlite3*>(DbHandle), Sql, nullptr, nullptr, &ErrMsg);
    if (Rc != SQLITE_OK)
    {
        fprintf(stderr, "[SemanticGraph] Ошибка схемы: %s\n", ErrMsg);
        sqlite3_free(ErrMsg);
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Вспомогательный метод: получить или создать ID слова
// ---------------------------------------------------------------------------
int32 FSemanticGraph::GetOrCreateWordId(const FString& Lemma, const FString& Pos)
{
    sqlite3* Db = static_cast<sqlite3*>(DbHandle);

    // Сначала поиск
    {
        sqlite3_stmt* Stmt = nullptr;
        sqlite3_prepare_v2(Db, "SELECT id FROM words WHERE lemma=?", -1, &Stmt, nullptr);
        sqlite3_bind_text(Stmt, 1, *Lemma, -1, SQLITE_TRANSIENT);
        int32 Id = -1;
        if (sqlite3_step(Stmt) == SQLITE_ROW)
            Id = sqlite3_column_int(Stmt, 0);
        sqlite3_finalize(Stmt);
        if (Id != -1)
            return Id;
    }

    // Вставка
    {
        sqlite3_stmt* Stmt = nullptr;
        sqlite3_prepare_v2(Db, "INSERT INTO words (lemma, pos) VALUES (?, ?)", -1, &Stmt, nullptr);
        sqlite3_bind_text(Stmt, 1, *Lemma, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(Stmt, 2, *Pos,   -1, SQLITE_TRANSIENT);
        sqlite3_step(Stmt);
        sqlite3_finalize(Stmt);
        return static_cast<int32>(sqlite3_last_insert_rowid(Db));
    }
}

// ---------------------------------------------------------------------------
// Ручное заполнение
// ---------------------------------------------------------------------------
bool FSemanticGraph::AddWord(const FString& Lemma, const FString& Pos)
{
    if (!bLoaded)
        return false;

    sqlite3* Db = static_cast<sqlite3*>(DbHandle);

    // Проверить наличие
    sqlite3_stmt* Stmt = nullptr;
    sqlite3_prepare_v2(Db, "SELECT id FROM words WHERE lemma=?", -1, &Stmt, nullptr);
    sqlite3_bind_text(Stmt, 1, *Lemma, -1, SQLITE_TRANSIENT);
    bool Exists = (sqlite3_step(Stmt) == SQLITE_ROW);
    sqlite3_finalize(Stmt);

    if (Exists)
        return false;

    GetOrCreateWordId(Lemma, Pos);
    return true;
}

bool FSemanticGraph::AddRelation(const FString& FromLemma, const FString& ToLemma,
                                  ERelationType Type,
                                  float Weight, const FString& Source)
{
    if (!bLoaded || FromLemma.IsEmpty() || ToLemma.IsEmpty())
        return false;

    sqlite3* Db = static_cast<sqlite3*>(DbHandle);

    int32 WordId = GetOrCreateWordId(FromLemma);
    FString TypeName = RelationTypeName(Type);

    // Проверить дубль
    {
        sqlite3_stmt* Stmt = nullptr;
        sqlite3_prepare_v2(Db,
            "SELECT id FROM relations WHERE word_id=? AND target_lemma=? AND relation_type=?",
            -1, &Stmt, nullptr);
        sqlite3_bind_int (Stmt, 1, WordId);
        sqlite3_bind_text(Stmt, 2, *ToLemma,  -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(Stmt, 3, *TypeName,  -1, SQLITE_TRANSIENT);
        bool Dup = (sqlite3_step(Stmt) == SQLITE_ROW);
        sqlite3_finalize(Stmt);
        if (Dup)
            return false;
    }

    // Вставить
    sqlite3_stmt* Stmt = nullptr;
    sqlite3_prepare_v2(Db,
        "INSERT INTO relations (word_id, target_lemma, relation_type, weight, source) "
        "VALUES (?, ?, ?, ?, ?)",
        -1, &Stmt, nullptr);
    sqlite3_bind_int   (Stmt, 1, WordId);
    sqlite3_bind_text  (Stmt, 2, *ToLemma,  -1, SQLITE_TRANSIENT);
    sqlite3_bind_text  (Stmt, 3, *TypeName,  -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(Stmt, 4, Weight);
    sqlite3_bind_text  (Stmt, 5, *Source,    -1, SQLITE_TRANSIENT);
    sqlite3_step(Stmt);
    sqlite3_finalize(Stmt);
    return true;
}

// ---------------------------------------------------------------------------
// Семантические запросы
// ---------------------------------------------------------------------------
TArray<FString> FSemanticGraph::GetRelationsByType(
    const FString& Lemma, ERelationType Type) const
{
    TArray<FString> Result;
    if (!bLoaded)
        return Result;

    sqlite3* Db = static_cast<sqlite3*>(DbHandle);
    FString TypeName = RelationTypeName(Type);

    sqlite3_stmt* Stmt = nullptr;
    sqlite3_prepare_v2(Db,
        "SELECT r.target_lemma FROM relations r "
        "JOIN words w ON r.word_id=w.id "
        "WHERE w.lemma=? AND r.relation_type=? "
        "ORDER BY r.weight DESC",
        -1, &Stmt, nullptr);
    sqlite3_bind_text(Stmt, 1, *Lemma,    -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(Stmt, 2, *TypeName, -1, SQLITE_TRANSIENT);

    while (sqlite3_step(Stmt) == SQLITE_ROW)
    {
        const char* Raw = reinterpret_cast<const char*>(sqlite3_column_text(Stmt, 0));
        if (Raw)
            Result.Add(FString(Raw));
    }
    sqlite3_finalize(Stmt);
    return Result;
}

TArray<FString> FSemanticGraph::GetSynonyms(const FString& Lemma) const
{
    return GetRelationsByType(Lemma, ERelationType::Synonym);
}

TArray<FString> FSemanticGraph::GetAntonyms(const FString& Lemma) const
{
    return GetRelationsByType(Lemma, ERelationType::Antonym);
}

TArray<FString> FSemanticGraph::GetHypernyms(const FString& Lemma) const
{
    return GetRelationsByType(Lemma, ERelationType::Hypernym);
}

TArray<FString> FSemanticGraph::GetHyponyms(const FString& Lemma) const
{
    return GetRelationsByType(Lemma, ERelationType::Hyponym);
}

TArray<FString> FSemanticGraph::GetMeronyms(const FString& Lemma) const
{
    return GetRelationsByType(Lemma, ERelationType::Meronym);
}

TArray<FSemanticRelation> FSemanticGraph::GetRelations(const FString& Lemma) const
{
    TArray<FSemanticRelation> Result;
    if (!bLoaded)
        return Result;

    sqlite3* Db = static_cast<sqlite3*>(DbHandle);

    sqlite3_stmt* Stmt = nullptr;
    sqlite3_prepare_v2(Db,
        "SELECT r.target_lemma, r.relation_type, r.weight, r.source "
        "FROM relations r "
        "JOIN words w ON r.word_id=w.id "
        "WHERE w.lemma=? "
        "ORDER BY r.weight DESC",
        -1, &Stmt, nullptr);
    sqlite3_bind_text(Stmt, 1, *Lemma, -1, SQLITE_TRANSIENT);

    while (sqlite3_step(Stmt) == SQLITE_ROW)
    {
        FSemanticRelation Rel;
        const char* Target = reinterpret_cast<const char*>(sqlite3_column_text(Stmt, 0));
        const char* RType  = reinterpret_cast<const char*>(sqlite3_column_text(Stmt, 1));
        const char* Src    = reinterpret_cast<const char*>(sqlite3_column_text(Stmt, 3));

        if (Target) Rel.TargetLemma = FString(Target);
        if (RType)  Rel.Type        = RelationTypeFromName(FString(RType));
        Rel.Weight = static_cast<float>(sqlite3_column_double(Stmt, 2));
        if (Src)    Rel.Source      = FString(Src);

        Result.Add(Rel);
    }
    sqlite3_finalize(Stmt);
    return Result;
}

bool FSemanticGraph::AreRelated(const FString& A, const FString& B,
                                 ERelationType& OutType) const
{
    OutType = ERelationType::Unknown;
    if (!bLoaded)
        return false;

    sqlite3* Db = static_cast<sqlite3*>(DbHandle);

    sqlite3_stmt* Stmt = nullptr;
    sqlite3_prepare_v2(Db,
        "SELECT r.relation_type FROM relations r "
        "JOIN words w ON r.word_id=w.id "
        "WHERE w.lemma=? AND r.target_lemma=? "
        "LIMIT 1",
        -1, &Stmt, nullptr);
    sqlite3_bind_text(Stmt, 1, *A, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(Stmt, 2, *B, -1, SQLITE_TRANSIENT);

    bool Found = false;
    if (sqlite3_step(Stmt) == SQLITE_ROW)
    {
        const char* RType = reinterpret_cast<const char*>(sqlite3_column_text(Stmt, 0));
        if (RType)
            OutType = RelationTypeFromName(FString(RType));
        Found = true;
    }
    sqlite3_finalize(Stmt);
    return Found;
}

// ---------------------------------------------------------------------------
// Статистика
// ---------------------------------------------------------------------------
int32 FSemanticGraph::GetWordCount() const
{
    if (!bLoaded)
        return 0;

    sqlite3_stmt* Stmt = nullptr;
    sqlite3_prepare_v2(static_cast<sqlite3*>(DbHandle),
        "SELECT COUNT(*) FROM words", -1, &Stmt, nullptr);
    int32 Count = 0;
    if (sqlite3_step(Stmt) == SQLITE_ROW)
        Count = sqlite3_column_int(Stmt, 0);
    sqlite3_finalize(Stmt);
    return Count;
}

int32 FSemanticGraph::GetRelationCount() const
{
    if (!bLoaded)
        return 0;

    sqlite3_stmt* Stmt = nullptr;
    sqlite3_prepare_v2(static_cast<sqlite3*>(DbHandle),
        "SELECT COUNT(*) FROM relations", -1, &Stmt, nullptr);
    int32 Count = 0;
    if (sqlite3_step(Stmt) == SQLITE_ROW)
        Count = sqlite3_column_int(Stmt, 0);
    sqlite3_finalize(Stmt);
    return Count;
}
