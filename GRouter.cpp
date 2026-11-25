#include<iostream>
#include<fstream>
#include<string>
#include<utility>
#include<vector>
#include<map>
#include<set>
#include<chrono>

#include "global.h"
#include "GRouter.h"
#include "RDL.h"
#include "DTC.h"

/*
Json::Value BIPGRouter::ReadJSON(std::string filename) {
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

bool BIPGRouter::WriteJSON(const std::string& filename, const Json::Value& root) {
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

void BIPGRouter::GenetateDB() {
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
        chiplet_i->setXYNumPins(_json_datadescript["chiplets"][chipletIdx]["numPins"][0].asInt(), 
                                _json_datadescript["chiplets"][chipletIdx]["numPins"][1].asInt());
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
*/
void BIPGRouter::ClusterPairGenetation() {
    for (Net net : _db.nets()) {
        if (net.getId() == 0 || net.getId() == 1 || net.getPins().size() < 2) {
            continue;
        } 
        std::vector<Pin*> pins = net.getPins();
        bool is_added_to_clusterpair = false;
        for (ClusterPairType& cpair : _vcluster_pair) {
            is_added_to_clusterpair = cpair.addPinPair(pins[0], pins[1]);
            if (is_added_to_clusterpair) { break; }
        }
        if (!is_added_to_clusterpair) {
            ClusterPairType cpair = ClusterPairType(pins[0]->getChiplet(), pins[1]->getChiplet());
            cpair.powerPinsInit(pins[0]->type_power(), pins[1]->type_power());
            cpair.addPinPair(pins[0], pins[1]);
            _vcluster_pair.emplace_back(cpair);
        }
    }
    for (ClusterPairType& cpair : _vcluster_pair) {
        cpair.pin_tmpId_setting(); // tmp_id: order of pin in cpair
        // cpair.print();
    }
}

