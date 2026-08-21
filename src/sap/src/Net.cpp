#include "sap/Net.h"
#include "odb/db.h"

namespace sap
{

Net::Net(odb::dbNet* net){
    net_ = net;
}

Net::~Net(){}

void Net::update(){
    lx_ = s_lx_;
    ly_ = s_ly_;
    ux_ = s_ux_;
    uy_ = s_uy_;
    for (auto pin : dynamic_pins_) {
        lx_ = std::min(pin->xMin(), lx_);
        ly_ = std::min(pin->yMin(), ly_);
        ux_ = std::max(pin->xMin() + pin->dx(), ux_);
        uy_ = std::max(pin->yMin() + pin->dy(), uy_);
    }

}

int Net::getHpwl(){
    return (ux_ - lx_) + (uy_ - ly_);
}

bool Net::operator==( Net& other) const{
    return (net_ == other.net_);
}

void Net::addDynamicPin(Pin* pin){
    dynamic_pins_.push_back(pin);
}

void Net::addStaticPin(Pin* pin){
    static_pins_.push_back(pin);
}

odb::dbNet* Net::getDbNet(){
    return net_;
}

bool Net::containsDynamicPin(odb::dbITerm* i){
    for(auto pin : dynamic_pins_){
        if (pin->getITerm() == i)
            return true;
    }
    return false;
}

void Net::updateStaticBBox(){
    s_lx_ = INT_MAX;
    s_ly_ = INT_MAX;
    s_ux_ = INT_MIN;
    s_uy_ = INT_MIN;
    for (auto pin : static_pins_) {
        s_lx_ = std::min(pin->xMin(), s_lx_);
        s_ly_ = std::min(pin->yMin(), s_ly_);
        s_ux_ = std::max(pin->xMin() + pin->dx(), s_ux_);
        s_uy_ = std::max(pin->yMin() + pin->dy(), s_uy_);
    }
}

std::vector<Pin*> Net::getDynamicPins(){
    return dynamic_pins_;
}
std::vector<Pin*> Net::getStaticPins(){
    return static_pins_;
}

    
}