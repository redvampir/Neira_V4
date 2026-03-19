#pragma once
// Минимальный тест-раннер, имитирующий UE Automation Test API.

#include "ue_compat.h"
#include <vector>
#include <functional>
#include <string>
#include <cstdio>
#include <cmath>

// --------------------------------------------------------------------------
// Глобальный реестр тестов
// --------------------------------------------------------------------------
struct TestCase {
    std::string Name;
    std::function<bool()> Run;
};

inline std::vector<TestCase>& GetTestRegistry() {
    static std::vector<TestCase> Registry;
    return Registry;
}

// Счётчики
inline int& PassCount() { static int n = 0; return n; }
inline int& FailCount() { static int n = 0; return n; }
inline std::string g_CurrentTest;
inline bool        g_CurrentPassed = false;

// --------------------------------------------------------------------------
// Assertions — возвращают bool, не кидают исключений
// --------------------------------------------------------------------------
template<typename A, typename B>
inline bool TestEqual(const char* Desc, const A& Got, const B& Expected) {
    if (Got == (A)Expected) return true;
    // Формируем сообщение об ошибке
    g_CurrentPassed = false;
    printf("    FAIL [%s]: значения не совпадают\n", Desc);
    return false;
}

// Специализация для FString
inline bool TestEqual(const char* Desc, const FString& Got, const FString& Expected) {
    if (Got == Expected) return true;
    g_CurrentPassed = false;
    printf("    FAIL [%s]: '%s' != '%s'\n", Desc, Got.Data.c_str(), Expected.Data.c_str());
    return false;
}

inline bool TestEqual(const char* Desc, int32 Got, int32 Expected) {
    if (Got == Expected) return true;
    g_CurrentPassed = false;
    printf("    FAIL [%s]: %d != %d\n", Desc, Got, Expected);
    return false;
}

inline bool TestEqual(const char* Desc, float Got, float Expected) {
    if (std::fabs(Got - Expected) < 1e-5f) return true;
    g_CurrentPassed = false;
    printf("    FAIL [%s]: %.6f != %.6f\n", Desc, Got, Expected);
    return false;
}

inline bool TestTrue(const char* Desc, bool Value) {
    if (Value) return true;
    g_CurrentPassed = false;
    printf("    FAIL [%s]: ожидалось true\n", Desc);
    return false;
}

inline bool TestFalse(const char* Desc, bool Value) {
    if (!Value) return true;
    g_CurrentPassed = false;
    printf("    FAIL [%s]: ожидалось false\n", Desc);
    return false;
}

template<typename T>
inline bool TestNotNull(const char* Desc, const T* Ptr) {
    if (Ptr != nullptr) return true;
    g_CurrentPassed = false;
    printf("    FAIL [%s]: ожидался не-null указатель\n", Desc);
    return false;
}

template<typename T>
inline bool TestNull(const char* Desc, const T* Ptr) {
    if (Ptr == nullptr) return true;
    g_CurrentPassed = false;
    printf("    FAIL [%s]: ожидался null указатель\n", Desc);
    return false;
}

// --------------------------------------------------------------------------
// Макрос для регистрации теста (имитация IMPLEMENT_SIMPLE_AUTOMATION_TEST)
// --------------------------------------------------------------------------
struct AutoTestRegistrar {
    AutoTestRegistrar(const char* Name, std::function<bool(const FString&)> Fn) {
        GetTestRegistry().push_back({
            Name,
            [Name, Fn]() -> bool {
                g_CurrentPassed = true;
                g_CurrentTest = Name;
                bool ret = Fn(FString(""));
                return g_CurrentPassed && ret;
            }
        });
    }
};

// Имитация: класс с методом RunTest
#define IMPLEMENT_SIMPLE_AUTOMATION_TEST(ClassName, TestName, Flags)        \
    struct ClassName {                                                        \
        bool RunTest(const FString& Parameters);                             \
    };                                                                        \
    static AutoTestRegistrar ClassName##_Reg(TestName,                       \
        [](const FString& P) -> bool { ClassName inst; return inst.RunTest(P); });

// Флаги — игнорируем
namespace EAutomationTestFlags {
    enum : int { EditorContext = 0, ProductFilter = 0 };
}

// --------------------------------------------------------------------------
// main() — запускает все зарегистрированные тесты
// --------------------------------------------------------------------------
inline int RunAllTests() {
    auto& tests = GetTestRegistry();
    printf("=== Neira Native Tests (%d тестов) ===\n\n", (int)tests.size());

    for (auto& t : tests) {
        bool ok = t.Run();
        if (ok) {
            printf("  PASS  %s\n", t.Name.c_str());
            PassCount()++;
        } else {
            printf("  FAIL  %s\n", t.Name.c_str());
            FailCount()++;
        }
    }

    printf("\n=== Итог: %d PASS, %d FAIL из %d ===\n",
        PassCount(), FailCount(), (int)tests.size());
    return FailCount() > 0 ? 1 : 0;
}
