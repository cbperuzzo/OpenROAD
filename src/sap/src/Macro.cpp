#include "sap/Macro.h"

namespace sap {

Macro::Macro(odb::dbInst* inst) :
    inst_(inst),
    dx_(inst->getBBox()->getDX()),
    dy_(inst->getBBox()->getDY()),
    xMin_(inst->getBBox()->xMin()),
    yMin_(inst->getBBox()->yMin()),
    dx_to_macro(0),
    dy_to_macro(0)
{
    

}

Macro::~Macro() {}

void Macro::applyHalo(int halo_width, int halo_heigh){

    dx_to_macro = halo_width;
    dy_to_macro = halo_heigh;
    dx_ = dx_ + (halo_width * 2);    
    dy_ = dy_ + (halo_heigh * 2); 

    for(auto pin : pins_){
        pin->xMove(halo_width);
        pin->yMove(halo_heigh);
    }
    
}

odb::dbInst* Macro::getInst(){ 
    return inst_;
 }

bool Macro::isFixed() const {
    return inst_->isFixed();
}

void Macro::addPin(Pin* pin){
    pins_.push_back(pin);
}

odb::dbSet<odb::dbITerm> Macro::listITerms(){
    return inst_->getITerms();
}

void Macro::xMin(int x){
    int d = x - xMin_;
    xMin_ = x;

    for(auto pin : pins_){
        pin->xMove(d);
    }
}
void Macro::yMin(int y){
    int d = y - yMin_;
    yMin_ = y;

    for(auto pin : pins_){
        pin->yMove(d);
    }
}

int Macro::xMin(){
    return xMin_;
}
int Macro::yMin(){
    return yMin_;
}

int Macro::dx(){
    return dx_;
}
int Macro::dy(){
    return dy_;
}

int Macro::cx(){
    return (dx_ / 2) + xMin_;
}
int Macro::cy(){
    return (dy_ / 2) + yMin_;
}

std::vector<Pin*> Macro::getPins(){
    return pins_;
}

void Macro::updateInst(){
    inst_->setOrigin(xMin_ + dx_to_macro, yMin_ + dy_to_macro);
    inst_->setPlacementStatus(odb::dbPlacementStatus::PLACED);

}

void Macro::updateInstWithOffset(int dx, int dy){
    inst_->setOrigin(xMin_ + dx_to_macro + dx, yMin_ + dy_to_macro + dy);
    inst_->setPlacementStatus(odb::dbPlacementStatus::PLACED);
}

void Macro::createHaloBlockage(odb::dbBlock* chip_block){
    odb::dbBlockage::create(chip_block, xMin_, yMin_, xMin_ + dx_, yMin_ + dy_);
}

}
