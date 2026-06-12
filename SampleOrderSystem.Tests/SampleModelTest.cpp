#include <gtest/gtest.h>
#include "Model/Sample.h"
#include "Model/Order.h"
#include "Model/ProductionJob.h"

// ── Sample ───────────────────────────────────────────────────────────────────

TEST(SampleModelTest, ToJson_ContainsAllFields) {
    Sample s;
    s.id                = 1;
    s.name              = L"시료A";
    s.avgProductionTime = 10;
    s.yield             = 0.85;
    s.stock             = 50;
    s.createdAt         = L"2026-01-01 09:00:00";

    JsonValue j = s.toJson();
    EXPECT_EQ(j["id"].getInt(),                 1);
    EXPECT_EQ(j["avgProductionTime"].getInt(),  10);
    EXPECT_DOUBLE_EQ(j["yield"].getNumber(),    0.85);
    EXPECT_EQ(j["stock"].getInt(),              50);
}

TEST(SampleModelTest, FromJson_RestoresAllFields) {
    JsonValue j = JsonValue::makeObject();
    j["id"]                 = JsonValue(2);
    j["name"]               = JsonValue(std::string("SampleB"));
    j["avgProductionTime"]  = JsonValue(5);
    j["yield"]              = JsonValue(0.9);
    j["stock"]              = JsonValue(100);
    j["createdAt"]          = JsonValue(std::string("2026-01-02 10:00:00"));

    Sample s = Sample::fromJson(j);
    EXPECT_EQ(s.id,                2);
    EXPECT_EQ(s.avgProductionTime, 5);
    EXPECT_DOUBLE_EQ(s.yield,      0.9);
    EXPECT_EQ(s.stock,             100);
}

TEST(SampleModelTest, RoundTrip) {
    Sample original;
    original.id                = 3;
    original.name              = L"RoundTrip";
    original.avgProductionTime = 7;
    original.yield             = 0.75;
    original.stock             = 20;
    original.createdAt         = L"2026-06-01 08:00:00";

    Sample restored = Sample::fromJson(original.toJson());
    EXPECT_EQ(restored.id,                original.id);
    EXPECT_EQ(restored.avgProductionTime, original.avgProductionTime);
    EXPECT_DOUBLE_EQ(restored.yield,      original.yield);
    EXPECT_EQ(restored.stock,             original.stock);
}

// ── Order ────────────────────────────────────────────────────────────────────

TEST(OrderModelTest, ToJson_ContainsAllFields) {
    Order o;
    o.id           = 1;
    o.sampleId     = 2;
    o.customerName = L"홍길동";
    o.quantity     = 10;
    o.status       = OrderStatus::RESERVED;
    o.createdAt    = L"2026-01-01 09:00:00";

    JsonValue j = o.toJson();
    EXPECT_EQ(j["id"].getInt(),       1);
    EXPECT_EQ(j["sampleId"].getInt(), 2);
    EXPECT_EQ(j["quantity"].getInt(), 10);
    EXPECT_EQ(j["status"].getString(), "RESERVED");
}

TEST(OrderModelTest, FromJson_RestoresStatus_Confirmed) {
    JsonValue j = JsonValue::makeObject();
    j["id"]           = JsonValue(5);
    j["sampleId"]     = JsonValue(1);
    j["customerName"] = JsonValue(std::string("Kim"));
    j["quantity"]     = JsonValue(3);
    j["status"]       = JsonValue(std::string("CONFIRMED"));
    j["createdAt"]    = JsonValue(std::string("2026-01-01 00:00:00"));
    j["updatedAt"]    = JsonValue(std::string("2026-01-01 01:00:00"));

    Order o = Order::fromJson(j);
    EXPECT_EQ(o.id,     5);
    EXPECT_EQ(o.status, OrderStatus::CONFIRMED);
}

TEST(OrderModelTest, RoundTrip_ProducingStatus) {
    Order original;
    original.id     = 7;
    original.sampleId = 3;
    original.quantity = 5;
    original.status   = OrderStatus::PRODUCING;

    Order restored = Order::fromJson(original.toJson());
    EXPECT_EQ(restored.status, OrderStatus::PRODUCING);
}

// ── ProductionJob ─────────────────────────────────────────────────────────────

TEST(ProductionJobModelTest, ToJson_ContainsAllFields) {
    ProductionJob job;
    job.id           = 1;
    job.orderId      = 10;
    job.sampleId     = 2;
    job.shortage     = 7;
    job.targetQty    = 15;
    job.producedQty  = 0;
    job.totalMinutes = 150;
    job.status       = JobStatus::WAITING;

    JsonValue j = job.toJson();
    EXPECT_EQ(j["id"].getInt(),           1);
    EXPECT_EQ(j["orderId"].getInt(),      10);
    EXPECT_EQ(j["shortage"].getInt(),     7);
    EXPECT_EQ(j["targetQty"].getInt(),    15);
    EXPECT_EQ(j["totalMinutes"].getInt(), 150);
    EXPECT_EQ(j["status"].getString(),    "WAITING");
}

TEST(ProductionJobModelTest, FromJson_RestoresRunningStatus) {
    JsonValue j = JsonValue::makeObject();
    j["id"]           = JsonValue(2);
    j["orderId"]      = JsonValue(20);
    j["sampleId"]     = JsonValue(3);
    j["shortage"]     = JsonValue(5);
    j["targetQty"]    = JsonValue(8);
    j["producedQty"]  = JsonValue(3);
    j["totalMinutes"] = JsonValue(80);
    j["startedAtMs"]  = JsonValue(0);
    j["startedAt"]    = JsonValue(std::string(""));
    j["status"]       = JsonValue(std::string("RUNNING"));

    ProductionJob job = ProductionJob::fromJson(j);
    EXPECT_EQ(job.status,   JobStatus::RUNNING);
    EXPECT_EQ(job.orderId,  20);
    EXPECT_EQ(job.shortage, 5);
}
