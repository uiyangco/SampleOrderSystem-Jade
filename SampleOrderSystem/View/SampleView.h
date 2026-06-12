#pragma once
#include "ConsoleUI.h"
#include "../Model/Sample.h"
#include <vector>

class SampleView {
public:
    explicit SampleView(ConsoleUI& ui) : ui_(ui) {}

    void showList(const std::vector<Sample>& samples);
    void showSearchResult(const std::vector<Sample>& results);
    Sample promptForSample();

private:
    ConsoleUI& ui_;
    void printRow(const Sample& s);
};
