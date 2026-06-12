#include "ProductionJob.h"
#include "../Utils.h"

JsonValue ProductionJob::toJson() const {
    JsonValue j = JsonValue::makeObject();
    j["id"]           = JsonValue(id);
    j["orderId"]      = JsonValue(orderId);
    j["sampleId"]     = JsonValue(sampleId);
    j["shortage"]        = JsonValue(shortage);
    j["stockAtApproval"] = JsonValue(stockAtApproval);
    j["targetQty"]       = JsonValue(targetQty);
    j["producedQty"]  = JsonValue(producedQty);
    j["totalMinutes"] = JsonValue(totalMinutes);
    j["startedAtMs"]  = JsonValue(static_cast<int>(startedAtMs));
    j["startedAt"]    = JsonValue(Utils::wstringToUtf8(startedAt));
    j["status"]       = JsonValue(jobStatusToString(status));
    return j;
}

ProductionJob ProductionJob::fromJson(const JsonValue& j) {
    ProductionJob job;
    job.id           = j["id"].getInt();
    job.orderId      = j["orderId"].getInt();
    job.sampleId     = j["sampleId"].getInt();
    job.shortage        = j.contains("shortage")        ? j["shortage"].getInt()        : 0;
    job.stockAtApproval = j.contains("stockAtApproval") ? j["stockAtApproval"].getInt() : 0;
    job.targetQty       = j["targetQty"].getInt();
    job.producedQty  = j["producedQty"].getInt();
    job.totalMinutes = j["totalMinutes"].getInt();
    job.startedAtMs  = j.contains("startedAtMs") ? static_cast<int64_t>(j["startedAtMs"].getInt()) : 0;
    job.startedAt    = Utils::utf8ToWstring(j["startedAt"].getString());
    job.status       = jobStatusFromString(j["status"].getString());
    return job;
}
