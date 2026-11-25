#ifndef ROUTER_GROUTER_H_
#define ROUTER_GROUTER_H_

#include <vector>
#include <iostream>

#include "global.h"
#include "ilp_solver.h"
#include "RDL.h"
#include "DTC.h"

#include "json/json.h"

class BIPGRouter {
    public:
        // initial
        // BIPGRouter(std::string dirname, std::string casedir) : _dirname(dirname), _casedir(casedir) {
        //     std::cout << "Hello Router. " << std::endl;
        //     _json_database = ReadJSON("chiplet.json");
        //     _json_datadescript = ReadJSON("case_discription.json");
        //     // test
        //     std::cout << _json_database["chiplets"][0]["id"].asInt() << ", "
        //               << _json_database["chiplets"][0]["name"].asString() << std::endl;
        // }
        BIPGRouter(Database db) : _db(db) {
            std::cout << "Hello Router. " << std::endl;
        }
        void setnumLayer(IndexType numLayer) { 
            _vlayer.clear();
            _numLayer = numLayer; 
            for (IndexType layerIdx = 0; layerIdx < _numLayer; layerIdx++) {
                _vlayer.emplace_back(Layer(layerIdx));
            }
        }
        IndexType getnumLayer() { return _numLayer; }

        void delemptyLayer() {}
        // database prepare
        // Json::Value ReadJSON(std::string filename);
        // bool WriteJSON(const std::string& filename, const Json::Value& root);
        // void GenetateDB();
        /* cluster pair classification
         * output: pair <cluster, cluster> */
        void ClusterPairGenetation();
        bool ClusterPairEsc(ClusterPairType& cpair);
        bool EscapePlanning(IndexType);
        IndexType FindMinimumLayersWithBinarySearch();
        void EscapeRouting();
        Json::Value TrackOutput();
        void DTCInsert();
        Json::Value DTCOutput();
        void clear_rout();

        // IndexType PhysicalMsg2varId(IndexType netIdx, IndexType layerIdx, IndexType esc0, IndexType esc1);
        // std::vector<IndexType> varId2PhysicalMsg(IndexType id);


        Json::Value run() {
            ClusterPairGenetation();
            PinReDistribution redis(_vcluster_pair);
            // redis.ReDistribution(true);
            // FindMinimumLayersWithBinarySearch();
            EscapePlanning(16);
            EscapeRouting();
            return TrackOutput();
            // std::cout << "finished! " << std::endl;
        }
        Json::Value run_RDL() {
            ClusterPairGenetation();
            PinReDistribution redis(_vcluster_pair); 
            setnumLayer(10);
            redis.ILPnonOverFlow(10);
            redis.ReDistribution(false);
            EscapeRouting();
            DTCInsert();
            return TrackOutput();
            // std::cout << "finished! " << std::endl;
        }

        Json::Value output_database();

    private:
        // std::string _dirname;
        // std::string _casedir;
        // Json::Value _json_database;
        // Json::Value _json_datadescript;
        Database _db;
        GlobalParameter global_parameter;
        std::vector<ClusterPairType> _vcluster_pair;
        std::vector<Layer> _vlayer;
        IndexType _numLayer;
        // DTC
        DTCTemplateDatabase _dtc_template;
        std::vector<LocXYType> _all_dtc_loc;
};



#endif


