#include "ProductionThread.h"
#include "../Utils.h"
#include <algorithm>
#include <chrono>

void ProductionThread::start() {
    running_.store(true);
    thread_ = std::thread(&ProductionThread::loop, this);
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
        double elapsedSec = (Utils::nowMs() - runningIt->startedAtMs) / 1000.0;
        double elapsedMin = elapsedSec / timeScale_;
        if (elapsedMin >= static_cast<double>(runningIt->totalMinutes)) {
            completeJob(*runningIt);
            auto updated = jobRepo_.readAll();
            startNextWaiting(updated);
        }
    } else {
        startNextWaiting(jobs);
    }
}

void ProductionThread::completeJob(const ProductionJob& job) {
    auto sampleOpt = sampleRepo_.read(job.sampleId);
    if (sampleOpt) {
        Sample s = *sampleOpt;
        s.stock += job.targetQty;
        sampleRepo_.update(s);
    }
    auto orderOpt = orderRepo_.read(job.orderId);
    if (orderOpt) {
        Order o  = *orderOpt;
        o.status = OrderStatus::CONFIRMED;
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
