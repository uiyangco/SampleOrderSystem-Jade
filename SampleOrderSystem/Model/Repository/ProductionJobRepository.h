#pragma once
#include "JsonRepository.h"
#include "../ProductionJob.h"

class ProductionJobRepository : public JsonRepository<ProductionJob> {
public:
    explicit ProductionJobRepository(const std::string& filePath)
        : JsonRepository<ProductionJob>(filePath) {}
};
