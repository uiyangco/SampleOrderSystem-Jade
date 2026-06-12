#include "ProductionThread.h"
#include "ProductionCalc.h"
#include "../Utils.h"
#include <algorithm>
#include <chrono>

void ProductionThread::start() {
    running_.store(true);
    adjustStartedAtForResume();
    thread_ = std::thread(&ProductionThread::loop, this);
}

void ProductionThread::adjustStartedAtForResume() {
    auto jobs = jobRepo_.readAll();
    auto runningIt = std::find_if(jobs.begin(), jobs.end(),
        [](const ProductionJob& j) -> bool { return j.status == JobStatus::RUNNING; });
    if (runningIt == jobs.end()) return;

    ProductionJob job = *runningIt;
    if (job.targetQty <= 0 || job.producedQty >= job.targetQty) return;

    // 이미 생산된 수량에 해당하는 경과 시간만큼 startedAtMs를 현재 기준으로 재계산
    double perUnitMin      = static_cast<double>(job.totalMinutes) / job.targetQty;
    double alreadyMin      = job.producedQty * perUnitMin;
    int64_t alreadyMs      = static_cast<int64_t>(alreadyMin * timeScale_ * 1000.0);
    job.startedAtMs        = Utils::nowMs() - alreadyMs;
    jobRepo_.update(job);
}

void ProductionThread::stop() {
    running_.store(false);
    if (thread_.joinable()) thread_.join();
}

void ProductionThread::loop() {
    while (running_.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        tick();
    }
}

void ProductionThread::tick() {
    std::lock_guard<std::mutex> lock(mutex_);

    auto jobs = jobRepo_.readAll();

    auto runningIt = std::find_if(jobs.begin(), jobs.end(),
        [](const ProductionJob& j) -> bool { return j.status == JobStatus::RUNNING; });

    if (runningIt != jobs.end()) {
        ProductionJob job = *runningIt;

        double elapsedSec = (Utils::nowMs() - job.startedAtMs) / 1000.0;
        double elapsedMin = elapsedSec / timeScale_;

        // 단위당 생산 시간 (분)
        double perUnitMin = job.targetQty > 0
            ? static_cast<double>(job.totalMinutes) / job.targetQty
            : 1.0;

        int newProduced = calcProducedQty(elapsedMin, perUnitMin, job.targetQty);
        int delta = newProduced - job.producedQty;

        if (delta > 0) {
            // 새로 생산된 수량만큼 즉시 재고에 반영
            auto sampleOpt = sampleRepo_.read(job.sampleId);
            if (sampleOpt) {
                Sample s = *sampleOpt;
                s.stock += delta;
                sampleRepo_.update(s);
            }
            job.producedQty = newProduced;
            jobRepo_.update(job);
        }

        if (newProduced >= job.targetQty) {
            completeJob(job);
            auto updated = jobRepo_.readAll();
            startNextWaiting(updated);
        }
    } else {
        startNextWaiting(jobs);
    }
}

void ProductionThread::completeJob(const ProductionJob& job) {
    // 재고는 tick()에서 이미 증분 반영됨 — 여기서는 주문 상태만 CONFIRMED 처리
    auto orderOpt = orderRepo_.read(job.orderId);
    if (orderOpt) {
        Order o = *orderOpt;
        o.status    = OrderStatus::CONFIRMED;
        o.updatedAt = Utils::nowWstring();
        orderRepo_.update(o);
    }
    ProductionJob done = job;
    done.status = JobStatus::DONE;
    jobRepo_.update(done);
}

void ProductionThread::startNextWaiting(const std::vector<ProductionJob>& jobs) {
    auto waitingIt = std::find_if(jobs.begin(), jobs.end(),
        [](const ProductionJob& j) -> bool { return j.status == JobStatus::WAITING; });
    if (waitingIt == jobs.end()) return;

    ProductionJob next = *waitingIt;
    next.status      = JobStatus::RUNNING;
    next.startedAtMs = Utils::nowMs();
    next.startedAt   = Utils::nowWstring();
    jobRepo_.update(next);
}
