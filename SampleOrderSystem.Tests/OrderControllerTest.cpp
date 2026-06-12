#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Controller/OrderController.h"
#include "Mocks/MockSampleRepository.h"
#include "Mocks/MockOrderRepository.h"
#include "Mocks/MockProductionJobRepository.h"

using ::testing::Return;
using ::testing::_;
using ::testing::Invoke;

// ── reserve ──────────────────────────────────────────────────────────────────

TEST(OrderControllerTest, Reserve_CreatesOrderWithReservedStatus) {
    MockOrderRepository         orderRepo;
    MockSampleRepository        sampleRepo;
    MockProductionJobRepository jobRepo;

    Sample s; s.setId(1); s.name = L"시료A"; s.stock = 10;
    EXPECT_CALL(sampleRepo, read(1)).WillOnce(Return(s));
    EXPECT_CALL(orderRepo, create(_))
        .WillOnce(Invoke([](Order& o) {
            EXPECT_EQ(o.status, OrderStatus::RESERVED);
            EXPECT_EQ(o.sampleId, 1);
            EXPECT_EQ(o.quantity, 3);
            o.setId(1);
            return true;
        }));

    OrderController ctrl(orderRepo, sampleRepo, jobRepo);
    ctrl.reserve(1, L"홍길동", 3);
}

// ── approve: 재고 충분 → shortage=0, targetQty=0 (즉시 완료용 Job) ────────────

TEST(OrderControllerTest, Approve_WhenStockSufficient_ShortageZero) {
    MockOrderRepository         orderRepo;
    MockSampleRepository        sampleRepo;
    MockProductionJobRepository jobRepo;

    Order o; o.setId(10); o.sampleId = 2; o.quantity = 5;
    o.status = OrderStatus::RESERVED;

    Sample s; s.setId(2); s.stock = 10; s.yield = 0.9; s.avgProductionTime = 5;

    // 다른 주문 없음 → effective=10, shortage=0 → 잡 없이 바로 CONFIRMED
    EXPECT_CALL(orderRepo, read(10)).WillOnce(Return(o));
    EXPECT_CALL(sampleRepo, read(2)).WillOnce(Return(s));
    EXPECT_CALL(orderRepo,  readAll()).WillOnce(Return(std::vector<Order>{}));
    EXPECT_CALL(sampleRepo, update(_)).Times(0);
    EXPECT_CALL(jobRepo,    create(_)).Times(0);
    EXPECT_CALL(orderRepo, update(_))
        .WillOnce(Invoke([](const Order& updated) {
            EXPECT_EQ(updated.status, OrderStatus::CONFIRMED);
            return true;
        }));

    OrderController ctrl(orderRepo, sampleRepo, jobRepo);
    ctrl.approve(10);
}

// ── approve: 재고 부족 → ceil(shortage/(yield*0.9)) ──────────────────────────

TEST(OrderControllerTest, Approve_WhenStockInsufficient_UsesYieldFormula) {
    MockOrderRepository         orderRepo;
    MockSampleRepository        sampleRepo;
    MockProductionJobRepository jobRepo;

    Order o; o.setId(11); o.sampleId = 3; o.quantity = 10;
    o.status = OrderStatus::RESERVED;

    // stock=3, qty=10 → shortage=7 → ceil(7/(0.8*0.9))=ceil(9.72)=10
    Sample s; s.setId(3); s.stock = 3; s.yield = 0.8; s.avgProductionTime = 6;

    EXPECT_CALL(orderRepo, read(11)).WillOnce(Return(o));
    EXPECT_CALL(sampleRepo, read(3)).WillOnce(Return(s));
    EXPECT_CALL(orderRepo,  readAll()).WillOnce(Return(std::vector<Order>{}));
    EXPECT_CALL(sampleRepo, update(_)).Times(0);
    EXPECT_CALL(jobRepo, create(_))
        .WillOnce(Invoke([](ProductionJob& job) {
            EXPECT_EQ(job.shortage,        7);
            EXPECT_EQ(job.stockAtApproval, 3);   // stock at approval time
            EXPECT_EQ(job.targetQty,       10);   // ceil(7/0.72)
            EXPECT_EQ(job.totalMinutes,    60);   // 6 * 10
            EXPECT_EQ(job.status, JobStatus::WAITING);
            job.setId(1);
            return true;
        }));
    EXPECT_CALL(orderRepo, update(_))
        .WillOnce(Invoke([](const Order& updated) {
            EXPECT_EQ(updated.status, OrderStatus::PRODUCING);
            return true;
        }));

    OrderController ctrl(orderRepo, sampleRepo, jobRepo);
    ctrl.approve(11);
}

