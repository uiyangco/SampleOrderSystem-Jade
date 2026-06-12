#include "SampleView.h"
#include <iomanip>
#include <iostream>

void SampleView::printRow(const Sample& s) {
    std::wstring stockStatus;
    WORD color = ConsoleUI::GREEN;
    if (s.stock == 0) {
        stockStatus = L"[고갈]"; color = ConsoleUI::RED;
    } else {
        stockStatus = L"[여유]"; color = ConsoleUI::GREEN;
    }
    ui_.printLine(
        std::wstring(L"  ") +
        std::to_wstring(s.id) + L"\t" +
        s.name + L"\t" +
        L"생산시간:" + std::to_wstring(s.avgProductionTime) + L"분\t" +
        L"수율:" + std::to_wstring(static_cast<int>(s.yield * 100)) + L"%\t" +
        L"재고:" + std::to_wstring(s.stock) + L"\t" +
        stockStatus, color);
}

void SampleView::showList(const std::vector<Sample>& samples) {
    ui_.printHeader(L"시료 목록");
    if (samples.empty()) { ui_.printInfo(L"등록된 시료가 없습니다."); return; }
    ui_.printLine(L"  ID\t이름\t평균생산시간\t수율\t재고\t상태", ConsoleUI::CYAN);
    ui_.printSeparator();
    for (const auto& s : samples) printRow(s);
}

void SampleView::showSearchResult(const std::vector<Sample>& results) {
    ui_.printHeader(L"검색 결과");
    if (results.empty()) { ui_.printInfo(L"검색 결과가 없습니다."); return; }
    for (const auto& s : results) printRow(s);
}

Sample SampleView::promptForSample() {
    ui_.printHeader(L"시료 등록");
    Sample s;
    s.name              = ui_.inputNonEmptyLine(L"  이름: ");
    s.avgProductionTime = ui_.inputInt(L"  평균 생산시간(분/개): ");
    s.yield             = ui_.inputDouble(L"  수율 (0.0 ~ 1.0, 예: 0.92): ", 0.0, 1.0);
    s.stock             = ui_.inputInt(L"  초기 재고: ");
    return s;
}
