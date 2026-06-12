#pragma once
#include "../Model/Repository/IRepository.h"
#include "../Model/ProductionJob.h"
#include "../Model/Order.h"
#include "../Model/Sample.h"
#include <thread>
#include <atomic>
#include <mutex>

class ProductionThread {
public:
    ProductionThread(
        IRepository<ProductionJob>& jobRepo,
        IRepository<Order>&         orderRepo,
        IRepository<Sample>&        sampleRepo,
        double                      timeScaleSecPerMin = 5.0)
        : jobRepo_(jobRepo), orderRepo_(orderRepo), sampleRepo_(sampleRepo)
        , timeScale_(timeScaleSecPerMin), running_(false) {}

    ~ProductionThread() { stop(); }

    void start();
    void stop();

private:
    void loop();
    void tick();
    void completeJob(const ProductionJob& job);
    void startNextWaiting(const std::vector<ProductionJob>& jobs);

    IRepository<ProductionJob>& jobRepo_;
    IRepository<Order>&         orderRepo_;
    IRepository<Sample>&        sampleRepo_;
    double                      timeScale_;

    std::atomic<bool> running_;
    std::thread       thread_;
    std::mutex        mutex_;
};
