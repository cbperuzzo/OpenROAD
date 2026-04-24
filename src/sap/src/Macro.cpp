#include "sap/Macro.h"

namespace sap {

Macro::Macro(odb::dbInst* inst) : inst_(inst), dx_(inst->getBBox()->getDX()),dy_(inst->getBBox()->getDY()) {}

Macro::~Macro() {}

bool Macro::isFixed() const {
    return inst_->isFixed();
}

odb::Rect Macro::getBBox() const {
    return inst_->getBBox()->getBox();
}

std::unordered_set<odb::dbNet*> Macro::listNets(){
    std::unordered_set<odb::dbNet*> nets;
    for(auto i : inst_->getITerms()){
        auto net = i->getNet();
        if (net != nullptr)
            nets.insert(net);
    }
    return nets;
}

}