bool BIPGRouter::ClusterPairEsc(ClusterPairType& cpair) {
    auto PhysicalMsg2varId = [&](IndexType netIdx, IndexType layerIdx, IndexType esc0, IndexType esc1) -> IndexType  {
        return (_numLayer*4)*netIdx + (4)*layerIdx + (2)*esc0 + esc1;
    };

    auto varId2PhysicalMsg = [&](IndexType id) -> std::vector<IndexType> {
        std::vector<IndexType> v_tmp;
        const std::array<IndexType, 4> divisors = {_numLayer*4, 4, 2, 1};
        
        for (const auto& divisor : divisors) {
            IndexType tmp = id / divisor;
            id %= divisor;
            v_tmp.emplace_back(tmp);
        }
        return v_tmp;
    };

    // variable init
    ILP_Solver solver;
    solver.addBinaryVariables(cpair.pins0().size() * _numLayer * 2 * 2);
    std::cout << "generate constraints... " << std::endl;
    Constraints cons;
    // one net constraints
    for (IndexType netIdx=0; netIdx<cpair.pins0().size(); netIdx++) {
        Constraint& con = cons.addConstraint();
        for (IndexType layerIdx=0; layerIdx<_numLayer; layerIdx++) {
            for (IndexType esc_0=0; esc_0<2; esc_0++) {
                for (IndexType esc_1=0; esc_1<2; esc_1++) {
                    con.varId_coeff_pair.emplace_back(VarIdCoeffPairType(PhysicalMsg2varId(netIdx, layerIdx, esc_0, esc_1), LocType(1)));
                }
            }
        }
        con.contype = ConType::Equal;
        con.objective = 1;
    }

    // capacitor constraints
    for (IndexType idx = 0; idx < 2; idx++) {
        Chiplet*       chiplet = idx == 0 ? cpair.chiplet0()   : cpair.chiplet1();
        std::vector<Pin*> pins = idx == 0 ? cpair.pins0()      : cpair.pins1();
        Direction    direction = idx == 0 ? cpair.direction0() : cpair.direction1();
        SortedPinsType& sortedPins = idx == 0 ? cpair.sorted_pins0() : cpair.sorted_pins1(); 
        for (const auto& pin : pins) {
            if (direction == Direction::U || direction == Direction::D) {
                sortedPins[pin->getXOffset()].emplace_back(pin);
            } else {
                sortedPins[pin->getYOffset()].emplace_back(pin);
            }
        }
        for (auto it = sortedPins.begin(); it != sortedPins.end(); ++it) {
            auto it_next = std::next(it);
            if (it == sortedPins.begin()) {
                for (IndexType layerIdx = 0; layerIdx < _numLayer; layerIdx++) {
                    Constraint& con = cons.addConstraint();
                    for (Pin* pin : it->second) {
                        if (idx == 0) {
                            con.varId_coeff_pair.emplace_back(VarIdCoeffPairType(PhysicalMsg2varId(pin->id_tmp(), layerIdx, 0, 0), LocType(1)));
                            con.varId_coeff_pair.emplace_back(VarIdCoeffPairType(PhysicalMsg2varId(pin->id_tmp(), layerIdx, 0, 1), LocType(1)));
                        } else {
                            con.varId_coeff_pair.emplace_back(VarIdCoeffPairType(PhysicalMsg2varId(pin->id_tmp(), layerIdx, 0, 0), LocType(1)));
                            con.varId_coeff_pair.emplace_back(VarIdCoeffPairType(PhysicalMsg2varId(pin->id_tmp(), layerIdx, 1, 0), LocType(1)));
                        }
                    }
                    con.contype = ConType::LessEq;
                    con.objective = chiplet->getWidth() * _numLayer * 1.0 / chiplet->getXNumPins() / 2;
                }
            }
            if (it_next == sortedPins.end()) {
                for (IndexType layerIdx = 0; layerIdx < _numLayer; layerIdx++) {
                    Constraint& con = cons.addConstraint();
                    for (Pin* pin : it->second) {
                        if (idx == 0) {
                            con.varId_coeff_pair.emplace_back(VarIdCoeffPairType(PhysicalMsg2varId(pin->id_tmp(), layerIdx, 1, 0), LocType(1)));
                            con.varId_coeff_pair.emplace_back(VarIdCoeffPairType(PhysicalMsg2varId(pin->id_tmp(), layerIdx, 1, 1), LocType(1)));
                        } else {
                            con.varId_coeff_pair.emplace_back(VarIdCoeffPairType(PhysicalMsg2varId(pin->id_tmp(), layerIdx, 0, 1), LocType(1)));
                            con.varId_coeff_pair.emplace_back(VarIdCoeffPairType(PhysicalMsg2varId(pin->id_tmp(), layerIdx, 1, 1), LocType(1)));
                        }
                    }
                    con.contype = ConType::LessEq;
                    con.objective = chiplet->getWidth() * 1.0 / chiplet->getXNumPins() / 2;
                }
                break;
            }
            for (IndexType layerIdx = 0; layerIdx < _numLayer; layerIdx++) {
                Constraint& con = cons.addConstraint();
                for (Pin* pin : it->second) {
                    if (idx == 0) {
                        con.varId_coeff_pair.emplace_back(VarIdCoeffPairType(PhysicalMsg2varId(pin->id_tmp(), layerIdx, 1, 0), LocType(1)));
                        con.varId_coeff_pair.emplace_back(VarIdCoeffPairType(PhysicalMsg2varId(pin->id_tmp(), layerIdx, 1, 1), LocType(1)));
                    } else {
                        con.varId_coeff_pair.emplace_back(VarIdCoeffPairType(PhysicalMsg2varId(pin->id_tmp(), layerIdx, 0, 1), LocType(1)));
                        con.varId_coeff_pair.emplace_back(VarIdCoeffPairType(PhysicalMsg2varId(pin->id_tmp(), layerIdx, 1, 1), LocType(1)));
                    }
                }
                for (Pin* pin : it_next->second) {
                    if (idx == 0) {
                        con.varId_coeff_pair.emplace_back(VarIdCoeffPairType(PhysicalMsg2varId(pin->id_tmp(), layerIdx, 0, 0), LocType(1)));
                        con.varId_coeff_pair.emplace_back(VarIdCoeffPairType(PhysicalMsg2varId(pin->id_tmp(), layerIdx, 0, 1), LocType(1)));
                    } else {
                        con.varId_coeff_pair.emplace_back(VarIdCoeffPairType(PhysicalMsg2varId(pin->id_tmp(), layerIdx, 0, 0), LocType(1)));
                        con.varId_coeff_pair.emplace_back(VarIdCoeffPairType(PhysicalMsg2varId(pin->id_tmp(), layerIdx, 1, 0), LocType(1)));
                    }
                }
                con.contype = ConType::LessEq;
                con.objective = (it_next->first - it->first) * 1.0 / (global_parameter.line_width + global_parameter.line_pitch);
            }

        }
    }

    // non cross constraints
    for (IndexType pinIdx0=0; pinIdx0<cpair.pins0().size(); pinIdx0++) {
        for (IndexType pinIdx1=pinIdx0+1; pinIdx1<cpair.pins0().size(); pinIdx1++) {
            std::vector<std::pair<IndexType, IndexType>> ch0_lower, ch0_upper, ch1_lower, ch1_upper;
            for (IndexType idx = 0; idx < 4; idx++) {
                IndexType esc_0 = (idx >> 0) & 1;
                IndexType esc_1 = (idx >> 1) & 1;

                // classification of net0 upper and lower, net1 upper and lower
                if (cpair.judge_front_func0(cpair.pins0()[pinIdx0], cpair.pins0()[pinIdx1], esc_0, esc_1)) {
                    ch0_upper.emplace_back(std::pair<IndexType, IndexType>(esc_0, esc_1));
                } else {
                    ch0_lower.emplace_back(std::pair<IndexType, IndexType>(esc_0, esc_1));
                }
                if (cpair.judge_front_func1(cpair.pins1()[pinIdx0], cpair.pins1()[pinIdx1], esc_0, esc_1)) {
                    ch1_upper.emplace_back(std::pair<IndexType, IndexType>(esc_0, esc_1));
                } else {
                    ch1_lower.emplace_back(std::pair<IndexType, IndexType>(esc_0, esc_1));
                }
            }


            // if (ch0_upper.size() == 4 && ch1_upper.size() == 4 || ch0_upper.size() == 0 && ch1_upper.size() == 0) {
            //     // non-cross
            //     continue;
            // } else if (ch0_upper.size() == 4 && ch1_upper.size() == 0 || ch0_upper.size() == 0 && ch1_upper.size() == 4) {
            //     // cross
            //     for (IndexType layerIdx = 0; layerIdx < _numLayer; layerIdx++) {
            //         Constraint& con = cons.addConstraint();
            //         for (IndexType idx = 0; idx < 4; idx++) {
            //             IndexType esc_0 = (idx >> 0) & 1;
            //             IndexType esc_1 = (idx >> 1) & 1;
            //             con.varId_coeff_pair.emplace_back(VarIdCoeffPairType(PhysicalMsg2varId(pinIdx0, layerIdx, esc_0, esc_1), 1));
            //             con.varId_coeff_pair.emplace_back(VarIdCoeffPairType(PhysicalMsg2varId(pinIdx1, layerIdx, esc_0, esc_1), 1));
            //         }
            //         con.contype = ConType::LessEq;
            //         con.objective = 1;
            //     }
            // } else {
                // 
            /* 
            for (IndexType layerIdx = 0; layerIdx < _numLayer; layerIdx++) {
                if (ch0_lower.size() != 0 && ch1_upper.size() != 0) {
                    Constraint& con = cons.addConstraint();
                    for (auto esc_pair : ch0_lower) {
                        con.varId_coeff_pair.emplace_back(VarIdCoeffPairType(PhysicalMsg2varId(pinIdx0, layerIdx, esc_pair.first, esc_pair.second), 1));
                    }
                    for (auto esc_pair : ch1_upper) {
                        con.varId_coeff_pair.emplace_back(VarIdCoeffPairType(PhysicalMsg2varId(pinIdx1, layerIdx, esc_pair.first, esc_pair.second), 1));
                    }
                    con.contype = ConType::LessEq;
                    con.objective = 1;
                }
            }
            for (IndexType layerIdx = 0; layerIdx < _numLayer; layerIdx++) {
                if (ch0_upper.size() != 0 && ch1_lower.size() != 0) {
                    // constraints aer wrong!!!!!
                    Constraint& con = cons.addConstraint();
                    for (auto esc_pair : ch0_upper) {
                        con.varId_coeff_pair.emplace_back(VarIdCoeffPairType(PhysicalMsg2varId(pinIdx0, layerIdx, esc_pair.first, esc_pair.second), 1));
                    }
                    for (auto esc_pair : ch1_lower) {
                        con.varId_coeff_pair.emplace_back(VarIdCoeffPairType(PhysicalMsg2varId(pinIdx1, layerIdx, esc_pair.first, esc_pair.second), 1));
                    }
                    con.contype = ConType::LessEq;
                    con.objective = 1;
                }
            }
            // }
            */
            for (IndexType layerIdx = 0; layerIdx < _numLayer; layerIdx++) {
                for (IndexType idx = 0; idx < 2; idx++) {
                    std::vector<std::pair<IndexType, IndexType>>& ch0 = idx == 0 ? ch0_lower : ch0_upper;
                    std::vector<std::pair<IndexType, IndexType>>& ch1 = idx == 0 ? ch1_upper : ch1_lower;
                    
                    std::set<IndexType> cons_var_ids;
                    for (auto ch0_pair : ch0) {
                        for (auto ch1_pair : ch1) {
                            cons_var_ids.insert(PhysicalMsg2varId(pinIdx0, layerIdx, ch0_pair.first,  ch1_pair.first));
                            cons_var_ids.insert(PhysicalMsg2varId(pinIdx1, layerIdx, ch0_pair.second, ch1_pair.second));
                        }
                    }
                    Constraint& con = cons.addConstraint();
                    for (IndexType var : cons_var_ids) {
                        con.varId_coeff_pair.emplace_back(VarIdCoeffPairType(var, 1));
                    }
                    con.contype = ConType::LessEq;
                    con.objective = 1;
                }
            }
        }
    }

    // minimize layer
    std::vector<RealType> obj_coeff;
    for (IndexType pinIdx=0; pinIdx<cpair.pins0().size(); pinIdx++) {
        for (IndexType layerIdx=0; layerIdx<_numLayer; layerIdx++) {
            for (IndexType esc0=0; esc0<2; esc0++) {
                for (IndexType esc1=0; esc1<2; esc1++) {
                    obj_coeff.emplace_back(_numLayer-layerIdx);
                }
            }
        }
    }

    solver.ILPaddConstraints(cons);
    solver.setMaximizeObjective(obj_coeff);
    // solver.setMaximizeObjective(std::vector<double>(cpair.pins0().size()*_numLayer*2*2, 1.0));
    std::cout << "solve ILP poblem... " << std::endl;
    if (!solver.solve()) {
        std::cout << "No solution found!" << std::endl;
        return false;
    }
    std::cout << "Objective Value: " << solver.getObjectiveValue() << std::endl;
    std::vector<IndexType> solution = solver.getSolution();
    cpair.solution() = solution;
    // cpair.print_solution();
    // 
    for (IndexType idx=0; idx<cpair.solution().size(); idx++) { 
        if (cpair.solution()[idx] == 0) { continue; }
        std::vector<IndexType> physical_msg = varId2PhysicalMsg(idx);
        Pin* pin0 = cpair.pins0()[physical_msg[0]]; pin0->routeLayer() = physical_msg[1]; pin0->escape01() = physical_msg[2];
        Pin* pin1 = cpair.pins1()[physical_msg[0]]; pin1->routeLayer() = physical_msg[1]; pin1->escape01() = physical_msg[3];
        // std::cout << idx << ": " << physical_msg[0] << ", " << physical_msg[1] << ", " << physical_msg[2] << ", " << physical_msg[3] << std::endl;
        // std::cout << idx << ": " << physical_msg[0] << ", " << pin0->routeLayer() << ", " << pin0->escape01() << ", " << pin1->escape01() << std::endl;
    }
    return true;
}