// ── approve: yield=0(미설정)일 때 부족분을 그대로 생산량으로 사용 ────────────

TEST(OrderControllerTest, Approve_WhenYieldIsZero_UsesShortageAsTargetQty) {
    MockOrderRepository         orderRepo;
    MockSampleRepository        sampleRepo;
    MockProductionJobRepository jobRepo;

    Order o; o.setId(12); o.sampleId = 9; o.quantity = 5;
    o.status = OrderStatus::RESERVED;

    // yield=0(미설정), stock=0 → shortage=5, targetQty=5(1:1)
    Sample s; s.setId(9); s.stock = 0; s.yield = 0.0; s.avgProductionTime = 3;

    EXPECT_CALL(orderRepo, read(12)).WillOnce(Return(o));
    EXPECT_CALL(sampleRepo, read(9)).WillOnce(Return(s));
    EXPECT_CALL(orderRepo,  readAll()).WillOnce(Return(std::vector<Order>{}));
    EXPECT_CALL(sampleRepo, update(_)).Times(0);
    EXPECT_CALL(jobRepo, create(_))
        .WillOnce(Invoke([](ProductionJob& job) {
            EXPECT_EQ(job.shortage,     5);
            EXPECT_EQ(job.targetQty,    5);    // yield=0 → 1:1
            EXPECT_EQ(job.totalMinutes, 15);   // 3 * 5
            job.setId(1);
            return true;
        }));
    EXPECT_CALL(orderRepo, update(_)).WillOnce(Return(true));

    OrderController ctrl(orderRepo, sampleRepo, jobRepo);
    ctrl.approve(12);
}

// ── approve: PRODUCING 주문량만 재고에서 차감, 잡 예정 생산량 불포함 ────────────

TEST(OrderControllerTest, Approve_WithRunningJob_StockMinusProducingCommitment) {
    MockOrderRepository         orderRepo;
    MockSampleRepository        sampleRepo;
    MockProductionJobRepository jobRepo;

    Order newOrder; newOrder.setId(40); newOrder.sampleId = 6; newOrder.quantity = 25;
    newOrder.status = OrderStatus::RESERVED;

    // stock=40 (running job이 40개 생산해서 재고에 반영된 상태)
    // PRODUCING order qty=30 → effective = 40 - 30 = 10
    // shortage = 25-10 = 15, targetQty = ceil(15/0.81) = 19
    Sample s; s.setId(6); s.stock = 40; s.yield = 0.9; s.avgProductionTime = 5;

    Order producing; producing.setId(7); producing.sampleId = 6;
    producing.quantity = 30; producing.status = OrderStatus::PRODUCING;

    EXPECT_CALL(orderRepo, read(40)).WillOnce(Return(newOrder));
    EXPECT_CALL(sampleRepo, read(6)).WillOnce(Return(s));
    EXPECT_CALL(orderRepo,  readAll()).WillOnce(Return(std::vector<Order>{producing}));
    EXPECT_CALL(sampleRepo, update(_)).Times(0);
    EXPECT_CALL(jobRepo, create(_))
        .WillOnce(Invoke([](ProductionJob& job) {
            EXPECT_EQ(job.shortage,     15);
            EXPECT_EQ(job.targetQty,    19);   // ceil(15/0.81)
            EXPECT_EQ(job.totalMinutes, 95);   // 5 * 19
            job.setId(2);
            return true;
        }));
    EXPECT_CALL(orderRepo, update(_)).WillOnce(Return(true));

    OrderController ctrl(orderRepo, sampleRepo, jobRepo);
    ctrl.approve(40);
}

// ── approve: 재고가 PRODUCING+새주문량 커버 시 바로 confirmed ─────────────────

