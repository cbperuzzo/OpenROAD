#pragma once

#include "odb/db.h"

#include <vector>
#include <random>
#include <deque>
#include <array>

#include "Net.h"
#include "Macro.h"
#include "Pin.h"
#include "Annealing.h"
#include "AdjacencyGraphBuilder.h"

namespace sta {
class dbSta;
}

namespace sap{

class SAplace{
    public:

        struct Partition {
            std::vector<Macro*> macros;
            std::vector<Net*> nets;
            Annealing::Corner corner;
        };

        SAplace(odb::dbDatabase* db, sta::dbSta* sta, utl::Logger* logger);
        ~SAplace();

        void init(int halo_width, int halo_height, float boundy_const, float boundry_coef);
        void run(int iterations_per_T, double initial_T, double alpha, bool worst_hpwl = false);

    private:

        void initializeProxies();
        AdjacencyMatrix buildAdjacencyGraph();
        //gets all the nets that have pins in this set of macros and also have pins in other macros;
        std::vector<Net*> findSharedNets(std::unordered_set<Macro*> macros);
        //gets all the nets that have pins in this set of macros and only in this set of macros;
        std::vector<Net*>  findExclusivedNets(std::unordered_set<Macro*> macros);
        std::array<Partition, 4> makePartitions(AdjacencyMatrix adj);
        // Kernighan-Lin graph bisection over ids (positions in macro_)
        std::array<std::vector<int>, 2> kernighanLinBisect(std::vector<int> ids, const AdjacencyMatrix& adj);

        odb::dbDatabase* db_;
        sta::dbSta* sta_;
        utl::Logger* log_;

        std::vector<Macro> macros_;
        std::deque<Net> nets_;
        std::deque<Pin> pins_;

        std::map<Net*,std::unordered_set<Macro*>> net_macros_;
        AdjacencyMatrix adjacency_;

        std::vector<int> pos_seq_;
        std::vector<int> neg_seq_;
        std::vector<int> pos_seq_backup_;
        std::vector<int> neg_seq_backup_;
        std::vector<int> best_pos_seq_;
        std::vector<int> best_neg_seq_;

        std::default_random_engine generator_;
        std::uniform_real_distribution<float> prob_;
        std::uniform_int_distribution<int> move_;

        int max_h_;
        int max_w_;
        int site_width_;
        int row_height_;

};

}