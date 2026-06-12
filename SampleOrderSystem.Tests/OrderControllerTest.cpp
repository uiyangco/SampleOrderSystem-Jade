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

// ── approve: 재고 충분해도 항상 PRODUCING + Job 생성 ─────────────────────────

TEST(OrderControllerTest, Approve_AlwaysCreatesJobAndSetsProducing) {
    MockOrderRepository         orderRepo;
    MockSampleRepository        sampleRepo;
    MockProductionJobRepository jobRepo;

    Order o; o.setId(10); o.sampleId = 2; o.quantity = 5;
    o.status = OrderStatus::RESERVED;

    // 재고가 충분해도(10 >= 5) 이제는 항상 PRODUCING
    Sample s; s.setId(2); s.stock = 10; s.yield = 0.9; s.avgProductionTime = 5;

    EXPECT_CALL(orderRepo, read(10)).WillOnce(Return(o));
    EXPECT_CALL(sampleRepo, read(2)).WillOnce(Return(s));
    // 재고 차감 없음 — sampleRepo.update 호출되지 않아야 한다
    EXPECT_CALL(sampleRepo, update(_)).Times(0);
    EXPECT_CALL(jobRepo, create(_))
        .WillOnce(Invoke([](ProductionJob& job) {
            EXPECT_EQ(job.targetQty,    5);    // 주문 수량 그대로
            EXPECT_EQ(job.totalMinutes, 25);   // avgProdTime(5) × qty(5)
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

// ── approve: 재고 부족도 동일하게 PRODUCING + Job 생성 ───────────────────────

TEST(OrderControllerTest, Approve_WhenStockInsufficient_AlsoSetsProducingAndCreatesJob) {
    MockOrderRepository         orderRepo;
    MockSampleRepository        sampleRepo;
    MockProductionJobRepository jobRepo;

    Order o; o.setId(11); o.sampleId = 3; o.quantity = 10;
    o.status = OrderStatus::RESERVED;

    Sample s; s.setId(3); s.stock = 3; s.yield = 0.8; s.avgProductionTime = 6;

    EXPECT_CALL(orderRepo, read(11)).WillOnce(Return(o));
    EXPECT_CALL(sampleRepo, read(3)).WillOnce(Return(s));
    EXPECT_CALL(sampleRepo, update(_)).Times(0);
    EXPECT_CALL(jobRepo, create(_))
        .WillOnce(Invoke([](ProductionJob& job) {
            EXPECT_EQ(job.targetQty,    10);   // 주문 수량 그대로 (yield 계산 없음)
            EXPECT_EQ(job.totalMinutes, 60);   // avgProdTime(6) × qty(10)
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

// ── processRelease: 재고 차감 ─────────────────────────────────────────────────

TEST(OrderControllerTest, ProcessRelease_DeductsStockAndSetsRelease) {
    MockOrderRepository         orderRepo;
    MockSampleRepository        sampleRepo;
    MockProductionJobRepository jobRepo;

    Order o; o.setId(30); o.sampleId = 5; o.quantity = 5;
    o.status = OrderStatus::CONFIRMED;

    Sample s; s.setId(5); s.stock = 10;

    EXPECT_CALL(orderRepo, read(30)).WillOnce(Return(o));
    EXPECT_CALL(sampleRepo, read(5)).WillOnce(Return(s));
    EXPECT_CALL(sampleRepo, update(_))
        .WillOnce(Invoke([](const Sample& updated) {
            EXPECT_EQ(updated.stock, 5);  // 10 - 5
            return true;
        }));
    EXPECT_CALL(orderRepo, update(_))
        .WillOnce(Invoke([](const Order& updated) {
            EXPECT_EQ(updated.status, OrderStatus::RELEASE);
            return true;
        }));

    OrderController ctrl(orderRepo, sampleRepo, jobRepo);
    bool result = ctrl.processRelease(30);
    EXPECT_TRUE(result);
}

// ── processRelease: 재고 부족 시 실패 ─────────────────────────────────────────

TEST(OrderControllerTest, ProcessRelease_WhenInsufficientStock_ReturnsFalse) {
    MockOrderRepository         orderRepo;
    MockSampleRepository        sampleRepo;
    MockProductionJobRepository jobRepo;

    Order o; o.setId(31); o.sampleId = 6; o.quantity = 10;
    o.status = OrderStatus::CONFIRMED;

    Sample s; s.setId(6); s.stock = 3;  // 재고 3 < 주문 10

    EXPECT_CALL(orderRepo, read(31)).WillOnce(Return(o));
    EXPECT_CALL(sampleRepo, read(6)).WillOnce(Return(s));
    EXPECT_CALL(sampleRepo, update(_)).Times(0);  // 재고 변경 없어야 함
    EXPECT_CALL(orderRepo, update(_)).Times(0);   // 상태 변경 없어야 함

    OrderController ctrl(orderRepo, sampleRepo, jobRepo);
    bool result = ctrl.processRelease(31);
    EXPECT_FALSE(result);
}
