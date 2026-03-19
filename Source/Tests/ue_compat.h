#pragma once
// Минимальный слой совместимости с Unreal Engine для нативной компиляции тестов.
// Заменяет CoreMinimal.h без зависимостей от UE.

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <stdarg.h>
#include <limits>

// --- UE export макросы — в нативной сборке пусты ---
#define NEIRACORE_API

// --- базовые типы ---
using int32  = int;
using uint8  = unsigned char;
using uint32 = unsigned int;
using int64  = long long;
using TCHAR  = char;

// --- Служебные утилиты ---
static constexpr int32 INDEX_NONE = -1;

template<typename T>
T&& MoveTemp(T& v) { return std::move(v); }

template<typename T>
using TFunction = std::function<T>;

// --- TEXT() — просто строковый литерал ---
#define TEXT(x) x

// --------------------------------------------------------------------------
// Forward-объявление TArray, чтобы FString мог использовать его в методе
// --------------------------------------------------------------------------
template<typename T> class TArray;

// --------------------------------------------------------------------------
// FString — обёртка над std::string
// --------------------------------------------------------------------------
class FString {
public:
    std::string Data;

    FString() = default;
    FString(const char* s) : Data(s ? s : "") {}
    FString(const std::string& s) : Data(s) {}

    bool  IsEmpty() const { return Data.empty(); }
    int32 Len()     const { return (int32)Data.size(); }

    FString TrimStartAndEnd() const {
        auto s = Data;
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char c){ return !std::isspace(c); }));
        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char c){ return !std::isspace(c); }).base(), s.end());
        return FString(s);
    }
    FString TrimEnd() const {
        std::string s = Data;
        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char c){ return !std::isspace(c); }).base(), s.end());
        return FString(s);
    }
    FString TrimStart() const {
        std::string s = Data;
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char c){ return !std::isspace(c); }));
        return FString(s);
    }

    FString Mid(int32 Start, int32 Count = -1) const {
        if (Start >= (int32)Data.size()) return FString("");
        if (Count < 0) return FString(Data.substr(Start));
        return FString(Data.substr(Start, Count));
    }
    FString Left(int32 Count) const {
        return FString(Data.substr(0, std::min((size_t)Count, Data.size())));
    }
    FString Right(int32 Count) const {
        int32 start = (int32)Data.size() - Count;
        return FString(Data.substr(start < 0 ? 0 : start));
    }
    FString LeftChop(int32 Count) const {
        int32 len = (int32)Data.size() - Count;
        return len <= 0 ? FString("") : FString(Data.substr(0, len));
    }
    FString RightChop(int32 Count) const {
        return Count >= (int32)Data.size() ? FString("") : FString(Data.substr(Count));
    }

    char operator[](int32 i) const { return Data[i]; }
    char Last(int32 Offset = 0) const {
        if (Data.empty()) return 0;
        int idx = (int)Data.size() - 1 - Offset;
        return idx >= 0 ? Data[idx] : 0;
    }

    FString ToLower() const {
        std::string s = Data;
        for (auto& c : s) {
            if (c >= 'A' && c <= 'Z') c = c - 'A' + 'a';
        }
        // Кириллица UTF-8
        std::string out; out.reserve(s.size());
        for (size_t i = 0; i < s.size(); ) {
            unsigned char c1 = (unsigned char)s[i];
            if (c1 == 0xD0 && i + 1 < s.size()) {
                unsigned char c2 = (unsigned char)s[i+1];
                if (c2 >= 0x90 && c2 <= 0x9F) { out += (char)0xD0; out += (char)(c2 + 0x20); i += 2; continue; }
                if (c2 >= 0xA0 && c2 <= 0xAF) { out += (char)0xD1; out += (char)(c2 - 0x20); i += 2; continue; }
            }
            out += s[i++];
        }
        return FString(out);
    }

    bool StartsWith(const FString& Prefix, bool bCS = true) const {
        const std::string& p = bCS ? Prefix.Data : Prefix.ToLower().Data;
        const std::string& d = bCS ? Data : this->ToLower().Data;
        return d.size() >= p.size() && d.substr(0, p.size()) == p;
    }
    bool EndsWith(const FString& Suffix, bool bCS = true) const {
        const std::string& s = bCS ? Suffix.Data : Suffix.ToLower().Data;
        const std::string& d = bCS ? Data : this->ToLower().Data;
        return d.size() >= s.size() && d.substr(d.size() - s.size()) == s;
    }
    bool Contains(const FString& Sub, bool bCS = true) const {
        if (bCS) return Data.find(Sub.Data) != std::string::npos;
        return ToLower().Data.find(Sub.ToLower().Data) != std::string::npos;
    }
    int32 Find(const FString& Sub, bool bCS = true, int32 StartPos = 0) const {
        auto pos = bCS ? Data.find(Sub.Data, StartPos) : ToLower().Data.find(Sub.ToLower().Data, StartPos);
        return pos == std::string::npos ? INDEX_NONE : (int32)pos;
    }

    FString Replace(const char* From, const char* To) const {
        std::string s = Data, f(From), t(To);
        size_t pos = 0;
        while ((pos = s.find(f, pos)) != std::string::npos) { s.replace(pos, f.size(), t); pos += t.size(); }
        return FString(s);
    }
    void RemoveAt(int32 Index, int32 Count = 1) { Data.erase(Index, Count); }
    int32 Num() const { return (int32)Data.size(); }

    static FString Printf(const char* Fmt, ...) {
        char buf[1024]; va_list args; va_start(args, Fmt); vsnprintf(buf, sizeof(buf), Fmt, args); va_end(args);
        return FString(buf);
    }

    bool operator==(const FString& O) const { return Data == O.Data; }
    bool operator!=(const FString& O) const { return Data != O.Data; }
    bool operator< (const FString& O) const { return Data <  O.Data; }
    FString operator+(const FString& O) const { return FString(Data + O.Data); }
    FString& operator+=(const FString& O) { Data += O.Data; return *this; }
    const char* operator*() const { return Data.c_str(); }

    // range-based for (итерация по байтам)
    const char* begin() const { return Data.c_str(); }
    const char*   end() const { return Data.c_str() + Data.size(); }

    // ParseIntoArrayWS — реализация ниже, после полного определения TArray<FString>
    void ParseIntoArrayWS(TArray<FString>& Out) const;
};

