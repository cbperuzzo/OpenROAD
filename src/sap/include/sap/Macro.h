#pragma once

#include "odb/db.h"
#include "Pin.h"
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

    std::vector<odb::dbITerm*> listITerms();

    void update_inst();

    void addPin(Pin* pin);

    std::vector<Pin*> getPins();
    
private:
    odb::dbInst* inst_;
    std::vector<Pin*> pins_;
    int xMin_;
    int yMin_;
    int dx_;
    int dy_;
};

}
