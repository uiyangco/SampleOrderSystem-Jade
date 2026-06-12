#pragma once
#include <gmock/gmock.h>
#include "Model/Repository/IRepository.h"
#include "Model/ProductionJob.h"

class MockProductionJobRepository : public IRepository<ProductionJob> {
public:
    MOCK_METHOD(bool,                    create,  (ProductionJob&),         (override));
    MOCK_METHOD(std::optional<ProductionJob>, read, (int),                  (override));
    MOCK_METHOD(std::vector<ProductionJob>,   readAll, (),                  (override));
    MOCK_METHOD(bool,                    update,  (const ProductionJob&),   (override));
    MOCK_METHOD(bool,                    remove,  (int),                    (override));
};
