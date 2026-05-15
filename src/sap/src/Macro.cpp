#include "sap/Macro.h"

namespace sap {

Macro::Macro(odb::dbInst* inst) :
    inst_(inst),
    dx_(inst->getBBox()->getDX()),
    dy_(inst->getBBox()->getDY()),
    xMin_(inst->getBBox()->xMin()),
    yMin_(inst->getBBox()->yMin())
{}

Macro::~Macro() {}

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
    return (dx_ + xMin_) / 2;
}
int Macro::cy(){
    return (dy_ + yMin_) / 2;
}

std::vector<Pin*> Macro::getPins(){
    return pins_;
}

void Macro::update_inst(){
    inst_->setOrigin(xMin_,yMin_);
    inst_->setPlacementStatus(odb::dbPlacementStatus::PLACED);
}

}
