#include "sap/SAplace.h"

#include <random>
#include <algorithm>
#include "sta/StaMain.hh"
#include "odb/db.h"

namespace sap{

SAplace::SAplace(odb::dbDatabase* db, utl::Logger* log):
  pos_seq_(),
  neg_seq_(),
  pos_seq_backup_(),
  neg_seq_backup_(),
  best_pos_seq_(),
  best_neg_seq_(),
  macros_(),
  nets_()
{
  db_ = db;
  log_ = log;
  generator_ = std::default_random_engine(44);
  prob_ = std::uniform_real_distribution<float>(0,1);
  move_ = std::uniform_int_distribution<int>(0,1);
  
}

SAplace::~SAplace(){

}

void SAplace::simulatedAnnealing(int n_threads, int iterations_per_T, double initial_T, double alpha)
{

  for(auto inst : db_->getChip()->getBlock()->getInsts()){
    if (inst->isBlock())
      macros_.push_back(inst);
  }

  for (auto net : db_->getChip()->getBlock()->getNets()){
    nets_.push_back(Net(net));
  }
  

  int n_macros = macros_.size();

  log_->report("total macros: {}",n_macros);
  
  pos_seq_.resize(n_macros);
  neg_seq_.resize(n_macros);
  pos_seq_backup_.resize(n_macros);
  neg_seq_backup_.resize(n_macros);

  // random place
  std::iota(pos_seq_.begin(),pos_seq_.end(),0);
  std::iota(neg_seq_.begin(),neg_seq_.end(),0);

  std::shuffle(pos_seq_.begin(),pos_seq_.end(),generator_);
  std::shuffle(neg_seq_.begin(),neg_seq_.end(),generator_);
  
  pack();
  float cost = calcCost();
  float best_cost = cost;
  best_pos_seq_ = pos_seq_;
  best_neg_seq_ = neg_seq_;
  float temperature = initial_T;

  while(temperature >= 1) {
    for(int iteration = 0; iteration < iterations_per_T; iteration++){

      saveState();
      perturb();
      pack();

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
        const float accept_chance = std::exp(-delta / temperature);
        float num = prob_(generator_);
        if (num < accept_chance){
          cost = new_cost;
        }
        else{
          restoreState();
        }
        
      }
      
    }
    temperature *= alpha;
  }

  pos_seq_ = best_pos_seq_;
  neg_seq_ = best_neg_seq_;

  pack();

}

void SAplace::pack(){
  
  std::vector<std::pair<int, int>> sequence_pair_pos(pos_seq_.size());

  for (int i = 0; i < pos_seq_.size(); i++) {
    sequence_pair_pos[pos_seq_[i]].first = i;
    sequence_pair_pos[neg_seq_[i]].second = i;
  }

  std::vector<int> accumulated_length(pos_seq_.size(), 0);
  for (int macro_id : pos_seq_) {
    const int neg_seq_pos = sequence_pair_pos[macro_id].second;

    odb::dbInst* macro = macros_[macro_id];
    odb::Rect rect = macro->getBBox()->getBox();

    if (!macro->isFixed()) {
      rect.set_xlo(accumulated_length[neg_seq_pos]);
    }

    const int current_length = rect.xMin() + rect.dx();
  
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
    accumulated_length[i] = 0;
  }

  for (int i = 0; i < pos_seq_.size(); i++) {
    const int macro_id = reversed_pos_seq[i];
    const int neg_seq_pos = sequence_pair_pos[macro_id].second;

    odb::dbInst* macro = macros_[macro_id];
    odb::Rect rect = macro->getBBox()->getBox();

    if (!macro->isFixed()) {
      rect.set_ylo(neg_seq_pos);
    }

    const int current_height = rect.yMin() + rect.dy();

    for (int j = neg_seq_pos; j < neg_seq_.size(); j++) {
      if (current_height > accumulated_length[j]) {
        accumulated_length[j] = current_height;
      } else {
        break;
      }
    }
  }

  height_ = accumulated_length[pos_seq_.size() - 1];

  for (auto net : nets_){
    net.update();
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
  return (float) hpwl();
}

int SAplace::hpwl(){
  
  int acumulated_hpwl;

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

// TODO: swap reverso ao invez de save/restore state
// TODO: proxy objects para macros também

