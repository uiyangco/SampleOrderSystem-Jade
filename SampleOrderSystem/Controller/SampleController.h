#pragma once
#include "../Model/Repository/IRepository.h"
#include "../Model/Sample.h"
#include "../View/SampleView.h"
#include "../View/ConsoleUI.h"

class SampleController {
public:
    SampleController(IRepository<Sample>& repo, SampleView& view, ConsoleUI& ui)
        : repo_(repo), view_(view), ui_(ui) {}

    void run();

private:
    void doRegister();
    void doList();
    void doSearch();

    IRepository<Sample>& repo_;
    SampleView&          view_;
    ConsoleUI&           ui_;
};
