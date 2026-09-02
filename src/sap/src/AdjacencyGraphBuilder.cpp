#include "sap/AdjacencyGraphBuilder.h"

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "sap/Macro.h"
#include "sta/Bfs.hh"
#include "sta/FuncExpr.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/PortDirection.hh"
#include "sta/Sdc.hh"
#include "sta/SearchPred.hh"
#include "sta/Sequential.hh"
#include "sta/Sta.hh"
#include "utl/Logger.h"

namespace sap {

AdjacencyGraphBuilder::AdjacencyGraphBuilder(sta::dbSta* sta,
                                             odb::dbDatabase* db,
                                             utl::Logger* logger)
    : sta_(sta), db_(db), logger_(logger)
{
}

AdjacencyMatrix AdjacencyGraphBuilder::build(std::vector<Macro>& macros)
{
  const size_t macro_count = macros.size();

  if (macros.empty()) {
    return {};
  }

  if (isMissingLiberty()) {
    logger_->report(
        "Some instances do not have Liberty models. Skipping "
        "connectivity-based macro adjacency.");
    return {};
  }

  macro_index_.clear();
  for (size_t i = 0; i < macro_count; i++) {
    macro_index_[&macros[i]] = i;
  }

  sta_->ensureLevelized();
  sta_->ensureClkNetwork(sta_->cmdMode());

  VertexFaninMap vertex_fanins;
  sta::SearchPred0 srch_pred(sta_);
  sta::BfsFwdIterator bfs(sta::BfsIndex::other, &srch_pred, sta_);

  seedFaninBfs(bfs, vertex_fanins, macros);
  findFanins(bfs, vertex_fanins);

  for (int i = 0; i < reg_adjacency_depth_; i++) {
    copyFaninsAcrossRegisters(bfs, vertex_fanins);
    findFanins(bfs, vertex_fanins);
  }

  AdjWeightMap adj_map;
  findAdjWeights(vertex_fanins, adj_map, macros);

  return fillMatrix(adj_map, macros);
}

bool AdjacencyGraphBuilder::isMissingLiberty()
{
  sta::Network* network = sta_->network();
  sta::LeafInstanceIterator* inst_iter = network->leafInstanceIterator();
  while (inst_iter->hasNext()) {
    sta::Instance* inst = inst_iter->next();
    if (network->libertyCell(inst) == nullptr) {
      delete inst_iter;
      return true;
    }
  }
  delete inst_iter;
  return false;
}

void AdjacencyGraphBuilder::seedFaninBfs(sta::BfsFwdIterator& bfs,
                                         VertexFaninMap& vertex_fanins,
                                         std::vector<Macro>& macros)
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  sta::Graph* graph = sta_->ensureGraph();
  // Seed the BFS with macro output pins.
  for (Macro& macro : macros) {
    for (odb::dbITerm* iterm : macro.getInst()->getITerms()) {
      sta::Pin* pin = network->dbToSta(iterm);
      if (pin == nullptr) {
        continue;
      }
      if (network->direction(pin)->isAnyOutput()
          && !sta_->isClock(pin, sta_->cmdMode())) {
        sta::Vertex* vertex = graph->pinDrvrVertex(pin);
        vertex_fanins[vertex].insert(&macro);
        bfs.enqueueAdjacentVertices(vertex);
      }
    }
  }
}

void AdjacencyGraphBuilder::findFanins(sta::BfsFwdIterator& bfs,
                                       VertexFaninMap& vertex_fanins)
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  sta::Graph* graph = sta_->ensureGraph();
  while (bfs.hasNext()) {
    sta::Vertex* vertex = bfs.next();
    MacroSet& fanins = vertex_fanins[vertex];
    sta::VertexInEdgeIterator fanin_iter(vertex, graph);
    while (fanin_iter.hasNext()) {
      sta::Edge* edge = fanin_iter.next();
      sta::Vertex* fanin_vertex = edge->from(graph);
      // Union fanin sets of fanin vertices.
      for (Macro* fanin : vertex_fanins[fanin_vertex]) {
        fanins.insert(fanin);
        debugPrint(logger_,
                  utl::SAP,
                  "find_fanins",
                  1,
                  "{} + {}",
                  vertex->name(network),
                  fanin->getInst()->getConstName());
      }
    }
    bfs.enqueueAdjacentVertices(vertex);
  }
}

