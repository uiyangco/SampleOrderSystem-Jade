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

// ── approve: 재고 충분 ────────────────────────────────────────────────────────

TEST(OrderControllerTest, Approve_WhenStockSufficient_SetsConfirmed) {
    MockOrderRepository         orderRepo;
    MockSampleRepository        sampleRepo;
    MockProductionJobRepository jobRepo;

    Order o; o.setId(10); o.sampleId = 2; o.quantity = 5;
    o.status = OrderStatus::RESERVED;

    Sample s; s.setId(2); s.stock = 10; s.yield = 0.9; s.avgProductionTime = 5;

    EXPECT_CALL(orderRepo, read(10)).WillOnce(Return(o));
    EXPECT_CALL(sampleRepo, read(2)).WillOnce(Return(s));
    EXPECT_CALL(sampleRepo, update(_))
        .WillOnce(Invoke([](const Sample& updated) {
            EXPECT_EQ(updated.stock, 5);  // 10 - 5
            return true;
        }));
    EXPECT_CALL(orderRepo, update(_))
        .WillOnce(Invoke([](const Order& updated) {
            EXPECT_EQ(updated.status, OrderStatus::CONFIRMED);
            return true;
        }));

    OrderController ctrl(orderRepo, sampleRepo, jobRepo);
    ctrl.approve(10);
}

// ── approve: 재고 부족 → 생산 큐 등록 ────────────────────────────────────────

TEST(OrderControllerTest, Approve_WhenStockInsufficient_SetsProducingAndCreatesJob) {
    MockOrderRepository         orderRepo;
    MockSampleRepository        sampleRepo;
    MockProductionJobRepository jobRepo;

    Order o; o.setId(11); o.sampleId = 3; o.quantity = 10;
    o.status = OrderStatus::RESERVED;

    // 재고 3개, 주문 10개 → 부족분 7개
    // targetQty = ceil(7 / (0.8 * 0.9)) = ceil(7 / 0.72) = ceil(9.72) = 10
    Sample s; s.setId(3); s.stock = 3; s.yield = 0.8; s.avgProductionTime = 6;

    EXPECT_CALL(orderRepo, read(11)).WillOnce(Return(o));
    EXPECT_CALL(sampleRepo, read(3)).WillOnce(Return(s));
    EXPECT_CALL(jobRepo, create(_))
        .WillOnce(Invoke([](ProductionJob& job) {
            EXPECT_EQ(job.orderId,      11);
            EXPECT_EQ(job.sampleId,     3);
            EXPECT_EQ(job.targetQty,    10);     // ceil(7/0.72)
            EXPECT_EQ(job.totalMinutes, 60);     // 6 * 10
            EXPECT_EQ(job.status,       JobStatus::WAITING);
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

// ── reject ────────────────────────────────────────────────────────────────────

TEST(OrderControllerTest, Reject_SetsRejectedStatus) {
    MockOrderRepository         orderRepo;
    MockSampleRepository        sampleRepo;
    MockProductionJobRepository jobRepo;

    Order o; o.setId(20); o.status = OrderStatus::RESERVED;
    EXPECT_CALL(orderRepo, read(20)).WillOnce(Return(o));
    EXPECT_CALL(orderRepo, update(_))
        .WillOnce(Invoke([](const Order& updated) {
            EXPECT_EQ(updated.status, OrderStatus::REJECTED);
            return true;
        }));

    OrderController ctrl(orderRepo, sampleRepo, jobRepo);
    ctrl.reject(20);
}

// ── processRelease ────────────────────────────────────────────────────────────

TEST(OrderControllerTest, ProcessRelease_SetsReleaseStatus) {
    MockOrderRepository         orderRepo;
    MockSampleRepository        sampleRepo;
    MockProductionJobRepository jobRepo;

    Order o; o.setId(30); o.status = OrderStatus::CONFIRMED;
    EXPECT_CALL(orderRepo, read(30)).WillOnce(Return(o));
    EXPECT_CALL(orderRepo, update(_))
        .WillOnce(Invoke([](const Order& updated) {
            EXPECT_EQ(updated.status, OrderStatus::RELEASE);
            return true;
        }));

    OrderController ctrl(orderRepo, sampleRepo, jobRepo);
    ctrl.processRelease(30);
}
