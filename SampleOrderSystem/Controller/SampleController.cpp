#include "SampleController.h"
#include "../Utils.h"
#include <algorithm>

void SampleController::run() {
    while (true) {
        ui_.printHeader(L"시료 관리");
        ui_.printLine(L"  1. 시료 등록");
        ui_.printLine(L"  2. 시료 조회");
        ui_.printLine(L"  3. 시료 검색");
        ui_.printLine(L"  0. 뒤로");
        int c = ui_.inputInt(L"  선택: ");
        switch (c) {
        case 1: doRegister(); break;
        case 2: doList();     break;
        case 3: doSearch();   break;
        case 0: return;
        }
    }
}

void SampleController::doRegister() {
    Sample s  = view_.promptForSample();
    s.createdAt = Utils::nowWstring();
    repo_.create(s);
    ui_.printSuccess(L"시료 등록 완료 (ID: " + std::to_wstring(s.getId()) + L")");
    ui_.inputLine(L"엔터를 눌러 계속...");
}

void SampleController::doList() {
    view_.showList(repo_.readAll());
    ui_.inputLine(L"엔터를 눌러 계속...");
}

void SampleController::doSearch() {
    std::wstring keyword = ui_.inputLine(L"  검색어: ");
    auto all = repo_.readAll();
    std::vector<Sample> result;
    std::copy_if(all.begin(), all.end(), std::back_inserter(result),
        [&](const Sample& s) {
            return s.name.find(keyword) != std::wstring::npos;
        });
    view_.showSearchResult(result);
    ui_.inputLine(L"엔터를 눌러 계속...");
}
