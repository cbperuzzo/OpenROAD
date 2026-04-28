#pragma once

#include "odb/db.h"
#include "Macro.h"

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

    void addMacro(Macro& macro);

    odb::dbNet* getDbNet();

    std::vector<Macro> getMacros();

private:

    std::vector<Macro> macros_;
    odb::dbNet* net_;
    int lx_;
    int ux_;
    int ly_;
    int uy_;

};

}