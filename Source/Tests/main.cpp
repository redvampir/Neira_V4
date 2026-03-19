// Точка входа нативного тест-раннера для Нейры.
// Все тесты регистрируются через IMPLEMENT_SIMPLE_AUTOMATION_TEST
// в подключаемых .cpp-файлах.

#include "test_runner.h"

int main() {
    return RunAllTests();
}
