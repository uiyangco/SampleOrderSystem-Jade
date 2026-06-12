#include <io.h>
#include <fcntl.h>
#include <Windows.h>
#include <iostream>

#include "Model/Repository/SampleRepository.h"
#include "Model/Repository/OrderRepository.h"
#include "Model/Repository/ProductionJobRepository.h"
#include "View/ConsoleUI.h"
#include "View/SampleView.h"
#include "View/OrderView.h"
#include "View/ProductionView.h"
#include "View/MonitorView.h"
#include "Controller/SampleController.h"
#include "Controller/OrderController.h"
#include "Controller/OrderMenuController.h"
#include "Controller/ProductionController.h"
#include "Controller/MonitorController.h"
#include "Monitor/ProductionThread.h"

static void initConsole() {
    _setmode(_fileno(stdout), _O_U16TEXT);
    _setmode(_fileno(stdin),  _O_U16TEXT);
    SetConsoleOutputCP(CP_UTF8);

    CONSOLE_FONT_INFOEX cfi{};
    cfi.cbSize = sizeof(cfi);
    GetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &cfi);
    wcscpy_s(cfi.FaceName, L"Malgun Gothic");
    SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &cfi);
}

int wmain() {
    initConsole();

    // 1분 = 5초 (테스트 편의용; 실운영 시 60.0으로 변경)
    constexpr double kTimeScaleSecPerMin = 1.0;

    // ── Repositories ────────────────────────────────────────────────────────
    SampleRepository        sampleRepo("data/samples.json");
    OrderRepository         orderRepo("data/orders.json");
    ProductionJobRepository jobRepo("data/production_jobs.json");

    // ── Production thread ───────────────────────────────────────────────────
    ProductionThread prodThread(jobRepo, orderRepo, sampleRepo, kTimeScaleSecPerMin);
    prodThread.start();

    // ── Views ────────────────────────────────────────────────────────────────
    ConsoleUI      ui;
    SampleView     sampleView(ui);
    OrderView      orderView(ui);
    ProductionView prodView(ui);
    MonitorView    monitorView(ui);

    // ── Controllers ──────────────────────────────────────────────────────────
    SampleController    sampleCtrl(sampleRepo, sampleView, ui);
    OrderController     orderCtrl(orderRepo, sampleRepo, jobRepo);
    OrderMenuController orderMenu(orderCtrl, orderRepo, sampleRepo, orderView, ui);
    ProductionController prodCtrl(jobRepo, orderRepo, sampleRepo, prodView, ui, kTimeScaleSecPerMin);
    MonitorController   monitorCtrl(orderRepo, sampleRepo, monitorView, ui);

    // ── Main loop ─────────────────────────────────────────────────────────────
    while (true) {
        ui.clear();
        ui.printHeader(L"SampleOrderSystem v1.0");
        ui.printLine(L"  1. 시료 관리");
        ui.printLine(L"  2. 시료 주문");
        ui.printLine(L"  3. 주문 승인/거절");
        ui.printLine(L"  4. 모니터링");
        ui.printLine(L"  5. 출고 처리");
        ui.printLine(L"  6. 생산 라인 조회");
        ui.printLine(L"  0. 종료");
        ui.printSeparator();

        int choice = ui.inputInt(L"  선택: ");
        switch (choice) {
        case 1: sampleCtrl.run();          break;
        case 2: orderMenu.runReserve();    break;
        case 3: orderMenu.runApproveReject(); break;
        case 4: monitorCtrl.run();         break;
        case 5: orderMenu.runRelease();    break;
        case 6: prodCtrl.run();            break;
        case 0:
            prodThread.stop();
            return 0;
        }
    }
}
