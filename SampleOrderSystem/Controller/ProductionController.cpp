#include "ProductionController.h"

void ProductionController::run() {
    view_.showProductionStatus(
        jobRepo_.readAll(),
        orderRepo_.readAll(),
        sampleRepo_.readAll(),
        timeScale_);
    ui_.inputLine(L"엔터를 눌러 계속...");
}
