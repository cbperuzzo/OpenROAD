#pragma once

#include "odb/db.h"

#include <vector>
#include <random>

#include "Net.h"
#include "Macro.h"
#include "Pin.h"

namespace sap{
        
class SAplace{
    public:
        SAplace(odb::dbDatabase* db, utl::Logger* logger);
        ~SAplace();

        void simulatedAnnealing(int n_threads, int iterations_per_T, double initial_T, double alpha);
    
    private:
        int hpwl();
        void perturb();
        float calcCost();
        float penalties();
        void pack();
        void saveState();
        void restoreState();
        void generateRandomIndices(int& index1, int& index2);
        void generateRandomIndices(int& index1, int& index2,int& index3);
        
        odb::dbDatabase* db_;
        utl::Logger* log_;

        std::vector<Macro> macros_;
        std::vector<Net> nets_;
        std::vector<Pin> pins_;

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

};

}