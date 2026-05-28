#include "sap/SAplace.h"

#include "sta/StaMain.hh"
#include "odb/db.h"
#include <map>

namespace sap{

SAplace::SAplace(odb::dbDatabase* db, utl::Logger* log):
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
  log_ = log;
  generator_ = std::default_random_engine(44);
  prob_ = std::uniform_real_distribution<float>(0,1);
  move_ = std::uniform_int_distribution<int>(0,1);
  
}

SAplace::~SAplace(){

}

void SAplace::simulatedAnnealing(int n_threads, int iterations_per_T, double initial_T, double alpha, int halo_size) {
  macros_.clear();
  nets_.clear();
  pins_.clear();

  max_h_ = db_->getChip()->getBlock()->getCoreArea().dx();
  max_w_ = db_->getChip()->getBlock()->getCoreArea().dy();

  initializeProxies();

  for(auto& macro : macros_)
    macro.applyHalo(halo_size, halo_size);
  
  for(auto& net : nets_)
    net.updateStaticBBox();
  
  pos_seq_.resize(macros_.size());
  neg_seq_.resize(macros_.size());

  // random placement
  std::iota(pos_seq_.begin(),pos_seq_.end(),0);
  std::iota(neg_seq_.begin(),neg_seq_.end(),0);

  std::shuffle(pos_seq_.begin(),pos_seq_.end(),generator_);
  std::shuffle(neg_seq_.begin(),neg_seq_.end(),generator_);

  int core_origin_x = db_->getChip()->getBlock()->getCoreArea().xMin();
  int core_origin_y = db_->getChip()->getBlock()->getCoreArea().yMin();
  
  pack(core_origin_x, core_origin_y);
  float cost = calcCost();
  float best_cost = cost;
  best_pos_seq_ = pos_seq_;
  best_neg_seq_ = neg_seq_;
  float temperature = initial_T;
  int t_iterations = 0;
  while(temperature >= 1) {
    for(int iteration = 0; iteration < iterations_per_T; iteration++){

      saveState();
      perturb();
      pack(core_origin_x, core_origin_y);

      float new_cost = calcCost();
      float delta = new_cost - cost;
      
      if (delta <= 0){
        cost = new_cost;
        if(new_cost < best_cost){
          best_cost = new_cost;
          best_pos_seq_ = pos_seq_;
          best_neg_seq_ = neg_seq_;
        }
      }
      // migth accept, event though cost incressed
      else{
        float accept_chance = std::exp(-delta / temperature);
        float num = prob_(generator_);
        bool accepted = num < accept_chance;
        if (accepted){
          cost = new_cost;
        }
        else{
          restoreState();
        }
      }
      
    }
    temperature *= alpha;
    log_->report("it: {} cost: {}",t_iterations ,calcCost());
    t_iterations++;
  }

  pos_seq_ = best_pos_seq_;
  neg_seq_ = best_neg_seq_;

  pack(core_origin_x, core_origin_y);

  for(auto& macro : macros_){
    macro.updateInst();
    macro.createHaloBlockage(db_->getChip()->getBlock());
  }
  
  log_->report("-------------------------");
  log_->report("best cost (lowest): {} ",calcCost());
  log_->report("Finished simulated annealing");
  log_->report("-------------------------");

}

void SAplace::pack(int offset_x, int offset_y){
  
  std::vector<std::pair<int, int>> sequence_pair_pos(pos_seq_.size());

  for (int i = 0; i < pos_seq_.size(); i++) {
    sequence_pair_pos[pos_seq_[i]].first = i;
    sequence_pair_pos[neg_seq_[i]].second = i;
  }

  std::vector<int> accumulated_length(pos_seq_.size(), offset_x);
  for (int macro_id : pos_seq_) {
    const int neg_seq_pos = sequence_pair_pos[macro_id].second;

    Macro& macro = macros_[macro_id];
    int x = macro.xMin();

    if (!macro.isFixed()) {
      x = accumulated_length[neg_seq_pos];
      macro.xMin(x);
    }

    const int current_length = x + macro.dx();
  
    for (int j = neg_seq_pos; j < neg_seq_.size(); j++) {
      if (current_length > accumulated_length[j]) {
        accumulated_length[j] = current_length;
      } else {
        break;
      }
    }
  }

  width_ = accumulated_length[pos_seq_.size() - 1];

  // calulate Y position
  std::vector<int> reversed_pos_seq(pos_seq_.size());
  for (int i = 0; i < reversed_pos_seq.size(); i++) {
    reversed_pos_seq[i] = pos_seq_[reversed_pos_seq.size() - 1 - i];
  }

  for (int i = 0; i < pos_seq_.size(); i++) {
    sequence_pair_pos[reversed_pos_seq[i]].first = i;
    sequence_pair_pos[neg_seq_[i]].second = i;

    // This is actually the accumulated height, but we use the same vector
    // to avoid more allocation.
    accumulated_length[i] = offset_y;
  }

  for (int i = 0; i < pos_seq_.size(); i++) {
    const int macro_id = reversed_pos_seq[i];
    const int neg_seq_pos = sequence_pair_pos[macro_id].second;

    Macro& macro = macros_[macro_id];
    int y = macro.yMin();

    if (!macro.isFixed()) {
      y = accumulated_length[neg_seq_pos];
      macro.yMin(y);
    }

    const int current_height = y + macro.dy();

    for (int j = neg_seq_pos; j < neg_seq_.size(); j++) {
      if (current_height > accumulated_length[j]) {
        accumulated_length[j] = current_height;
      } else {
        break;
      }
    }
  }

  height_ = accumulated_length[pos_seq_.size() - 1];

  for (auto& net : nets_){
    net.update();
  }
  
}

