#pragma once
#define NOMINMAX
#include <string>
#include <Windows.h>

class ConsoleUI {
public:
    static constexpr WORD WHITE  = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
    static constexpr WORD GRAY   = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    static constexpr WORD CYAN   = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
    static constexpr WORD GREEN  = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
    static constexpr WORD YELLOW = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
    static constexpr WORD RED    = FOREGROUND_RED | FOREGROUND_INTENSITY;
    static constexpr WORD BLUE   = FOREGROUND_BLUE | FOREGROUND_INTENSITY;

    ConsoleUI();
    void printLine(const std::wstring& text, WORD color = WHITE);
    void printHeader(const std::wstring& title);
    void printSeparator();
    void printError(const std::wstring& msg);
    void printSuccess(const std::wstring& msg);
    void printInfo(const std::wstring& msg);
    void clear();

    std::wstring inputLine(const std::wstring& prompt);
    int          inputInt(const std::wstring& prompt);
    double       inputDouble(const std::wstring& prompt, double minVal, double maxVal);

private:
    void setColor(WORD color);
    void resetColor();

    HANDLE hConsole_;
    WORD   defaultColor_;
};
