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

    // 다른 주문/잡 없음 → effective=10, shortage=0, targetQty=0
    EXPECT_CALL(orderRepo, read(10)).WillOnce(Return(o));
    EXPECT_CALL(sampleRepo, read(2)).WillOnce(Return(s));
    EXPECT_CALL(orderRepo,  readAll()).WillOnce(Return(std::vector<Order>{}));
    EXPECT_CALL(jobRepo,    readAll()).WillOnce(Return(std::vector<ProductionJob>{}));
    EXPECT_CALL(sampleRepo, update(_)).Times(0);
    EXPECT_CALL(jobRepo, create(_))
        .WillOnce(Invoke([](ProductionJob& job) {
            EXPECT_EQ(job.shortage,     0);
            EXPECT_EQ(job.targetQty,    0);
            EXPECT_EQ(job.totalMinutes, 0);
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
    EXPECT_CALL(jobRepo,    readAll()).WillOnce(Return(std::vector<ProductionJob>{}));
    EXPECT_CALL(sampleRepo, update(_)).Times(0);
    EXPECT_CALL(jobRepo, create(_))
        .WillOnce(Invoke([](ProductionJob& job) {
            EXPECT_EQ(job.shortage,     7);
            EXPECT_EQ(job.targetQty,    10);   // ceil(7/0.72)
            EXPECT_EQ(job.totalMinutes, 60);   // 6 * 10
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
    EXPECT_CALL(jobRepo,    readAll()).WillOnce(Return(std::vector<ProductionJob>{}));
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

// ── approve: 앞선 PRODUCING 잡의 예정 생산량을 고려한 부족분 계산 ─────────────

TEST(OrderControllerTest, Approve_WithExistingProducingJob_AccountsForPlannedProduction) {
    MockOrderRepository         orderRepo;
    MockSampleRepository        sampleRepo;
    MockProductionJobRepository jobRepo;

    Order newOrder; newOrder.setId(30); newOrder.sampleId = 5; newOrder.quantity = 5;
    newOrder.status = OrderStatus::RESERVED;

    Sample s; s.setId(5); s.stock = 2; s.yield = 0.9; s.avgProductionTime = 5;

    // 기존 PRODUCING 주문 qty=3, 그 잡 targetQty=4 (WAITING)
    // effective = 2 + 4(예정) - 3(선점) = 3, shortage=5-3=2
    // targetQty = ceil(2/0.81) = ceil(2.469) = 3
    Order producing; producing.setId(6); producing.sampleId = 5;
    producing.quantity = 3; producing.status = OrderStatus::PRODUCING;

    ProductionJob existingJob; existingJob.setId(1);
    existingJob.orderId = 6; existingJob.sampleId = 5;
    existingJob.targetQty = 4; existingJob.status = JobStatus::WAITING;

    EXPECT_CALL(orderRepo, read(30)).WillOnce(Return(newOrder));
    EXPECT_CALL(sampleRepo, read(5)).WillOnce(Return(s));
    EXPECT_CALL(orderRepo,  readAll()).WillOnce(Return(std::vector<Order>{producing}));
    EXPECT_CALL(jobRepo,    readAll()).WillOnce(Return(std::vector<ProductionJob>{existingJob}));
    EXPECT_CALL(sampleRepo, update(_)).Times(0);
    EXPECT_CALL(jobRepo, create(_))
        .WillOnce(Invoke([](ProductionJob& job) {
            EXPECT_EQ(job.shortage,     2);
            EXPECT_EQ(job.targetQty,    3);    // ceil(2/0.81)
            EXPECT_EQ(job.totalMinutes, 15);   // 5 * 3
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
