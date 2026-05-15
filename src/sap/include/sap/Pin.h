#pragma once

#include "odb/db.h"

namespace sap {
    
class Pin{

public:
    Pin(odb::dbITerm* i_term);

    int xMin();
    int yMin();

    void xMin(int x);
    void yMin(int y);

    void xMove(int d);
    void yMove(int d);

    int dx();
    int dy();

    void dx(int dx);
    void dy(int dy);

    odb::dbITerm* getITerm();

private:
    int xMin_;
    int yMin_;
    int dx_;
    int dy_;
    odb::dbITerm* i_term_;
};

}