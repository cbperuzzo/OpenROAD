#include "Annealing.h"

namespace sap
{

void Annealing::pack(){
  
  std::vector<std::pair<int, int>> sequence_pair_pos(pos_seq_.size());

  for (int i = 0; i < pos_seq_.size(); i++) {
    sequence_pair_pos[pos_seq_[i]].first = i;
    sequence_pair_pos[neg_seq_[i]].second = i;
  }
  
  // (0 + offset) in ll/ul | (origin_x - offset) in lr/ur
  std::vector<int> accumulated_length(pos_seq_.size(), packing_params_.offset_x);
  for (int macro_id : pos_seq_) {
    const int neg_seq_pos = sequence_pair_pos[macro_id].second;

    // xMin for ll/ul | xMax for lr|ur
    Macro& macro = macros_[macro_id];
    int x = macro.xMin();

    if (!macro.isFixed()) {
      x = accumulated_length[neg_seq_pos];
      macro.xMin(x);
    }

    // (x + macro.dx()) in ll/ul | (x - macro.dx()) in lr/ur
    const int current_length = x + macro.dx();
  
    for (int j = neg_seq_pos; j < neg_seq_.size(); j++) {
      if (current_length > accumulated_length[j]) {
        accumulated_length[j] = current_length;
      } else {
        break;
      }
    }
  }
  
  // (accumulated_length[pos_seq_.size() - 1]) in ll/ul || (origin_x - accumulated_length[pos_seq_.size() - 1]) in lr/ur
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
    accumulated_length[i] = packing_params_.offset_y;
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

}