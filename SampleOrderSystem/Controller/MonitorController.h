#pragma once
#include "../Model/Repository/IRepository.h"
#include "../Model/Order.h"
#include "../Model/Sample.h"
#include "../View/MonitorView.h"
#include "../View/ConsoleUI.h"

class MonitorController {
public:
    MonitorController(
        IRepository<Order>&  orderRepo,
        IRepository<Sample>& sampleRepo,
        MonitorView&         view,
        ConsoleUI&           ui)
        : orderRepo_(orderRepo), sampleRepo_(sampleRepo)
        , view_(view), ui_(ui) {}

    void run();

private:
    IRepository<Order>&  orderRepo_;
    IRepository<Sample>& sampleRepo_;
    MonitorView&         view_;
    ConsoleUI&           ui_;
};
