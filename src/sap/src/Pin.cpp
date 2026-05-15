
#include "sap/Pin.h"
#include "odb/db.h"

namespace sap {

Pin::Pin(odb::dbITerm* i_term){
    i_term_ = i_term;
    auto box = i_term->getBBox();
    xMin_ = box.xMin();
    yMin_ = box.yMin();
    dx_ = box.dx();
    dy_ = box.dy();
}

    int Pin::xMin(){
        return xMin_;
    }
    int Pin::yMin(){
        return yMin_;
    }

    void Pin::xMin(int x){
        xMin_ = x;
    }
    void Pin::yMin(int y){
        yMin_ = y;
    }

    void Pin::xMove(int d){
        xMin_ = xMin_ + d;
    }

    void Pin::yMove(int d){
        yMin_ = yMin_ + d;
    }

    int Pin::dx(){
        return dx_;
    }
    int Pin::dy(){
        return dy_;
    }

    void Pin::dx(int dx){
        dx_ = dx;
    }
    void Pin::dy(int dy){
        dy_ = dy;
    }

    odb::dbITerm* Pin::getITerm(){
        return i_term_;
    }
}