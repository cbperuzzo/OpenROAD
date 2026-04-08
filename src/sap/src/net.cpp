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
    for (odb::dbITerm* iTerm : net_->getITerms()) {
        odb::dbBox* box = iTerm->getInst()->getBBox();
        lx_ = std::min(box->xMin(), lx_);
        ly_ = std::min(box->yMin(), ly_);
        ux_ = std::max(box->xMax(), ux_);
        uy_ = std::max(box->yMax(), uy_);
    }
}

int Net::getHpwl(){
    return (ux_ - lx_) + (uy_ - ly_);
}
    
}