void SAplace::initializeProxies(){
  for(auto inst : db_->getChip()->getBlock()->getInsts()){
    if (inst->isBlock())
      macros_.push_back(Macro(inst));
  }

  std::map<odb::dbNet*,Net*> net_map;  
  
  for(auto& macro : macros_ ){
    for(auto i : macro.listITerms()){
  
      auto db_net = i->getNet();
      if (db_net == nullptr){
        log_->report("netless pin, somehow");
        continue;
      }

      pins_.push_back(Pin(i));
      macro.addPin(&pins_.back());
      
      if (net_map.find(db_net) != net_map.end()){
        net_map.at(db_net)->addDynamicPin(&pins_.back());
      }
      else{
        nets_.push_back(Net(db_net));
        nets_.back().addDynamicPin(&pins_.back());
        net_map[db_net] = &nets_.back();
      }
    }
  }

  for (auto& net : nets_){
    auto db_net = net.getDbNet();
    for(auto i : db_net->getITerms()){
      if(!net_map.at(db_net)->containsDynamicPin(i)){
        pins_.push_back(Pin(i));
        net_map.at(db_net)->addStaticPin(&pins_.back());
      }
    }
  }

}

void SAplace::saveState(){
  pos_seq_backup_ = pos_seq_;
  neg_seq_backup_ = neg_seq_;
}

void SAplace::restoreState(){
  pos_seq_ = pos_seq_backup_;
  neg_seq_ = neg_seq_backup_;
}

float SAplace::calcCost(){  
  return (float) hpwl() +  penalties();
}

float SAplace::penalties(){
  float penalty = 0.0;
  
  if(height_ >= max_h_){
    penalty += 1e+7; 
  }
  
  if(width_ >= max_w_){
    penalty += 1e+7;
  }

  return penalty;
}


int SAplace::hpwl(){
  
  int acumulated_hpwl = 0;

  for (auto net: nets_)
    acumulated_hpwl += net.getHpwl();
  
  return acumulated_hpwl;
  
}

void SAplace::perturb(){
  int m = move_(generator_);
  int seq = move_(generator_);

  std::vector<int>* chosen = &pos_seq_;

  if (seq == 0){
    chosen = &neg_seq_;
  }
  

  if (m == 0){
    int index1, index2;
    generateRandomIndices(index1,index2);
    int temp_index1_content = (*chosen)[index1];
    (*chosen)[index1] = (*chosen)[index2];
    (*chosen)[index2] = temp_index1_content;
  }

  else if(pos_seq_.size() >= 3){
    int index1, index2, index3;
    generateRandomIndices(index1,index2,index3);
    int temp_index1_content = (*chosen)[index1];
    (*chosen)[index1] = (*chosen)[index2];
    (*chosen)[index2] = (*chosen)[index3];
    (*chosen)[index3] = temp_index1_content;
  }
  
}

void SAplace::generateRandomIndices(int& index1, int& index2)
{

  index1 =  rand() % pos_seq_.size();
  index2 = rand() % pos_seq_.size();

  while (index1 == index2) {
    index2 = rand() % pos_seq_.size();
  }
}

void SAplace::generateRandomIndices(int& index1, int& index2,int& index3)
{

  index1 =  rand() % pos_seq_.size();
  index2 = rand() % pos_seq_.size();

  while (index1 == index2) {
    index2 = rand() % pos_seq_.size();
  }

  index3 = rand() % pos_seq_.size();

  while (index3 == index1 || index3 == index2){
    index3 = rand() % pos_seq_.size();
  }

}

}