void AdjacencyGraphBuilder::copyFaninsAcrossRegisters(
    sta::BfsFwdIterator& bfs,
    VertexFaninMap& vertex_fanins)
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  sta::Graph* graph = sta_->ensureGraph();
  sta::Instance* top_inst = network->topInstance();
  sta::LeafInstanceIterator* leaf_iter
      = network->leafInstanceIterator(top_inst);
  while (leaf_iter->hasNext()) {
    sta::Instance* inst = leaf_iter->next();
    sta::LibertyCell* lib_cell = network->libertyCell(inst);
    if (lib_cell != nullptr && lib_cell->hasSequentials()
        && !lib_cell->isMacro()) {
      for (const sta::Sequential& seq : lib_cell->sequentials()) {
        sta::FuncExpr* data_expr = seq.data();
        for (sta::LibertyPort* data_port : data_expr->ports()) {
          sta::Pin* data_pin = network->findPin(inst, data_port);
          sta::LibertyPort* out_port = seq.output();
          sta::Pin* out_pin = findSeqOutPin(inst, out_port);
          if (data_pin && out_pin) {
            sta::Vertex* data_vertex = graph->pinLoadVertex(data_pin);
            sta::Vertex* out_vertex = graph->pinDrvrVertex(out_pin);
            // Copy fanins from D to Q on register.
            vertex_fanins[out_vertex] = vertex_fanins[data_vertex];
            bfs.enqueueAdjacentVertices(out_vertex);
          }
        }
      }
    }
  }
  delete leaf_iter;
}

sta::Pin* AdjacencyGraphBuilder::findSeqOutPin(sta::Instance* inst,
                                               sta::LibertyPort* out_port)
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  if (out_port->direction()->isInternal()) {
    sta::InstancePinIterator* pin_iter = network->pinIterator(inst);
    while (pin_iter->hasNext()) {
      sta::Pin* pin = pin_iter->next();
      sta::LibertyPort* lib_port = network->libertyPort(pin);
      if (lib_port->direction()->isAnyOutput()) {
        sta::FuncExpr* func = lib_port->function();
        if (func->hasPort(out_port)) {
          sta::Pin* out_pin = network->findPin(inst, lib_port);
          if (out_pin) {
            delete pin_iter;
            return out_pin;
          }
        }
      }
    }
    delete pin_iter;
    return nullptr;
  }
  return network->findPin(inst, out_port);
}

void AdjacencyGraphBuilder::findAdjWeights(VertexFaninMap& vertex_fanins,
                                           AdjWeightMap& adj_map,
                                           std::vector<Macro>& macros)
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  sta::Graph* graph = sta_->ensureGraph();
  // Find adjacencies from macro input pin fanins.
  for (Macro& macro : macros) {
    for (odb::dbITerm* iterm : macro.getInst()->getITerms()) {
      sta::Pin* pin = network->dbToSta(iterm);
      if (pin == nullptr) {
        continue;
      }
      if (network->direction(pin)->isAnyInput()) {
        sta::Vertex* vertex = graph->pinLoadVertex(pin);
        MacroSet& pin_fanins = vertex_fanins[vertex];
        for (Macro* pin_fanin : pin_fanins) {
          // Adjacencies are symmetric so only fill in one side.
          if (pin_fanin != &macro) {
            MacroPair from_to = (pin_fanin > &macro)
                                    ? MacroPair(pin_fanin, &macro)
                                    : MacroPair(&macro, pin_fanin);
            adj_map[from_to]++;
          }
        }
      }
    }
  }
}

AdjacencyMatrix AdjacencyGraphBuilder::fillMatrix(AdjWeightMap& adj_map,
                                                  std::vector<Macro>& macros)
{
  AdjacencyMatrix matrix;

  for (const auto& [from_to, weight] : adj_map) {
    int idx1 = macro_index_.at(from_to.first);
    int idx2 = macro_index_.at(from_to.second);
    matrix[idx1 < idx2 ? std::make_pair(idx1, idx2)
                        : std::make_pair(idx2, idx1)]
        = weight;
    if (weight > 0) {
      debugPrint(logger_,
                utl::SAP,
                "weights",
                1,
                "{} -> {} {}",
                from_to.first->getInst()->getConstName(),
                from_to.second->getInst()->getConstName(),
                weight);
    }
  }

  return matrix;
}

}  // namespace sap
