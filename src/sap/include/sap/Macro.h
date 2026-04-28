#pragma once

#include "odb/db.h"
#include <unordered_set>

namespace sap {

class Macro {
public:
    Macro(odb::dbInst* inst);
    ~Macro();

    odb::dbInst* getInst();

    bool isFixed() const;

    void xMin(int x);
    void yMin(int y);

    int xMin();
    int yMin();

    int dx();
    int dy();

    int cx();
    int cy();

    std::unordered_set<odb::dbNet*> listDbNets();

    void update_inst();

private:
    odb::dbInst* inst_;
    int xMin_;
    int yMin_;
    int dx_;
    int dy_;
};

}