TEST(OrderControllerTest, Approve_StockCoversProducingAndNew_ShortageZero) {
    MockOrderRepository         orderRepo;
    MockSampleRepository        sampleRepo;
    MockProductionJobRepository jobRepo;

    Order newOrder; newOrder.setId(41); newOrder.sampleId = 6; newOrder.quantity = 20;
    newOrder.status = OrderStatus::RESERVED;

    // stock=121, PRODUCING order qty=100 → effective = 121-100 = 21 >= 20
    // shortage=0, targetQty=0 → 즉시 confirmed
    Sample s; s.setId(6); s.stock = 121; s.yield = 0.9; s.avgProductionTime = 5;

    Order producing; producing.setId(8); producing.sampleId = 6;
    producing.quantity = 100; producing.status = OrderStatus::PRODUCING;

    EXPECT_CALL(orderRepo, read(41)).WillOnce(Return(newOrder));
    EXPECT_CALL(sampleRepo, read(6)).WillOnce(Return(s));
    EXPECT_CALL(orderRepo,  readAll()).WillOnce(Return(std::vector<Order>{producing}));
    EXPECT_CALL(sampleRepo, update(_)).Times(0);
    EXPECT_CALL(jobRepo,    create(_)).Times(0);
    EXPECT_CALL(orderRepo, update(_))
        .WillOnce(Invoke([](const Order& updated) {
            EXPECT_EQ(updated.status, OrderStatus::CONFIRMED);
            return true;
        }));

    OrderController ctrl(orderRepo, sampleRepo, jobRepo);
    ctrl.approve(41);
}

// ── approve: 앞선 CONFIRMED 주문이 재고를 선점한 경우 ────────────────────────

TEST(OrderControllerTest, Approve_WithExistingConfirmedOrder_AccountsForCommittedStock) {
    MockOrderRepository         orderRepo;
    MockSampleRepository        sampleRepo;
    MockProductionJobRepository jobRepo;

    Order newOrder; newOrder.setId(20); newOrder.sampleId = 4; newOrder.quantity = 5;
    newOrder.status = OrderStatus::RESERVED;

    Sample s; s.setId(4); s.stock = 10; s.yield = 0.9; s.avgProductionTime = 5;

    // 기존 CONFIRMED 주문: qty=8 → effective = 10-8=2, shortage=5-2=3
    // targetQty = ceil(3/(0.9*0.9)) = ceil(3.703) = 4
    Order confirmed; confirmed.setId(5); confirmed.sampleId = 4;
    confirmed.quantity = 8; confirmed.status = OrderStatus::CONFIRMED;

    EXPECT_CALL(orderRepo, read(20)).WillOnce(Return(newOrder));
    EXPECT_CALL(sampleRepo, read(4)).WillOnce(Return(s));
    EXPECT_CALL(orderRepo,  readAll()).WillOnce(Return(std::vector<Order>{confirmed}));
    EXPECT_CALL(sampleRepo, update(_)).Times(0);
    EXPECT_CALL(jobRepo, create(_))
        .WillOnce(Invoke([](ProductionJob& job) {
            EXPECT_EQ(job.shortage,     3);
            EXPECT_EQ(job.targetQty,    4);    // ceil(3/0.81)
            EXPECT_EQ(job.totalMinutes, 20);   // 5 * 4
            job.setId(1);
            return true;
        }));
    EXPECT_CALL(orderRepo, update(_)).WillOnce(Return(true));

    OrderController ctrl(orderRepo, sampleRepo, jobRepo);
    ctrl.approve(20);
}

// ── approve: PRODUCING 주문량을 재고에서 차감해 부족분 계산 ──────────────────

