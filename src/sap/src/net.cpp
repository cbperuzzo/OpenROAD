#include "sap/net.h"
#include "odb/db.h"

namespace sap
{

Net::Net(odb::dbNet* net){
    net_ = net;
    update();
}

Net::~Net(){}

void Net::update(){
    lx_ = INT_MAX;
    ly_ = INT_MAX;
    ux_ = INT_MIN;
    uy_ = INT_MIN;
    for (auto macro : macros_) {
        lx_ = std::min(macro.cx(), lx_);
        ly_ = std::min(macro.cy(), ly_);
        ux_ = std::max(macro.cx(), ux_);
        uy_ = std::max(macro.cy(), uy_);
    }

}

int Net::getHpwl(){
    return (ux_ - lx_) + (uy_ - ly_);
}

bool Net::operator==( Net& other) const{
    return (net_ == other.net_);
}

void Net::addMacro(Macro& macro){
    macros_.push_back(macro);
}

odb::dbNet* Net::getDbNet(){
    return net_;
}

std::vector<Macro> Net::getMacros(){
    return macros_;
}
    
}