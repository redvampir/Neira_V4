@echo off
chcp 65001 >nul
cd /d "%~dp0"

echo ═══════════════════════════════════════════════════════════
echo   ТЕСТОВЫЙ ДИАЛОГ С НЕЙРОЙ
echo ═══════════════════════════════════════════════════════════
echo.

(
echo что такое нейра?
timeout /t 1 /nobreak >nul
echo ты можешь нарисовать квадрат?
timeout /t 1 /nobreak >nul
echo найди значение слова дом
timeout /t 1 /nobreak >nul
echo кот - это животное
timeout /t 1 /nobreak >nul
echo кто такой пушкин?
timeout /t 1 /nobreak >nul
echo объясни что такое синтаксис
timeout /t 1 /nobreak >nul
echo quit
) | NeiraDialog.exe

echo.
echo ═══════════════════════════════════════════════════════════
echo   ТЕСТ ЗАВЕРШЁН
echo ═══════════════════════════════════════════════════════════
pause
