#pragma once

#include "odb/db.h"
#include "Pin.h"

namespace sap
{

class Net
{

public:

    Net(odb::dbNet* net);
    ~Net();
    void update();
    int getHpwl();

    bool operator==( Net& other) const;

    void addDynamicPin(Pin* pin);
    void addStaticPin(Pin* pin);

    void updateStaticBBox();

    odb::dbNet* getDbNet();

    std::vector<Pin*> getDynamicPins();
    std::vector<Pin*> getStaticPins();

    bool containsDynamicPin(odb::dbITerm* i);

private:

    std::vector<Pin*> static_pins_;
    std::vector<Pin*> dynamic_pins_;
    odb::dbNet* net_;
    int lx_;
    int ux_;
    int ly_;
    int uy_;

    int s_lx_;
    int s_ux_;
    int s_ly_;
    int s_uy_;

};

}