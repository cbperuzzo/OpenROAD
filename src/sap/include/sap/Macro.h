#pragma once

#include "odb/db.h"
#include "Pin.h"
#include <unordered_set>
#include <vector>

namespace sap {

class Macro {
public:
    Macro(odb::dbInst* inst);
    ~Macro();

    odb::dbInst* getInst();

    bool isFixed() const;

    void xMin(int x);
    void yMin(int y);

    void xMax(int x);
    void yMax(int y);

    int xMin();
    int yMin();

    int xMax();
    int yMax();

    int dx();
    int dy();

    int cx();
    int cy();

    odb::dbSet<odb::dbITerm> listITerms();

    void updateInst();

    void updateInstWithOffset(int dx, int dy);

    void addPin(Pin* pin);

    void applyHalo(int halo_width, int halo_heigh );

    void createHaloBlockage(odb::dbBlock* chip_block);

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