bool BIPGRouter::EscapePlanning(IndexType numLayer) {
    std::cout << "--------------- numLayer = " << numLayer << " ---------------" << std::endl;
    setnumLayer(numLayer);
    for (auto& cpair : _vcluster_pair) {
        auto time_start = std::chrono::high_resolution_clock::now();
        if (!ClusterPairEsc(cpair)) { 
            
            std::cout << "------------ numLayer = " << numLayer << " failed. ------------" << std::endl;
            return false; 
        }
        auto time_end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = time_end - time_start;
        std::cout << "----- Elapsed time: " << elapsed.count() << " s -----" << std::endl;

    }
    std::cout << "----------- numLayer = " << numLayer << " success. -----------" << std::endl;
    return true;
}

IndexType BIPGRouter::FindMinimumLayersWithBinarySearch() {
    IndexType numLayer_lower = 0, numLayer_upper = 10;
    // EscapePlanning();
    while (!EscapePlanning(numLayer_upper)) {
        numLayer_lower = numLayer_upper; 
        numLayer_upper += 1;
    }
    while (numLayer_upper - numLayer_lower > 1 ) {
        IndexType numLayer_mid = (numLayer_lower + numLayer_upper) / 2;
        if (EscapePlanning(numLayer_mid)) {
            numLayer_upper = numLayer_mid;
        } else {
            numLayer_lower = numLayer_mid;
        }
    }
    EscapePlanning(numLayer_upper);
    std::cout << "-- find minimun layer: " << numLayer_upper << std::endl;
    return numLayer_upper;
}

