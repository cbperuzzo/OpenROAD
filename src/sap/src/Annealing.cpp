#include "Annealing.h"

namespace sap
{

Annealing::Annealing(
  std::vector<Macro> macros,
  std::vector<Net> nets,
  int max_w,
  int max_h,
  int origin_x,
  int orign_y,
  Corner corner,
  int offset_x,
  int offset_y,
  std::default_random_engine& generator,
  std::uniform_real_distribution<float>& prob,
  std::uniform_int_distribution<int>& move
){
  macros_ = macros;
  nets_ = nets;
  packing_params_.max_h = max_h;
  packing_params_.max_w = max_w;
  packing_params_.origin_x = origin_x;
  packing_params_.origin_y = orign_y;
  
  generator_ = generator;
  prob_ = prob;
  move_ = move;

  const bool anchor_left = corner == LL || corner == UL;
  const bool anchor_bottom = corner == LL || corner == LR;

  packing_params_.off_origin_x = anchor_left ? offset_x : origin_x - offset_x;
  packing_params_.off_origin_y = anchor_bottom ? offset_y : orign_y - offset_y;

  if (anchor_left) {
    packing_ops_.get_x = [](Macro& m) { return m.xMin(); };
    packing_ops_.set_x = [](Macro& m, int x) { m.xMin(x); };
    packing_ops_.acc_x = [](int x, int dx) { return x + dx; };
    packing_ops_.ahead_x = std::greater<int>();
    packing_ops_.finish_x = [](int /* origin_x */, int acc) { return acc; };
  } else {
    packing_ops_.get_x = [](Macro& m) { return m.xMax(); };
    packing_ops_.set_x = [](Macro& m, int x) { m.xMax(x); };
    packing_ops_.acc_x = [](int x, int dx) { return x - dx; };
    packing_ops_.ahead_x = std::less<int>();
    packing_ops_.finish_x = [](int origin_x, int acc) { return origin_x - acc; };
  }

  if (anchor_bottom) {
    packing_ops_.get_y = [](Macro& m) { return m.yMin(); };
    packing_ops_.set_y = [](Macro& m, int y) { m.yMin(y); };
    packing_ops_.acc_y = [](int y, int dy) { return y + dy; };
    packing_ops_.ahead_y = std::greater<int>();
    packing_ops_.finish_y = [](int /* origin_y */, int acc) { return acc; };
  } else {
    packing_ops_.get_y = [](Macro& m) { return m.yMax(); };
    packing_ops_.set_y = [](Macro& m, int y) { m.yMax(y); };
    packing_ops_.acc_y = [](int y, int dy) { return y - dy; };
    packing_ops_.ahead_y = std::less<int>();
    packing_ops_.finish_y = [](int origin_y, int acc) { return origin_y - acc; };
  }
}

void Annealing::pack(){

  std::vector<std::pair<int, int>> sequence_pair_pos(pos_seq_.size());

  for (int i = 0; i < pos_seq_.size(); i++) {
    sequence_pair_pos[pos_seq_[i]].first = i;
    sequence_pair_pos[neg_seq_[i]].second = i;
  }

  std::vector<int> accumulated_length(pos_seq_.size(), packing_params_.off_origin_x);
  for (int macro_id : pos_seq_) {
    const int neg_seq_pos = sequence_pair_pos[macro_id].second;

    Macro& macro = macros_[macro_id];
    int x = packing_ops_.get_x(macro);

    if (!macro.isFixed()) {
      x = accumulated_length[neg_seq_pos];
      packing_ops_.set_x(macro, x);
    }

    const int current_length = packing_ops_.acc_x(x, macro.dx());

    for (int j = neg_seq_pos; j < neg_seq_.size(); j++) {
      if (packing_ops_.ahead_x(current_length,accumulated_length[j])) {
        accumulated_length[j] = current_length;
      } else {
        break;
      }
    }
  }

  width_ = packing_ops_.finish_x(packing_params_.origin_x, accumulated_length[pos_seq_.size() - 1]);

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
    accumulated_length[i] = packing_params_.off_origin_y;
  }

  for (int i = 0; i < pos_seq_.size(); i++) {
    const int macro_id = reversed_pos_seq[i];
    const int neg_seq_pos = sequence_pair_pos[macro_id].second;

    Macro& macro = macros_[macro_id];
    int y = packing_ops_.get_y(macro);

    if (!macro.isFixed()) {
      y = accumulated_length[neg_seq_pos];
      packing_ops_.set_y(macro, y);
    }

    const int current_height =  packing_ops_.acc_y(y, macro.dy());

    for (int j = neg_seq_pos; j < neg_seq_.size(); j++) {
      if (packing_ops_.ahead_y(current_height, accumulated_length[j])) {
        accumulated_length[j] = current_height;
      } else {
        break;
      }
    }
  }

  height_ = packing_ops_.finish_y(packing_params_.origin_y, accumulated_length[pos_seq_.size() - 1]);

  for (auto& net : nets_){
    net.update();
  }

}

void Annealing::saveState(){
  pos_seq_backup_ = pos_seq_;
  neg_seq_backup_ = neg_seq_;
}

void Annealing::restoreState(){
  pos_seq_ = pos_seq_backup_;
  neg_seq_ = neg_seq_backup_;
}

float Annealing::calcCost(){  
  return (float) hpwl() +  penalties();
}

float Annealing::penalties(){
  float penalty = 0.0;
  
  if(height_ >= packing_params_.max_h){
    penalty += 1e+7; 
  }
  
  if(width_ >= packing_params_.max_w){
    penalty += 1e+7;
  }

  return penalty;
}


int Annealing::hpwl(){
  
  int acumulated_hpwl = 0;

  for (auto net: nets_)
    acumulated_hpwl += net.getHpwl();
  
  return acumulated_hpwl;
  
}

void Annealing::perturb(){
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

void Annealing::generateRandomIndices(int& index1, int& index2)
{

  index1 =  rand() % pos_seq_.size();
  index2 = rand() % pos_seq_.size();

  while (index1 == index2) {
    index2 = rand() % pos_seq_.size();
  }
}

void Annealing::generateRandomIndices(int& index1, int& index2,int& index3)
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