#include "OrderMenuController.h"

void OrderMenuController::runReserve() {
    auto samples = sampleRepo_.readAll();
    if (samples.empty()) {
        ui_.printError(L"등록된 시료가 없습니다.");
        ui_.inputLine(L"엔터를 눌러 계속...");
        return;
    }
    auto in = view_.promptForOrder(samples);
    ctrl_.reserve(in.sampleId, in.customerName, in.quantity);
    ui_.printSuccess(L"주문이 접수되었습니다. (RESERVED)");
    ui_.inputLine(L"엔터를 눌러 계속...");
}

void OrderMenuController::runApproveReject() {
    while (true) {
        ui_.clear();
        view_.showReservedList(orderRepo_.readAll(), sampleRepo_.readAll());

        int id = view_.promptOrderId(L"처리");
        if (id == 0) return;

        auto orderOpt = orderRepo_.read(id);
        if (!orderOpt || orderOpt->status != OrderStatus::RESERVED) {
            ui_.printError(L"유효하지 않은 주문 ID입니다.");
            ui_.inputLine(L"엔터를 눌러 계속...");
            continue;
        }

        if (view_.promptApproveOrReject()) {
            ctrl_.approve(id);
            ui_.printSuccess(L"주문 승인 처리 완료.");
        } else {
            ctrl_.reject(id);
            ui_.printSuccess(L"주문 거절 처리 완료.");
        }
        ui_.inputLine(L"엔터를 눌러 계속...");
        return;
    }
}

void OrderMenuController::runRelease() {
    while (true) {
        ui_.clear();
        view_.showConfirmedList(orderRepo_.readAll(), sampleRepo_.readAll());

        int id = view_.promptOrderId(L"출고");
        if (id == 0) return;

        auto orderOpt = orderRepo_.read(id);
        if (!orderOpt || orderOpt->status != OrderStatus::CONFIRMED) {
            ui_.printError(L"CONFIRMED 상태의 주문이 아닙니다.");
            ui_.inputLine(L"엔터를 눌러 계속...");
            continue;
        }

        ctrl_.processRelease(id);
        ui_.printSuccess(L"출고 처리 완료. (RELEASE)");
        ui_.inputLine(L"엔터를 눌러 계속...");
        return;
    }
}
