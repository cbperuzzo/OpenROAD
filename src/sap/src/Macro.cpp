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

std::vector<odb::dbITerm*> Macro::listITerms(){
    std::vector<odb::dbITerm*> i_terms;
    for(auto i : inst_->getITerms()){
        i_terms.push_back(i);
    }
    return i_terms;
}

void Macro::xMin(int x){
    xMin_ = x;
}
void Macro::yMin(int y){
    yMin_ = y;
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
