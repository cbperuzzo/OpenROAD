#include "sap/SAplace.h"

#include "sap/AdjacencyGraphBuilder.h"
#include "sta/StaMain.hh"
#include "odb/db.h"
#include "utl/Logger.h"
#include <map>

namespace sap{

SAplace::SAplace(odb::dbDatabase* db, sta::dbSta* sta, utl::Logger* log):
  pos_seq_(),
  neg_seq_(),
  pos_seq_backup_(),
  neg_seq_backup_(),
  best_pos_seq_(),
  best_neg_seq_(),
  macros_(),
  nets_(),
  max_h_(0),
  max_w_(0)
{
  db_ = db;
  sta_ = sta;
  log_ = log;
  generator_ = std::default_random_engine(44);
  prob_ = std::uniform_real_distribution<float>(0,1);
  move_ = std::uniform_int_distribution<int>(0,1);

}

SAplace::~SAplace(){}

void SAplace::init(int iterations_per_T, double initial_T, double alpha, int halo_width, int halo_height) {
  macros_.clear();
  nets_.clear();
  pins_.clear();

  max_h_ = db_->getChip()->getBlock()->getCoreArea().dx();
  max_w_ = db_->getChip()->getBlock()->getCoreArea().dy();

  initializeProxies();
  buildAdjacencyGraph();

  for(auto& macro : macros_)
    macro.applyHalo(halo_width, halo_height);
  
  for(auto& net : nets_)
    net.updateStaticBBox();
  
  pos_seq_.resize(macros_.size());
  neg_seq_.resize(macros_.size());

  for(auto& macro : macros_){
    macro.updateInst();
    macro.createHaloBlockage(db_->getChip()->getBlock());
  }
  
}

void SAplace::initializeProxies(){
  for(auto inst : db_->getChip()->getBlock()->getInsts()){
    if (inst->isBlock())
      macros_.push_back(Macro(inst));
  }

  std::map<uint32_t,Net*> net_map;
  std::map<Net*,std::unordered_set<Macro*>> net_macros_map;  
  
  for(auto& macro : macros_ ){
    for(auto i : macro.listITerms()){
  
      auto db_net = i->getNet();
      if (db_net == nullptr){
        log_->report("netless pin, somehow");
        continue;
      }

      pins_.push_back(Pin(i));
      macro.addPin(&pins_.back());
      
      if (net_map.find(db_net->getId()) != net_map.end()){
        net_map.at(db_net->getId())->addDynamicPin(&pins_.back());
      }
      else{
        nets_.push_back(Net(db_net));
        nets_.back().addDynamicPin(&pins_.back());
        net_map[db_net->getId()] = &nets_.back();
        net_macros_map[&nets_.back()].insert(&macro);
      }
    }
  }

  for (auto& net : nets_){
    auto db_net = net.getDbNet();
    for(auto i : db_net->getITerms()){
      if(!net_map.at(db_net->getId())->containsDynamicPin(i)){
        pins_.push_back(Pin(i));
        net_map.at(db_net->getId())->addStaticPin(&pins_.back());
      }
    }
  }

  for(auto& pair : net_macros_map){
    if(pair.second.size() <= 1){
      shared_nets_[pair.first] = pair.second;
    }
  }

}

void SAplace::buildAdjacencyGraph(){
  AdjacencyGraphBuilder builder(sta_, db_, log_);
  AdjacencyMatrix matrix = builder.build(macros_);

}

std::vector<Net*> SAplace::findSharedNets(std::unordered_set<Macro*>& macros){
  std::vector<Net*> out_nets;
  for(auto& pair : shared_nets_ ){
    for (Macro* set_macro : macros){
      if(pair.second.contains(set_macro)){
        bool is_out = false;
        for(Macro* net_macros : pair.second){
          is_out |= !macros.contains(net_macros);
        }
        if(is_out){
          out_nets.push_back(pair.first);
        }
      }
    }
  }
}

}
