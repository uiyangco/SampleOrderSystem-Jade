#include "SampleController.h"
#include "../Utils.h"
#include <algorithm>

void SampleController::run() {
    while (true) {
        ui_.printHeader(L"시료 관리");
        ui_.printLine(L"  1. 시료 등록");
        ui_.printLine(L"  2. 시료 조회");
        ui_.printLine(L"  3. 시료 검색");
        ui_.printLine(L"  4. 시료 제거");
        ui_.printLine(L"  0. 뒤로");
        int c = ui_.inputInt(L"  선택: ");
        switch (c) {
        case 1: doRegister(); break;
        case 2: doList();     break;
        case 3: doSearch();   break;
        case 4: doRemove();   break;
        case 0: return;
        }
    }
}

bool SampleController::isDuplicateName(const std::wstring&) const {
    return false;
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

void SampleController::doRemove() {
    auto samples = repo_.readAll();
    if (samples.empty()) {
        ui_.printError(L"등록된 시료가 없습니다.");
        ui_.inputLine(L"엔터를 눌러 계속...");
        return;
    }
    view_.showList(samples);
    int id = ui_.inputInt(L"  삭제할 시료 ID (0=뒤로): ");
    if (id == 0) return;

    auto it = std::find_if(samples.begin(), samples.end(),
        [id](const Sample& s) { return s.id == id; });
    if (it == samples.end()) {
        ui_.printError(L"존재하지 않는 ID입니다.");
        ui_.inputLine(L"엔터를 눌러 계속...");
        return;
    }

    ui_.printLine(L"  대상: " + it->name +
                  L"  재고:" + std::to_wstring(it->stock), ConsoleUI::YELLOW);
    std::wstring confirm = ui_.inputLine(L"  정말 삭제하시겠습니까? (y/n): ");
    if (confirm != L"y" && confirm != L"Y") {
        ui_.printInfo(L"취소되었습니다.");
        ui_.inputLine(L"엔터를 눌러 계속...");
        return;
    }

    repo_.remove(id);
    ui_.printSuccess(L"시료가 삭제되었습니다.");
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
