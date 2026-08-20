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
namespace sta {
class dbSta;
}

namespace sap{

class SAplace{

    typedef struct Partition {
        std::vector<Macro*> macros;
        std::vector<Net*> nets;
        Annealing::Corner corner;
    };

    public:
        SAplace(odb::dbDatabase* db, sta::dbSta* sta, utl::Logger* logger);
        ~SAplace();

        void init(int iterations_per_T, double initial_T, double alpha, int halo_width, int halo_height);

    private:
        
        void initializeProxies();
        void buildAdjacencyGraph();
        //gets all the nets have pins in this set of macros and also have pins in other macros;
        std::vector<Net*> findSharedNets(std::unordered_set<Macro*>& macros);
        std::array<Partition, 4> getPartitions();

        odb::dbDatabase* db_;
        sta::dbSta* sta_;
        utl::Logger* log_;

        std::vector<Macro> macros_;
        std::vector<Net> nets_;
        std::vector<Pin> pins_;

        std::map<Net*,std::unordered_set<Macro*>> shared_nets_;

        std::vector<int> pos_seq_;
        std::vector<int> neg_seq_;
        std::vector<int> pos_seq_backup_;
        std::vector<int> neg_seq_backup_;
        std::vector<int> best_pos_seq_;
        std::vector<int> best_neg_seq_;

        std::default_random_engine generator_;
        std::uniform_real_distribution<float> prob_;
        std::uniform_int_distribution<int> move_;

        int width_;
        int height_;

        int max_h_;
        int max_w_;


};

}