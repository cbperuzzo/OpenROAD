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

    odb::dbSet<odb::dbITerm> listITerms();

    void update_inst();

    void update_inst_with_offset(int dx, int dy);

    void addPin(Pin* pin);

    void applyHalo(int halo_width, int halo_heigh );

    std::vector<Pin*> getPins();
    
private:

    odb::dbInst* inst_;
    std::vector<Pin*> pins_;
    int xMin_;
    int yMin_;
    int dx_;
    int dy_;
    int dx_to_macro;
    int dy_to_macro;

    static int halo_width;
    static int halo_heigh;

};

}