TEST(OrderControllerTest, Approve_WithProducingOrder_DeductsQtyFromStock) {
    MockOrderRepository         orderRepo;
    MockSampleRepository        sampleRepo;
    MockProductionJobRepository jobRepo;

    Order newOrder; newOrder.setId(30); newOrder.sampleId = 5; newOrder.quantity = 5;
    newOrder.status = OrderStatus::RESERVED;

    Sample s; s.setId(5); s.stock = 2; s.yield = 0.9; s.avgProductionTime = 5;

    // PRODUCING 주문 qty=3 → effective = 2 - 3 = -1 → max(0,-1)=0
    // shortage = 5-0 = 5, targetQty = ceil(5/0.81) = 7
    Order producing; producing.setId(6); producing.sampleId = 5;
    producing.quantity = 3; producing.status = OrderStatus::PRODUCING;

    EXPECT_CALL(orderRepo, read(30)).WillOnce(Return(newOrder));
    EXPECT_CALL(sampleRepo, read(5)).WillOnce(Return(s));
    EXPECT_CALL(orderRepo,  readAll()).WillOnce(Return(std::vector<Order>{producing}));
    EXPECT_CALL(sampleRepo, update(_)).Times(0);
    EXPECT_CALL(jobRepo, create(_))
        .WillOnce(Invoke([](ProductionJob& job) {
            EXPECT_EQ(job.shortage,     5);
            EXPECT_EQ(job.targetQty,    7);    // ceil(5/0.81)
            EXPECT_EQ(job.totalMinutes, 35);   // 5 * 7
            job.setId(2);
            return true;
        }));
    EXPECT_CALL(orderRepo, update(_)).WillOnce(Return(true));

    OrderController ctrl(orderRepo, sampleRepo, jobRepo);
    ctrl.approve(30);
}

// ── reject ────────────────────────────────────────────────────────────────────

TEST(OrderControllerTest, Reject_SetsRejectedStatus) {
    MockOrderRepository         orderRepo;
    MockSampleRepository        sampleRepo;
    MockProductionJobRepository jobRepo;

    Order o; o.setId(40); o.status = OrderStatus::RESERVED;
    EXPECT_CALL(orderRepo, read(40)).WillOnce(Return(o));
    EXPECT_CALL(orderRepo, update(_))
        .WillOnce(Invoke([](const Order& updated) {
            EXPECT_EQ(updated.status, OrderStatus::REJECTED);
            return true;
        }));

    OrderController ctrl(orderRepo, sampleRepo, jobRepo);
    ctrl.reject(40);
}

// ── processRelease: 재고 차감 ─────────────────────────────────────────────────

TEST(OrderControllerTest, ProcessRelease_DeductsStockAndSetsRelease) {
    MockOrderRepository         orderRepo;
    MockSampleRepository        sampleRepo;
    MockProductionJobRepository jobRepo;

    Order o; o.setId(50); o.sampleId = 5; o.quantity = 5;
    o.status = OrderStatus::CONFIRMED;

    Sample s; s.setId(5); s.stock = 10;

    EXPECT_CALL(orderRepo, read(50)).WillOnce(Return(o));
    EXPECT_CALL(sampleRepo, read(5)).WillOnce(Return(s));
    EXPECT_CALL(sampleRepo, update(_))
        .WillOnce(Invoke([](const Sample& updated) {
            EXPECT_EQ(updated.stock, 5);
            return true;
        }));
    EXPECT_CALL(orderRepo, update(_))
        .WillOnce(Invoke([](const Order& updated) {
            EXPECT_EQ(updated.status, OrderStatus::RELEASE);
            return true;
        }));

    OrderController ctrl(orderRepo, sampleRepo, jobRepo);
    EXPECT_TRUE(ctrl.processRelease(50));
}

// ── processRelease: 재고 부족 시 실패 ─────────────────────────────────────────

TEST(OrderControllerTest, ProcessRelease_WhenInsufficientStock_ReturnsFalse) {
    MockOrderRepository         orderRepo;
    MockSampleRepository        sampleRepo;
    MockProductionJobRepository jobRepo;

    Order o; o.setId(51); o.sampleId = 6; o.quantity = 10;
    o.status = OrderStatus::CONFIRMED;

    Sample s; s.setId(6); s.stock = 3;

    EXPECT_CALL(orderRepo, read(51)).WillOnce(Return(o));
    EXPECT_CALL(sampleRepo, read(6)).WillOnce(Return(s));
    EXPECT_CALL(sampleRepo, update(_)).Times(0);
    EXPECT_CALL(orderRepo,  update(_)).Times(0);

    OrderController ctrl(orderRepo, sampleRepo, jobRepo);
    EXPECT_FALSE(ctrl.processRelease(51));
}
