#pragma once

#include "odb/db.h"

#include <vector>
#include <random>
#include <deque>
#include <functional>

#include "sap/Net.h"
#include "sap/Macro.h"
#include "sap/Pin.h"

namespace sap{

class Annealing{
public:

    enum Corner {LL,UL,LR,UR};

    Annealing(
        std::vector<Macro> macros,
        std::vector<Net> nets,
        int max_w,
        int max_h,
        int origin_x,
        int orign_y,
        // the exact packing strategy might change a bit based on the corner selected
        Corner corner,
        int offset_x,
        int offset_y,
        std::default_random_engine& generator,
        std::uniform_real_distribution<float>& prob,
        std::uniform_int_distribution<int>& move
    );
    ~Annealing();

    void run(int iterations_per_T, double initial_T, double alpha);

    std::vector<Macro> getMacros() { return macros_; }

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

    struct{
        int origin_x;
        int origin_y;
        int off_origin_x;
        int off_origin_y;
        int max_h;
        int max_w;
    } packing_params_;

    struct{
        std::function<int(Macro&)> get_x;
        std::function<void(Macro&, int)> set_x;
        std::function<int(int, int)> acc_x;
        std::function<bool(int, int)> ahead_x;
        std::function<int(int, int)> finish_x;

        std::function<int(Macro&)> get_y;
        std::function<void(Macro&, int)> set_y;
        std::function<int(int, int)> acc_y;
        std::function<bool(int, int)> ahead_y;
        std::function<int(int, int)> finish_y;
    } packing_ops_;

    int width_;
    int height_;


};

}