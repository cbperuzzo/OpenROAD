#pragma once

#include "odb/db.h"

#include <vector>
#include <random>
#include <deque>

#include "sap/Net.h"
#include "sap/Macro.h"
#include "sap/Pin.h"
namespace sap{

class Annealing{
    public:
        Annealing(
            std::vector<Macro> macros,
            std::vector<Net> nets,
            int max_w,
            int max_h,
            std::default_random_engine& generator,
            std::uniform_real_distribution<float>& prob,
            std::uniform_int_distribution<int>& move
        );
        ~Annealing();

        void run(int iterations_per_T, double initial_T, double alpha);

        std::vector<Macro> getMacros();

    private:
        int hpwl();
        void perturb();
        float calcCost();
        float penalties();
        void pack(int offset_x, int offset_y);
        void saveState();
        void restoreState();
        void generateRandomIndices(int& index1, int& index2);
        void generateRandomIndices(int& index1, int& index2,int& index3);
        

        std::vector<Macro> macros_;
        std::vector<Net> nets_;

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