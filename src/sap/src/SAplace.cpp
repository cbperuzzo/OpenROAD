#include "sap/SAplace.h"

#include "sap/AdjacencyGraphBuilder.h"
#include "sta/StaMain.hh"
#include "odb/db.h"
#include "utl/Logger.h"
#include <map>
#include <limits>
#include <unordered_set>
#include <utility>

namespace sap{

int edgeWeight(const AdjacencyMatrix& adj, int gi, int gj)
{
  if (gi == gj) {
    return 0;
  }
  // simetric "matrix", but the actual value is only stored at [a][b] where a < b
  // a = b doesnt metter/is always 0 anyways
  auto key = gi < gj ? std::make_pair(gi, gj) : std::make_pair(gj, gi);
  auto it = adj.find(key);
  return it == adj.end() ? 0 : it->second;
}

std::vector<int> computeD(const std::vector<int>& ids,
                                   const std::vector<int>& side,
                                   const AdjacencyMatrix& adj)
{
  const int n = static_cast<int>(ids.size());
  std::vector<int> d(n, 0);
  for (int i = 0; i < n; i++) {
    int internal = 0;
    int external = 0;
    for (int j = 0; j < n; j++) {
      if (i == j) {
        continue;
      }
      // only considers ids passed, reather then all related cells in adj
      // thats disered behaviour
      int w = edgeWeight(adj, ids[i], ids[j]);
      if (w == 0) {
        continue;
      }
      if (side[j] == side[i]) {
        internal += w;
      } else {
        external += w;
      }
    }
    d[i] = external - internal;
  }
  return d;
}

std::array<std::vector<int>, 2> SAplace::kernighanLinBisect(
    std::vector<int> ids,
    const AdjacencyMatrix& adj)
{
  const int n = static_cast<int>(ids.size());
  std::array<std::vector<int>, 2> result;

  if (n == 0) {
    return result;
  }
  if (n == 1) {
    result[0].push_back(ids[0]);
    return result;
  }

  // side A, first or 0 | side B second or 1
  std::vector<int> side(n);
  for (int i = 0; i < n; i++) {
    // |A| <= |B| needs to be true
    side[i] = i +1 % 2 ;
  }

  bool improved = true;
  while (improved) {
    improved = false;

    std::vector<int> d = computeD(ids, side, adj);

    std::vector<bool> locked(n, false);
    // side A, first or 0 | side B second or 1
    std::vector<std::pair<int, int>> swaps;
    std::vector<int> gains;

    int remaining = n;
    while (remaining > 0) {
      int best_a = -1;
      int best_b = -1;
      // the best gain can be negative sometimes
      int best_gain = std::numeric_limits<int>::min();
      for (int a = 0; a < n; a++) {
        if (locked[a] || side[a] != 0) {
          continue;
        }
        for (int b = 0; b < n; b++) {
          if (locked[b] || side[b] != 1) {
            continue;
          }
          int gain = d[a] + d[b] - 2 * edgeWeight(adj, ids[a], ids[b]);
          if (gain > best_gain) {
            best_gain = gain;
            best_a = a;
            best_b = b;
          }
        }
      }
      if (best_a == -1) {
        break;
      }

      // performs swap
      side[best_a] = 1;
      side[best_b] = 0;
      // marked as locked for this pass
      locked[best_a] = true;
      locked[best_b] = true;

      // stores the swaps performed
      swaps.emplace_back(best_a, best_b);
      gains.push_back(best_gain);
      remaining -= 2;

      for (int k = 0; k < n; k++) {
        if (locked[k]) {
          continue;
        }
        int w_ak = edgeWeight(adj, ids[k], ids[best_a]);
        int w_bk = edgeWeight(adj, ids[k], ids[best_b]);
        if (side[k] == 0) {
          d[k] += 2 * w_ak - 2 * w_bk;
        } else {
          d[k] += 2 * w_bk - 2 * w_ak;
        }
      }
    }

    int acc = 0;
    int max_acc = 0;
    int max_idx = -1;
    for (size_t i = 0; i < gains.size(); i++) {
      acc += gains[i];
      if (acc > max_acc) {
        max_acc = acc;
        max_idx = static_cast<int>(i);
      }
    }

    for (size_t i = static_cast<size_t>(max_idx + 1); i < swaps.size(); i++) {
      side[swaps[i].first] = 0;
      side[swaps[i].second] = 1;
    }
    if (max_idx >= 0) {
      improved = true;
    }
  }

  for (int i = 0; i < n; i++) {
    result[side[i]].push_back(ids[i]);
  }
  return result;
}

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
        net_macros_[&nets_.back()].insert(&macro);
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

}

void SAplace::buildAdjacencyGraph(){
  AdjacencyGraphBuilder builder(sta_, db_, log_);
  AdjacencyMatrix matrix = builder.build(macros_);

}

std::vector<Net*> SAplace::findSharedNets(std::unordered_set<Macro*> macros){
  std::vector<Net*> out_nets;
  for(auto& pair : net_macros_ ){
    if(pair.second.size() <= 1){
      continue;
    }
    bool touches_set = false;
    bool touches_outside = false;
    for(Macro* net_macro : pair.second){
      if(macros.contains(net_macro)){
        touches_set = true;
      } else {
        touches_outside = true;
      }
    }
    if(touches_set && touches_outside){
      out_nets.push_back(pair.first);
    }
  }
  return out_nets;
}

std::vector<Net*> SAplace::findExclusivedNets(std::unordered_set<Macro*> macros){
  std::vector<Net*> ex_nets;
  for(auto& pair : net_macros_ ){
    bool touches_set = false;
    bool touches_outside = false;
    for(Macro* net_macro : pair.second){
      if(macros.contains(net_macro)){
        touches_set = true;
      } else {
        touches_outside = true;
      }
    }
    if(touches_set && !touches_outside){
      ex_nets.push_back(pair.first);
    }
  }
  return ex_nets;
}

std::array<SAplace::Partition, 4> SAplace::makePartitions(AdjacencyMatrix adj){
  std::array<Partition, 4> partitions;

  std::vector<int> all_ids(macros_.size());
  for(size_t i = 0; i < macros_.size(); i++){
    all_ids[i] = static_cast<int>(i);
  }

  auto halves = kernighanLinBisect(std::move(all_ids), adj);
  auto quadrants_a = kernighanLinBisect(halves[0], adj);
  auto quadrants_b = kernighanLinBisect(halves[1], adj);

  std::array<std::vector<int>, 4> groups
      = {quadrants_a[0], quadrants_a[1], quadrants_b[0], quadrants_b[1]};
  static constexpr std::array<Annealing::Corner, 4> corners
      = {Annealing::LL, Annealing::UL, Annealing::LR, Annealing::UR};

  for(int i = 0; i < 4; i++){
    std::unordered_set<Macro*> macro_set;
    for(int idx : groups[i]){
      Macro* macro = &macros_[idx];
      partitions[i].macros.push_back(macro);
      macro_set.insert(macro);
    }
    partitions[i].nets = findExclusivedNets(macro_set);
    partitions[i].corner = corners[i];
  }

  return partitions;
}

}
