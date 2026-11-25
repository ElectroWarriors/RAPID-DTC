#include<set>
#include<iostream>
#include<string>
#include "flow.h"
#include "global.h"
#include<json/json.h>

Json::Value MainFlow::ReadJSON(std::string filename) {
    Json::Reader reader;
    Json::Value db;
    std::string filepath = _dirname+_casedir+filename;
    std::cout << filepath << std::endl;
    std::ifstream infile(filepath, std::ios::binary);
    if (!infile.is_open()) {
        std::cout << "Error opening file\n";
        return 0;
    }
    if (!reader.parse(infile, db)) {
        return 0;
    }
    return db;
}

bool MainFlow::WriteJSON(const std::string& filename, const Json::Value& root) {
    std::string filepath = _dirname+_casedir+filename;
    std::ofstream file(filepath);
    if (file.is_open()) {
        file << root.toStyledString(); // 美化输出
        file.close();
        std::cout << "JSON data written to " << filepath << std::endl;
        return true;
    } else {
        std::cerr << "Error opening file for writing" << std::endl;
        return false;
    }
}

void MainFlow::GenetateDB() {
    auto sizeTInterposer = _json_datadescript["interposer"]["sizeInterposer"];
    auto numTSV = _json_datadescript["interposer"]["numTSV"];
    _db.global().TSVXDistance() = static_cast<IndexType>(sizeTInterposer[0].asDouble() / numTSV[0].asDouble());
    _db.global().TSVYDistance() = static_cast<IndexType>(sizeTInterposer[1].asDouble() / numTSV[1].asDouble());

    // chiplets
    std::cout << "chiplet init..." << std::endl;
    for (IndexType chipletIdx=0; chipletIdx<_json_database["chiplets"].size(); chipletIdx++) {
        if (_json_database["chiplets"][chipletIdx]["id"].asInt() != _json_datadescript["chiplets"][chipletIdx]["id"].asInt()) {
            throw std::invalid_argument(" there are some bug to be fixed");
        }
        auto js_chiplet_i = _json_database["chiplets"][chipletIdx];
        Chiplet* chiplet_i = _db.allocateChiplet(js_chiplet_i["id"].asInt(), js_chiplet_i["name"].asString());
        chiplet_i->setShape(js_chiplet_i["size"][0].asInt(), js_chiplet_i["size"][1].asInt()); //
        chiplet_i->setLocation(js_chiplet_i["loc"][0].asInt(), js_chiplet_i["loc"][1].asInt());
        chiplet_i->setXYNumPins(_json_datadescript["chiplets"][chipletIdx]["numPin"][0].asInt(), 
                                _json_datadescript["chiplets"][chipletIdx]["numPin"][1].asInt());
    }
    // pins
    std::cout << "pin init..." << std::endl;
    for (auto js_pin : _json_database["pins"]) {
        Pin* pin_i = _db.allocatePin(js_pin["id"].asInt(), js_pin["name"].asString());
        pin_i->setShape(js_pin["ioShape"]["xLo"].asInt(), js_pin["ioShape"]["yLo"].asInt(), js_pin["ioShape"]["xHi"].asInt(), js_pin["ioShape"]["yHi"].asInt());
        pin_i->setOffset(js_pin["offset"][0].asInt(), js_pin["offset"][1].asInt());
    }
    // nets
    std::cout << "net init..." << std::endl;
    for (auto js_net : _json_database["nets"]) {
        Net* net_i = _db.allocateNet(js_net["id"].asInt(), js_net["name"].asString());
        net_i->setPower(js_net["isPower"].asInt()!=0);
    }

    std::cout << "tsv init..." << std::endl;
    for (auto js_tsv : _json_interposer["TSVs"]) {
        TSV* tsv_i = _db.allocateTSV(js_tsv["id"].asInt(), js_tsv["name"].asString());
        tsv_i->setOffset(js_tsv["loc"][0].asInt(), js_tsv["loc"][1].asInt()); 
        tsv_i->setSize(js_tsv["size"][0].asInt(), js_tsv["size"][1].asInt()); 
        tsv_i->inChiplet() = (js_tsv["inChiplet"].asInt() == 1);
        tsv_i->capDensity() = js_tsv["capDensity"].asDouble();
        // tsv_i->setCurrent(); 
    }

    // association
    std::cout << "generate association..." << std::endl;
    for (auto js_chiplet : _json_database["chiplets"]) {
        Chiplet* chiplet_i = _db.getChipletwithId(js_chiplet["id"].asInt());
        for (auto pinIdx : js_chiplet["pins_id"]) {
            Pin* pin = _db.getPinwithId(pinIdx.asInt());
            chiplet_i->addPin(pin);
        }
    }
    for (auto js_pin : _json_database["pins"]) {
        Pin* pin_i = _db.getPinwithId(js_pin["id"].asInt());
        Chiplet* chiplet = _db.getChipletwithId(js_pin["cell_id"].asInt());
        Net* net = _db.getNetwithId(js_pin["net_id"].asInt());
        pin_i->setChiplet(chiplet);
        pin_i->setNet(net);
        // js_pin["capDensity"].asDouble() is number_density
        RealType pin_xdistance = chiplet->getWidth()/chiplet->getXNumPins(); 
        RealType pin_ydistance = chiplet->getHeight()/chiplet->getYNumPins();
        RealType pin_size = pin_i->getXIOShape(); 
        RealType dtc_size = global_parameter.dtc_size;

        RealType numCap = js_pin["capDensity"].asDouble() * (pin_xdistance) * (pin_ydistance);
        pin_i->type_power() = js_pin["typePower"].asInt();

        if (pin_i->type_power() == 0) {
            numCap = std::max(numCap, 1.0);
        } else {
            // numCap = numCap + 5; // case0
            numCap = numCap + 9; // case_ai
        }

        pin_i->numCap() = numCap;

        if (CONSIDER_DTC_BLKG) {
            if (pin_i->numCap() <=8) {
                pin_i->esc1_spcace() = 0;
            } else {
                RealType esc_num = (pin_i->numCap()-8.0) * global_parameter.dtc_size / 5 / (global_parameter.line_width+global_parameter.line_pitch);
                pin_i->esc1_spcace() = static_cast<IndexType>(esc_num+1) * (global_parameter.line_width+global_parameter.line_pitch);
            }
        } else {
            pin_i->esc1_spcace() = 0;
        }
        // std::cout << "pin_i->esc1_spcace(): " << pin_i->esc1_spcace() << std::endl;
    }
    
    for (auto js_net : _json_database["nets"]) {
        Net* net_i = _db.getNetwithId(js_net["id"].asInt());
        for (auto pinIdx : js_net["pins_id"]) {
            Pin* pin = _db.getPinwithId(pinIdx.asInt());
            net_i->addPin(pin);
        }
    } 

    std::cout << "database generate success. " << std::endl;
}

