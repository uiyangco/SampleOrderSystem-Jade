#include "ProductionController.h"
#include <thread>
#include <atomic>
#include <chrono>
#include <iostream>

void ProductionController::run() {
    std::atomic<bool> watching(true);

    std::thread watchThread([&]() {
        while (watching.load()) {
            ui_.clear();
            view_.showProductionStatus(
                jobRepo_.readAll(),
                orderRepo_.readAll(),
                sampleRepo_.readAll(),
                timeScale_);
            ui_.printLine(L"\n  [실시간 갱신 중] 종료하려면 엔터를 누르세요.", ConsoleUI::GRAY);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });

    std::wstring dummy;
    std::getline(std::wcin, dummy);
    watching.store(false);
    watchThread.join();
}