void BIPGRouter::EscapeRouting() {
    for (ClusterPairType& cpair : _vcluster_pair) {
        // for (auto it = layer_sorted_pins.begin(); it != layer_sorted_pins.end(); ++it) {
        //     SortedPinPair xy_sorted_pins;
        //     for (auto pinpair : it->second) {
        //     }
        // }
        // classification by layer
        SortedPinPairType layer_sorted_pin_pair;
        for (IndexType netIdx=0; netIdx<cpair.pins0().size(); netIdx++) {
            Pin* pin0 = cpair.pins0()[netIdx];
            Pin* pin1 = cpair.pins1()[netIdx];
            layer_sorted_pin_pair[pin0->routeLayer()].emplace_back(std::pair<Pin*, Pin*>(pin0, pin1));
        }
        // add element to layer
        for (auto it = layer_sorted_pin_pair.begin(); it != layer_sorted_pin_pair.end(); it++) {
            SortedPinsType sp0, sp1;
            Direction d0 = cpair.direction0(), d1 = cpair.direction1();
            for (auto pin_pair : it->second) {
                if (d0 == Direction::U || d0 == Direction::D) sp0[pin_pair.first->getXOffset()].emplace_back(pin_pair.first); 
                else sp0[pin_pair.first->getYOffset()].emplace_back(pin_pair.first);
                if (d1 == Direction::U || d1 == Direction::D) sp1[pin_pair.second->getXOffset()].emplace_back(pin_pair.second); 
                else sp1[pin_pair.second->getYOffset()].emplace_back(pin_pair.second);
            }
            _vlayer[it->first].all_pin_esc_pair().emplace_back(std::make_pair(std::make_pair(sp0, d0), std::make_pair(sp1, d1)));
        }
    }

    std::cout << "generate tracks... " << std::endl;
    // in layer
    for (Layer& layer : _vlayer) {
        for (PinEscPairType& pin_esc_pair : layer.all_pin_esc_pair()) {
            for (IndexType idx = 0; idx < 2; idx++) {
                PinEscType& pin_esc = idx == 0 ? pin_esc_pair.first : pin_esc_pair.second;
                SortedPinsType& sorted_pin = pin_esc.first;
                Direction& direction = pin_esc.second;
                for (auto it = sorted_pin.begin(); it != sorted_pin.end(); it++) {
                    std::vector<Pin*> vpins = it->second;
                    if (vpins.size() == 0) { continue; }
                    Chiplet* chiplet = vpins[0]->getChiplet();
                    LocType blockage_space = 0;
                    for (Pin* pin : vpins) {
                        // blockage_space = (pin->esc1_spcace() < blockage_space) ? blockage_space : pin->esc1_spcace();
                        blockage_space = std::max(pin->esc1_spcace(), blockage_space);
                    }
                    // std::cout << "blockage_space: " << blockage_space << std::endl;
                    if (direction == Direction::U) {
                        // y, from big to small
                        std::sort(vpins.begin(), vpins.end(), [](Pin* a, Pin* b) -> bool {return a->getYOffset() > b->getYOffset(); });
                        LocType y_boundary = 0 + chiplet->getHeight();
                        LocType x_offset = vpins[0]->getXOffset();
                        LocType x_shape  = vpins[0]->getXIOShape();
                        IndexType num_esc_0 = 1, num_esc_1 = 1;
                        for (Pin* pin : vpins) {
                            pin->escTrack().addPoint(pin->getXCenter(), pin->getYCenter());
                            if (pin->escape01() == 0) {
                                LocType delta_x = num_esc_0 * (global_parameter.line_width + global_parameter.line_pitch);
                                LocType x1 = x_offset - delta_x;
                                LocType y1 = pin->getYOffset() + pin->getYIOShape() + delta_x;
                                pin->escTrack().addPoint(x1, y1);
                                pin->escTrack().addPoint(x1, y_boundary);
                                num_esc_0++;
                            } else {
                                LocType delta_x = blockage_space + num_esc_1 * (global_parameter.line_width + global_parameter.line_pitch);
                                LocType x1 = x_offset + x_shape + delta_x;
                                LocType y1 = pin->getYOffset() + pin->getYIOShape() + delta_x;
                                pin->escTrack().addPoint(x1, y1);
                                pin->escTrack().addPoint(x1, y_boundary);
                                num_esc_1++;
                            }
                        }
                    } else if (direction == Direction::D) {
                        // y, from small to big
                        std::sort(vpins.begin(), vpins.end(), [](Pin* a, Pin* b) -> bool {return a->getYOffset() < b->getYOffset(); });
                        LocType y_boundary = 0;
                        LocType x_offset = vpins[0]->getXOffset();
                        LocType x_shape  = vpins[0]->getXIOShape();
                        IndexType num_esc_0 = 1, num_esc_1 = 1;
                        for (Pin* pin : vpins) {
                            pin->escTrack().addPoint(pin->getXCenter(), pin->getYCenter());
                            if (pin->escape01() == 0) {
                                LocType delta_x = num_esc_0 * (global_parameter.line_width + global_parameter.line_pitch);
                                LocType x1 = x_offset - delta_x;
                                LocType y1 = pin->getYOffset() - delta_x;
                                pin->escTrack().addPoint(x1, y1);
                                pin->escTrack().addPoint(x1, y_boundary);
                                num_esc_0++;
                            } else {
                                LocType delta_x = blockage_space + num_esc_1 * (global_parameter.line_width + global_parameter.line_pitch);
                                LocType x1 = x_offset + x_shape + delta_x;
                                LocType y1 = pin->getYOffset() - delta_x;
                                pin->escTrack().addPoint(x1, y1);
                                pin->escTrack().addPoint(x1, y_boundary);
                                num_esc_1++;
                            }
                        }
                    } else if (direction == Direction::L) {
                        // x, from small to big
                        std::sort(vpins.begin(), vpins.end(), [](Pin* a, Pin* b) -> bool {return a->getXOffset() < b->getXOffset(); });
                        LocType x_boundary = 0;
                        LocType y_offset = vpins[0]->getYOffset();
                        LocType y_shape  = vpins[0]->getYIOShape();
                        IndexType num_esc_0 = 1, num_esc_1 = 1;
                        for (Pin* pin : vpins) {
                            pin->escTrack().addPoint(pin->getXCenter(), pin->getYCenter());
                            if (pin->escape01() == 0) {
                                LocType delta_y = num_esc_0 * (global_parameter.line_width + global_parameter.line_pitch);
                                LocType y1 = y_offset - delta_y;
                                LocType x1 = pin->getXOffset() - delta_y;
                                pin->escTrack().addPoint(x1, y1);
                                pin->escTrack().addPoint(x_boundary, y1);
                                num_esc_0++;
                            } else {
                                LocType delta_y = blockage_space + num_esc_1 * (global_parameter.line_width + global_parameter.line_pitch);
                                LocType y1 = y_offset + y_shape + delta_y;
                                LocType x1 = pin->getXOffset() - delta_y;
                                pin->escTrack().addPoint(x1, y1);
                                pin->escTrack().addPoint(x_boundary, y1);
                                num_esc_1++;
                            }
                        }
                    } else if (direction == Direction::R) {
                        // x, from big to small
                        std::sort(vpins.begin(), vpins.end(), [](Pin* a, Pin* b) -> bool { return a->getXOffset() > b->getXOffset(); });
                        LocType x_boundary = 0 + chiplet->getWidth();
                        LocType y_offset = vpins[0]->getYOffset();
                        LocType y_shape  = vpins[0]->getYIOShape();
                        IndexType num_esc_0 = 1, num_esc_1 = 1;
                        for (Pin* pin : vpins) {
                            pin->escTrack().addPoint(pin->getXCenter(), pin->getYCenter());
                            if (pin->escape01() == 0) {
                                LocType delta_y = num_esc_0 * (global_parameter.line_width + global_parameter.line_pitch);
                                LocType y1 = y_offset - delta_y;
                                LocType x1 = pin->getXOffset() + pin->getXIOShape() + delta_y;
                                pin->escTrack().addPoint(x1, y1);
                                pin->escTrack().addPoint(x_boundary, y1);
                                num_esc_0++;
                            } else {
                                LocType delta_y = blockage_space + num_esc_1 * (global_parameter.line_width + global_parameter.line_pitch);
                                LocType y1 = y_offset + y_shape + delta_y;
                                LocType x1 = pin->getXOffset() + pin->getXIOShape() + delta_y;
                                pin->escTrack().addPoint(x1, y1);
                                pin->escTrack().addPoint(x_boundary, y1);
                                num_esc_1++;
                            }
                        }
                    }
                }
            }
        }
    }
    // concrete track with same net (directly)
    for (Net& net : _db.nets()) {
        if (net.getId() == 0 || net.getId() == 1 || net.getPins().size() < 2) { continue; } 
        std::vector<Pin*> pins = net.getPins();
        IndexType pin_rout_layer = pins[0]->routeLayer();
        Track& track = _vlayer[pin_rout_layer].new_track();
        // pin0
        Chiplet* chiplet = pins[0]->getChiplet();
        for (IndexType idx = 0; idx < pins[0]->escTrack().xTrack().size(); idx++) {
            track.addPoint(chiplet->getXLoc()+pins[0]->escTrack().xTrack().at(idx), chiplet->getYLoc()+pins[0]->escTrack().yTrack().at(idx));
        }
        // pin1
        chiplet = pins[1]->getChiplet();
        for (IndexType idx = pins[1]->escTrack().xTrack().size(); idx > 0; idx--) {
            track.addPoint(chiplet->getXLoc()+pins[1]->escTrack().xTrack().at(idx-1), chiplet->getYLoc()+pins[1]->escTrack().yTrack().at(idx-1));
        }
    }
}

