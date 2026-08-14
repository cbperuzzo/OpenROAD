#pragma once

#include <deque>
#include <map>
#include <set>
#include <utility>

#include "db_sta/dbSta.hh"
#include "sta/Bfs.hh"


namespace utl {
class Logger;
}

namespace sap {

class Macro;

using AdjacencyMatrix = std::map<std::pair<int, int>, int>;

class AdjacencyGraphBuilder
{
 public:
  AdjacencyGraphBuilder(sta::dbSta* sta,
                        odb::dbDatabase* db,
                        utl::Logger* logger);

  AdjacencyMatrix build(std::deque<Macro>& macros);

 private:
  using MacroSet = std::set<Macro*>;
  using VertexFaninMap = std::map<sta::Vertex*, MacroSet>;
  using MacroPair = std::pair<Macro*, Macro*>;
  using AdjWeightMap = std::map<MacroPair, int>;

  bool isMissingLiberty();
  void seedFaninBfs(sta::BfsFwdIterator& bfs,
                    VertexFaninMap& vertex_fanins,
                    std::deque<Macro>& macros);
  void findFanins(sta::BfsFwdIterator& bfs, VertexFaninMap& vertex_fanins);
  void copyFaninsAcrossRegisters(sta::BfsFwdIterator& bfs,
                                 VertexFaninMap& vertex_fanins);
  sta::Pin* findSeqOutPin(sta::Instance* inst, sta::LibertyPort* out_port);
  void findAdjWeights(VertexFaninMap& vertex_fanins,
                      AdjWeightMap& adj_map,
                      std::deque<Macro>& macros);
  AdjacencyMatrix fillMatrix(AdjWeightMap& adj_map, std::deque<Macro>& macros);

  sta::dbSta* sta_;
  odb::dbDatabase* db_;
  utl::Logger* logger_;

  std::map<Macro*, int> macro_index_;

  static constexpr int reg_adjacency_depth_ = 3;
};

}
