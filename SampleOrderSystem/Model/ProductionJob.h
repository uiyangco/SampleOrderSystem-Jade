#pragma once
#include <string>
#include "../json/JsonValue.h"

enum class JobStatus {
    WAITING,
    RUNNING,
    DONE
};

inline std::string jobStatusToString(JobStatus s) {
    switch (s) {
    case JobStatus::WAITING: return "WAITING";
    case JobStatus::RUNNING: return "RUNNING";
    case JobStatus::DONE:    return "DONE";
    }
    return "WAITING";
}

inline JobStatus jobStatusFromString(const std::string& s) {
    if (s == "RUNNING") return JobStatus::RUNNING;
    if (s == "DONE")    return JobStatus::DONE;
    return JobStatus::WAITING;
}

struct ProductionJob {
    int          id = 0;
    int          orderId = 0;
    int          sampleId = 0;
    int          shortage = 0;
    int          targetQty = 0;
    int          producedQty = 0;
    int          totalMinutes = 0;
    int64_t      startedAtMs = 0;   // Unix ms, 0 = not started
    std::wstring startedAt;         // 표시용
    JobStatus    status = JobStatus::WAITING;

    int  getId()        const { return id; }
    void setId(int i)         { id = i; }

    JsonValue          toJson()                        const;
    static ProductionJob fromJson(const JsonValue& j);
};