Json::Value BIPGRouter::TrackOutput() {
    std::cout << "save routing result... " << std::endl;
    Json::Value json_root;
    Json::Value json_layer(Json::arrayValue);
    for (Layer layer_i : _vlayer) {
        if (layer_i.all_track().size() == 0) { continue; }
        Json::Value json_layer_i;
        json_layer_i["id"] = layer_i.id();
        Json::Value json_layer_i_track(Json::arrayValue);
        for (Track& track : layer_i.all_track()) {
            Json::Value json_layer_i_track_i(Json::arrayValue);
            for (IndexType idx = 0; idx < track.xTrack().size(); idx++) {
                Json::Value elem(Json::arrayValue);
                elem.append(track.xTrack().at(idx)); 
                elem.append(track.yTrack().at(idx)); 
                json_layer_i_track_i.append(elem);
                // json_layer_i_track_i.append(Json::Value({track.xTrack().at(idx), track.yTrack().at(idx)}));
            }
            json_layer_i_track.append(json_layer_i_track_i);
        }
        json_layer_i["track"] = json_layer_i_track;
        json_layer.append(json_layer_i);
    }
    std::cout << "number of layer: " << json_layer.size() << std::endl;
    json_root["layer"] = json_layer;
    if (_all_dtc_loc.size() != 0) {
        json_root["dtc"] = DTCOutput();
    }
    return json_root;
    // if (!WriteJSON("output_track.json", json_root)) {
    //     std::cerr << "Failed to write JSON data." << std::endl;
    // }
    // Json::StreamWriterBuilder writerBuilder;
    // std::string jsonString = Json::writeString(writerBuilder, json_root);
    // std::cout << "Final JSON String: " << jsonString << std::endl;

    // if (!WriteJSON("output_track.json", json_root)) {
    //     std::cerr << "Failed to write JSON data." << std::endl;
    // }
}

