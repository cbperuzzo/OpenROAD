#pragma once

#include "odb/db.h"
#include <unordered_set>

namespace sap {

class Macro {
public:
    Macro(odb::dbInst* inst);
    ~Macro();

    odb::dbInst* getInst() const { return inst_; }

    bool isFixed() const;

    void x(int x);
    void y(int y);

    int x();
    int y();

    int dx();
    int dy();

    int cx();
    int cy();

    std::unordered_set<odb::dbNet*> listNets();

    void update_inst();
    odb::Rect getBBox() const;

private:
    odb::dbInst* inst_;
    int x_;
    int y_;
    const int dx_;
    const int dy_;
};

}
