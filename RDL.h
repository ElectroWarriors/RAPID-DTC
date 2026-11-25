#ifndef RDL_H_
#define RDL_H_

#include<iostream>
#include<vector>
#include<unordered_map>
#include<algorithm>
#include<random>

#include"global.h"

class PinReDistribution {
    public:
        PinReDistribution(std::vector<ClusterPairType> vcluster_pair) : _vcluster_pair(vcluster_pair) {
            calculateChipletRDLPriority();
        }
        void calculateChipletRDLPriority();
        bool ILPnonOverFlow(IndexType numLayer);
        void ReDistribution(bool random_esc);

    private:
        GlobalParameter global_parameter;
        std::vector<ClusterPairType> _vcluster_pair;
        std::unordered_map<IndexType, IndexType> _freq;
};



#endif