void BIPGRouter::DTCInsert() {
    // TODO: clear data
    //
    std::set<std::pair<LocType, LocType>> unique_pairs;
    for (auto chiplet : _db.chiplets()) {
        // assume x = y
        LocType pin_distance = chiplet.getWidth() / chiplet.getXNumPins();
        LocType pin_size = chiplet.getPins()[0]->getXIOShape();
        unique_pairs.insert(std::make_pair(pin_distance, pin_size));
    }
    // generate template
    std::cout << "number of pins type: " << unique_pairs.size() << std::endl;
    for (auto pair : unique_pairs) {
        // different routing blockage template creation
        for (LocType blockage=0; blockage<=(pair.first - pair.second); blockage+=(global_parameter.line_width + global_parameter.line_pitch)) {
            _dtc_template.CreateTemplate(pair.first, pair.second, blockage+global_parameter.line_pitch, global_parameter.dtc_size);
        }
        // power region template creation
        _dtc_template.CreateTemplate(pair.first, pair.second, -1, global_parameter.dtc_size);
    }
    // // TSV
    // LocType tsv_xdistance, LocType tsv_ydistance, LocType tsv_size, LocType rectshape
    _dtc_template.CreateTSVTemplate(_db.global().TSVXDistance(), _db.global().TSVYDistance(), _db.tsvs()[0].getWidth(), global_parameter.dtc_size);

    // deal with routing pins
    for (ClusterPairType& cpair : _vcluster_pair) {
        for (IndexType idx = 0; idx < 2; idx++) {
            Chiplet*       chiplet  = idx == 0 ? cpair.chiplet0()   : cpair.chiplet1();
            std::vector<Pin*> pins  = idx == 0 ? cpair.pins0()      : cpair.pins1();
            std::vector<Pin*> ppins = idx == 0 ? cpair.ppins0()     : cpair.ppins1();
            Direction    direction  = idx == 0 ? cpair.direction0() : cpair.direction1();

            pins.insert(pins.end(), ppins.begin(), ppins.end());
            LocType pin_distance = chiplet->getWidth() / chiplet->getXNumPins();
            LocType pin_size = pins[0]->getXIOShape();

            for (auto pin : pins) {
                // std::cout << "pin->numCap(): " << pin->numCap() << std::endl;
                LocType pin_xloc  = pin->getXOffset() , pin_yloc  = pin->getYOffset();
                LocType pin_xsize = pin->getXIOShape(), pin_ysize = pin->getYIOShape();
                IndexType num_template = pin->esc1_spcace() / (global_parameter.line_width + global_parameter.line_pitch);
                LocType blockage = num_template * (global_parameter.line_width + global_parameter.line_pitch);
                RectInsert ins = _dtc_template.getTemplate(pin_distance, pin_size, blockage+global_parameter.line_pitch);
                // std::cout << "need: " << pin->numCap() << ", " << std::endl; 
                if (pin->getId() == 1) { 
                    std::cout << pin->esc1_spcace() << std::endl;
                }
                if (direction == Direction::U) {
                    // std::vector<LocXYType> all_dtc_relative_loc = ins.allRect();
                    std::vector<LocXYType> all_dtc_relative_loc = ins.selectRandomElements(ins.allRect(), 5+pin->numCap());
                    for (auto dtc_relative_loc : all_dtc_relative_loc) {
                        LocType xx = dtc_relative_loc.first + pin_xloc + chiplet->getXLoc();
                        LocType yy = dtc_relative_loc.second + pin_yloc + pin_ysize + chiplet->getYLoc();
                        _all_dtc_loc.emplace_back(LocXYType(xx, yy));
                    }
                } else if (direction == Direction::D) {
                    // std::vector<LocXYType> all_dtc_relative_loc = ins.flipXaxes();
                    std::vector<LocXYType> all_dtc_relative_loc = ins.selectRandomElements(ins.flipXaxes(), 5+pin->numCap());
                    for (auto dtc_relative_loc : all_dtc_relative_loc) {
                        LocType xx = dtc_relative_loc.first + pin_xloc + chiplet->getXLoc();
                        LocType yy = dtc_relative_loc.second + pin_yloc + chiplet->getYLoc();
                        _all_dtc_loc.emplace_back(LocXYType(xx, yy));
                    }
                } else if (direction == Direction::L) {
                    // std::vector<LocXYType> all_dtc_relative_loc = ins.invRotate90();
                    std::vector<LocXYType> all_dtc_relative_loc = ins.selectRandomElements(ins.invRotate90(), 5+pin->numCap());
                    for (auto dtc_relative_loc : all_dtc_relative_loc) {
                        LocType xx = dtc_relative_loc.first + pin_xloc + chiplet->getXLoc();
                        LocType yy = dtc_relative_loc.second + pin_yloc + chiplet->getYLoc();
                        _all_dtc_loc.emplace_back(LocXYType(xx, yy));
                    }
                } else if (direction == Direction::R) {
                    // std::vector<LocXYType> all_dtc_relative_loc = ins.invRotate90andFlipYaxes();
                    std::vector<LocXYType> all_dtc_relative_loc = ins.selectRandomElements(ins.invRotate90andFlipYaxes(), 5+pin->numCap());
                    for (auto dtc_relative_loc : all_dtc_relative_loc) {
                        LocType xx = dtc_relative_loc.first + pin_xloc + pin_xsize + chiplet->getXLoc();
                        LocType yy = dtc_relative_loc.second + pin_yloc + chiplet->getYLoc();
                        _all_dtc_loc.emplace_back(LocXYType(xx, yy));
                    }
                } else {
                    throw std::invalid_argument("direction error. ");
                }
            }
        }
    }

    // deal with power pins (in cluster)
    // cluster power
    
    // deal with power pins (not in cluster)
    for (auto& pin : _db.pins()) {
        if (pin.type_power() != 0) { continue; }
        Chiplet* chiplet = pin.getChiplet();
        LocType pin_size = pin.getXIOShape();
        LocType pin_distance = chiplet->getWidth() / chiplet->getXNumPins();
        RectInsert ins = _dtc_template.getTemplate(pin_distance, pin_size, -1);
        std::vector<LocXYType> all_dtc_relative_loc = ins.selectRandomElements(ins.allRect(), 5+pin.numCap());
        for (auto dtc_relative_loc : all_dtc_relative_loc) {
            LocType xx = dtc_relative_loc.first  + pin.getXOffset() + chiplet->getXLoc();
            LocType yy = dtc_relative_loc.second + pin.getYOffset() + chiplet->getYLoc();
            _all_dtc_loc.emplace_back(LocXYType(xx, yy));
        }
    }
    // deal with TSV
    for (auto& tsv : _db.tsvs()) {
        if (tsv.inChiplet()) { continue; }
        RectInsert ins = _dtc_template.getTSVTemplate();
        std::vector<LocXYType> all_dtc_relative_loc = ins.selectRandomElements(ins.allRect(), 5+tsv.capDensity()*_db.global().TSVXDistance()*_db.global().TSVYDistance());
        // std::cout << "num DTC around TSV" << tsv.capDensity()*_db.global().TSVXDistance()*_db.global().TSVYDistance() << std::endl;
        for (auto dtc_relative_loc : all_dtc_relative_loc) {
            LocType tsv_offset_x = (_db.global().TSVXDistance() - tsv.getWidth()) / 2;
            LocType tsv_offset_y = (_db.global().TSVYDistance() - tsv.getHeight()) / 2;
            LocType xx = dtc_relative_loc.first  + tsv.getXOffset() - tsv_offset_x;
            LocType yy = dtc_relative_loc.second + tsv.getYOffset() - tsv_offset_y;
            _all_dtc_loc.emplace_back(LocXYType(xx, yy));
        }
    }

}

