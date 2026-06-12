#pragma once
#include "JsonRepository.h"
#include "../Sample.h"

class SampleRepository : public JsonRepository<Sample> {
public:
    explicit SampleRepository(const std::string& filePath)
        : JsonRepository<Sample>(filePath) {}
};
