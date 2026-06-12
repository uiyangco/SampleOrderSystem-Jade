#pragma once
#include "../Model/Repository/IRepository.h"
#include "../Model/ProductionJob.h"
#include "../Model/Order.h"
#include "../Model/Sample.h"
#include "../View/ProductionView.h"
#include "../View/ConsoleUI.h"

class ProductionController {
public:
    ProductionController(
        IRepository<ProductionJob>& jobRepo,
        IRepository<Order>&         orderRepo,
        IRepository<Sample>&        sampleRepo,
        ProductionView&             view,
        ConsoleUI&                  ui,
        double                      timeScaleSecPerMin)
        : jobRepo_(jobRepo), orderRepo_(orderRepo), sampleRepo_(sampleRepo)
        , view_(view), ui_(ui), timeScale_(timeScaleSecPerMin) {}

    void run();

private:
    IRepository<ProductionJob>& jobRepo_;
    IRepository<Order>&         orderRepo_;
    IRepository<Sample>&        sampleRepo_;
    ProductionView&             view_;
    ConsoleUI&                  ui_;
    double                      timeScale_;
};