Json::Value BIPGRouter::DTCOutput() {
    Json::Value json_root(Json::arrayValue);
    IndexType dtc_idx = 0;
    for (auto dtc_loc : _all_dtc_loc) {
        Json::Value json_dtc;
        // // id
        json_dtc["id"] = dtc_idx;
        // // location
        Json::Value json_dtc_loc(Json::arrayValue);
        json_dtc_loc.append(dtc_loc.first);
        json_dtc_loc.append(dtc_loc.second);
        json_dtc["loc"] = json_dtc_loc;
        // // size
        Json::Value json_dtc_size(Json::arrayValue);
        json_dtc_size.append(global_parameter.dtc_size);
        json_dtc_size.append(global_parameter.dtc_size);
        json_dtc["size"] = json_dtc_size;
        // add this dtc to root
        json_root.append(json_dtc);
        dtc_idx++; 
    }
    return json_root;
}

void BIPGRouter::clear_rout() {
    for (auto& pin :_db.pins()) { pin.clear_track_msg(); }
    for (auto& cpair : _vcluster_pair) {
        cpair.sorted_pins0().clear(); 
        cpair.sorted_pins1().clear(); 
    }
    _vlayer.clear();
}


Json::Value BIPGRouter::output_database() {
    Json::Value json_root;

    Json::Value json_chiplets(Json::arrayValue);
    for (auto& c : _db.chiplets()) {
        Json::Value json_chiplet;
        json_chiplet["id"] = c.getId();
        json_chiplet["name"] = c.getName();
        // location
        Json::Value json_chiplet_loc(Json::arrayValue);
        json_chiplet_loc.append(c.getXLoc());
        json_chiplet_loc.append(c.getYLoc());
        json_chiplet["loc"] = json_chiplet_loc;
        // size
        Json::Value json_chiplet_size(Json::arrayValue);
        json_chiplet_size.append(c.getWidth());
        json_chiplet_size.append(c.getHeight());
        json_chiplet["size"] = json_chiplet_size;
        // numPin
        Json::Value json_chiplet_numpins(Json::arrayValue);
        json_chiplet_numpins.append(c.getXNumPins());
        json_chiplet_numpins.append(c.getYNumPins());
        json_chiplet["numPin"] = json_chiplet_numpins;
        // pins_id
        Json::Value json_chiplet_pinsid(Json::arrayValue);
        for (auto p : c.getPins()) {
            json_chiplet_pinsid.append(p->getId());
        }
        json_chiplet["pins_id"] = json_chiplet_pinsid;
        // add to chiplets
        json_chiplets.append(json_chiplet);
    }
    json_root["chiplets"] = json_chiplets;

    Json::Value json_pins(Json::arrayValue);
    for (auto& p : _db.pins()) {
        Json::Value json_pin;
        json_pin["id"] = p.getId();
        json_pin["name"] = p.getName();
        json_pin["cell_id"] = p.getChiplet()->getId();
        json_pin["net_id"]  = p.getNet()->getId();
        // offset
        Json::Value json_pin_offset(Json::arrayValue);
        json_pin_offset.append(p.getXOffset());
        json_pin_offset.append(p.getYOffset());
        json_pin["offset"]  = json_pin_offset;
        // ioShape
        Json::Value json_pin_ioShape;
        json_pin_ioShape["xLo"] = p.ioshape(0);
        json_pin_ioShape["yLo"] = p.ioshape(1);
        json_pin_ioShape["xHi"] = p.ioshape(2);
        json_pin_ioShape["yHi"] = p.ioshape(3);
        json_pin["ioShape"]  = json_pin_ioShape;
        json_pin["typePower"]  = p.type_power();
        // add to pins
        json_pins.append(json_pin);
    }
    json_root["pins"] = json_pins;

    Json::Value json_nets(Json::arrayValue);
    for (auto& n : _db.nets()) {
        Json::Value json_net;
        json_net["id"] = n.getId();
        json_net["name"] = n.getName();
        json_net["isPower"] = n.ispower();
        // cells_id
        Json::Value json_net_cellsid(Json::arrayValue);
        Json::Value json_net_pinsid(Json::arrayValue);
        for (auto p : n.getPins()) {
            json_net_cellsid.append(p->getChiplet()->getId());
            json_net_pinsid.append(p->getId());
        }
        json_net["cells_id"] = json_net_cellsid;
        json_net["pins_id"]  = json_net_pinsid;
        // add to nets
        json_nets.append(json_net);
    }
    json_root["nets"] = json_nets;

    return json_root;
}
