#pragma once
#include <gmock/gmock.h>
#include "Model/Repository/IRepository.h"
#include "Model/Sample.h"

class MockSampleRepository : public IRepository<Sample> {
public:
    MOCK_METHOD(bool,             create, (Sample&),              (override));
    MOCK_METHOD(std::optional<Sample>, read, (int),              (override));
    MOCK_METHOD(std::vector<Sample>,   readAll, (),               (override));
    MOCK_METHOD(bool,             update, (const Sample&),        (override));
    MOCK_METHOD(bool,             remove, (int),                  (override));
};
