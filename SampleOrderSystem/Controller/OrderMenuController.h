#pragma once
#include "OrderController.h"
#include "../View/OrderView.h"
#include "../View/ConsoleUI.h"
#include "../Model/Repository/IRepository.h"

// UI 메뉴 루프를 담당하는 래퍼. OrderController는 순수 비즈니스 로직만 가짐.
class OrderMenuController {
public:
    OrderMenuController(
        OrderController&     ctrl,
        IRepository<Order>&  orderRepo,
        IRepository<Sample>& sampleRepo,
        OrderView&           view,
        ConsoleUI&           ui)
        : ctrl_(ctrl), orderRepo_(orderRepo), sampleRepo_(sampleRepo)
        , view_(view), ui_(ui) {}

    void runReserve();
    void runApproveReject();
    void runRelease();

private:
    OrderController&     ctrl_;
    IRepository<Order>&  orderRepo_;
    IRepository<Sample>& sampleRepo_;
    OrderView&           view_;
    ConsoleUI&           ui_;
};
