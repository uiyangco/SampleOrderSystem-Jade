#include "MonitorController.h"
#include <thread>
#include <atomic>
#include <chrono>
#include <iostream>

void MonitorController::run() {
    std::atomic<bool> watching(true);

    // 별도 스레드: 2초마다 화면 갱신
    std::thread watchThread([&]() {
        while (watching.load()) {
            ui_.clear();
            view_.showDashboard(orderRepo_.readAll(), sampleRepo_.readAll());
            ui_.printLine(L"\n  [모니터링 중] 종료하려면 엔터를 누르세요.", ConsoleUI::GRAY);
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    });

    // 메인 스레드: 입력 대기
    std::wstring dummy;
    std::getline(std::wcin, dummy);
    watching.store(false);
    watchThread.join();
}
