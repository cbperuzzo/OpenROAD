#pragma once

#include "odb/db.h"

namespace sap
{

class Net
{

public:

    Net(odb::dbNet* net);
    ~Net();
    void update();
    int getHpwl();

private:

    odb::dbNet* net_;
    int lx_;
    int ux_;
    int ly_;
    int uy_;

};

}