// Hash для FString
namespace std {
template<> struct hash<FString> {
    size_t operator()(const FString& s) const { return std::hash<std::string>{}(s.Data); }
};
}

// --------------------------------------------------------------------------
// TArray<T>
// --------------------------------------------------------------------------
template<typename T>
class TArray : public std::vector<T> {
public:
    using std::vector<T>::vector;
    int32 Num() const { return (int32)this->size(); }
    void Add(const T& v) { this->push_back(v); }
    void Add(T&& v)      { this->push_back(std::move(v)); }
    bool Contains(const T& v) const { return std::find(this->begin(), this->end(), v) != this->end(); }
    T* FindByPredicate(std::function<bool(const T&)> Pred) {
        for (auto& e : *this) {
            if (Pred(e)) {
                return &e;
            }
        }
        return nullptr;
    }
    const T* FindByPredicate(std::function<bool(const T&)> Pred) const {
        for (const auto& e : *this) {
            if (Pred(e)) {
                return &e;
            }
        }
        return nullptr;
    }
    void Reset() { this->clear(); }
    void Reserve(int32 Count) { this->reserve((size_t)Count); }
    bool IsEmpty() const { return this->empty(); }
    bool IsValidIndex(int32 i) const { return i >= 0 && i < (int32)this->size(); }
};

// Реализация ParseIntoArrayWS (после полного определения TArray<FString>)
inline void FString::ParseIntoArrayWS(TArray<FString>& Out) const {
    Out.clear();
    std::istringstream iss(Data);
    std::string token;
    while (iss >> token) Out.Add(FString(token));
}

// --------------------------------------------------------------------------
// Hash для enum-ключей
// --------------------------------------------------------------------------
struct EnumHash {
    template<typename T>
    size_t operator()(T t) const { return std::hash<int>{}(static_cast<int>(t)); }
};

// --------------------------------------------------------------------------
// TMap<K,V>
// --------------------------------------------------------------------------
template<typename K, typename V>
class TMap : public std::unordered_map<K, V, EnumHash> {
public:
    void Add(const K& Key, const V& Val) { (*this)[Key] = Val; }
    bool Contains(const K& Key) const { return this->count(Key) > 0; }
    V* Find(const K& Key) { auto it = this->find(Key); return it != this->end() ? &it->second : nullptr; }
    const V* Find(const K& Key) const { auto it = this->find(Key); return it != this->end() ? &it->second : nullptr; }
};

// --------------------------------------------------------------------------
// TSet<T>
// --------------------------------------------------------------------------
template<typename T>
class TSet : public std::unordered_set<T> {
public:
    void Add(const T& v) { this->insert(v); }
    bool Contains(const T& v) const { return this->count(v) > 0; }
};

// --------------------------------------------------------------------------
// FChar
// --------------------------------------------------------------------------
struct FChar {
    static bool IsDigit(char c)      { return c >= '0' && c <= '9'; }
    static bool IsAlpha(char c)      { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
    static bool IsWhitespace(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }
};


// --------------------------------------------------------------------------
// TNumericLimits (UE-совместимый адаптер)
// --------------------------------------------------------------------------
template<typename T>
struct TNumericLimits {
    static constexpr T Lowest() { return std::numeric_limits<T>::lowest(); }
};

// --------------------------------------------------------------------------
// FMath
// --------------------------------------------------------------------------
struct FMath {
    static float Clamp(float V, float Min, float Max) { return V < Min ? Min : (V > Max ? Max : V); }
};

// --------------------------------------------------------------------------
// UE_LOG — printf в stderr
// --------------------------------------------------------------------------
#define LogWarning "Warning"
#define LogTemp    "Temp"
#define UE_LOG(Cat, Level, Fmt, ...) fprintf(stderr, "[" #Level "] " Fmt "\n", ##__VA_ARGS__)
