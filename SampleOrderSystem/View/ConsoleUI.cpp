#include "ConsoleUI.h"
#include <iostream>
#include <io.h>
#include <fcntl.h>

ConsoleUI::ConsoleUI() {
    hConsole_     = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info{};
    GetConsoleScreenBufferInfo(hConsole_, &info);
    defaultColor_ = info.wAttributes;
}

void ConsoleUI::setColor(WORD color) {
    SetConsoleTextAttribute(hConsole_, color);
}

void ConsoleUI::resetColor() {
    SetConsoleTextAttribute(hConsole_, defaultColor_);
}

void ConsoleUI::printLine(const std::wstring& text, WORD color) {
    setColor(color);
    std::wcout << text << L"\n";
    resetColor();
}

void ConsoleUI::printHeader(const std::wstring& title) {
    std::wstring bar(50, L'=');
    printLine(bar, CYAN);
    printLine(L"  " + title, CYAN);
    printLine(bar, CYAN);
}

void ConsoleUI::printSeparator() {
    printLine(std::wstring(50, L'-'), GRAY);
}

void ConsoleUI::printError(const std::wstring& msg) {
    printLine(L"[오류] " + msg, RED);
}

void ConsoleUI::printSuccess(const std::wstring& msg) {
    printLine(L"[완료] " + msg, GREEN);
}

void ConsoleUI::printInfo(const std::wstring& msg) {
    printLine(L"[정보] " + msg, YELLOW);
}

void ConsoleUI::clear() {
    system("cls");
}

std::wstring ConsoleUI::inputLine(const std::wstring& prompt) {
    setColor(WHITE);
    std::wcout << prompt;
    resetColor();
    std::wstring line;
    std::getline(std::wcin, line);
    return line;
}

int ConsoleUI::inputInt(const std::wstring& prompt) {
    while (true) {
        std::wstring s = inputLine(prompt);
        try {
            return std::stoi(s);
        } catch (...) {
            printError(L"숫자를 입력하세요.");
        }
    }
}
