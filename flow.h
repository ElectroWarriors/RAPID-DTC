#ifndef FLOW_H_
#define FLOW_H_

#include<iostream>
#include<string>
#include"global.h"
#include"GRouter.h"
#include"json/json.h"

class MainFlow {
    public: 
        MainFlow(std::string dirname, std::string casedir) : _dirname(dirname), _casedir(casedir) {
            std::cout << "Hello Router. " << std::endl;
            _json_database = ReadJSON("json_chiplets/chiplet.json");
            _json_interposer = ReadJSON("json_chiplets/interposer.json");
            _json_datadescript = ReadJSON("place_init/case_discription.json");
            // test
            std::cout << _json_database["chiplets"][0]["id"].asInt() << ", "
                    << _json_database["chiplets"][0]["name"].asString() << std::endl;
        }

        Json::Value ReadJSON(std::string filename);
        bool WriteJSON(const std::string& filename, const Json::Value& root);
        void GenetateDB();

        void runflow() {
            // init
            GenetateDB();
            // router
            RunRouter();
            
        }

        void RunRouter() {
            BIPGRouter gr(_db);
            Json::Value json_root;
            if (RUN_RDL) {
                json_root = gr.run_RDL();
            } else {
                json_root = gr.run();
            }
            
            if (!WriteJSON("json_chiplets/output_track.json", json_root)) {
                std::cerr << "Failed to write JSON data." << std::endl;
            }
            
            if (RUN_RDL) {
                json_root = gr.output_database();
                if (!WriteJSON("json_chiplets/chiplets_rdl.json", json_root)) {
                    std::cerr << "Failed to write JSON data." << std::endl;
                }
            }

        }

    private:
        std::string _dirname;
        std::string _casedir;
        Json::Value _json_database;
        Json::Value _json_interposer;
        Json::Value _json_datadescript;
        Database _db;
        GlobalParameter global_parameter;

};




#endif