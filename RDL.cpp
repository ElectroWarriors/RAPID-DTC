#include "RDL.h"
#include "ilp_solver.h"

void PinReDistribution::calculateChipletRDLPriority() {
    for (auto cpair : _vcluster_pair) {
        _freq[cpair.chiplet0()->getId()]++;
        _freq[cpair.chiplet1()->getId()]++;
    }
}

bool PinReDistribution::ILPnonOverFlow(IndexType numLayer){
    auto PhysicalMsg2varId = [&](IndexType netIdx, IndexType layerIdx, IndexType esc) -> IndexType  {
        return (numLayer*2)*netIdx + (2)*layerIdx + (1)*esc;
    };
    auto varId2PhysicalMsg = [&](IndexType id) -> std::vector<IndexType> {
        std::vector<IndexType> v_tmp;
        const std::array<IndexType, 3> divisors = {numLayer*2, 2, 1};
        
        for (const auto& divisor : divisors) {
            IndexType tmp = id / divisor;
            id %= divisor;
            v_tmp.emplace_back(tmp);
        }
        return v_tmp;
    };

    for (auto cpair : _vcluster_pair) {
        IndexType numPinPair = cpair.pins0().size();
        ILP_Solver solver;
        solver.addBinaryVariables(2 * numPinPair * numLayer * 2);
        std::cout << "generate constraints... " << std::endl;
        Constraints cons;

        // one net constraints, including pins0 and pins1
        for (IndexType pinIdx=0; pinIdx<2*numPinPair; pinIdx++) {
            Constraint& con = cons.addConstraint();
            for (IndexType layerIdx=0; layerIdx<numLayer; layerIdx++) {
                for (IndexType esc=0; esc<2; esc++) {
                    con.varId_coeff_pair.emplace_back(VarIdCoeffPairType(PhysicalMsg2varId(pinIdx, layerIdx, esc), LocType(1)));
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
            LocType max_capacity = (direction == Direction::U || direction == Direction::D) ? \
                                    chiplet->getWidth()  / chiplet->getXNumPins() - pins[0]->getXIOShape() : \
                                    chiplet->getHeight() / chiplet->getYNumPins() - pins[0]->getYIOShape();
            
            for (const auto& pin : pins) {
                if (direction == Direction::U || direction == Direction::D) {
                    sortedPins[pin->getXOffset()].emplace_back(pin);
                } else {
                    sortedPins[pin->getYOffset()].emplace_back(pin);
                }
            }
            for (auto it = sortedPins.begin(); it != sortedPins.end(); ++it) {
                auto it_next = std::next(it);
                LocType capacity = max_capacity;
                LocType blockage_cap = 0;
                for (Pin* pin : it->second) {
                    // blockage_cap = (pin->esc1_spcace() < blockage_cap) ? blockage_cap : pin->esc1_spcace();
                    blockage_cap = std::max(blockage_cap, pin->esc1_spcace());
                    // if (pin->esc1_spcace() > 0) { 
                    //     std::cout << std::endl; 
                    // }
                    // if (blockage_cap > capacity/2) { blockage_cap = capacity/2; break; }
                }
                blockage_cap = std::min(blockage_cap*1.0, capacity - (global_parameter.line_pitch + global_parameter.line_width));
                // update all blockage to max value
                for (Pin* pin : it->second) { pin->esc1_spcace() = blockage_cap; }

                capacity -= blockage_cap;
                capacity /= (global_parameter.line_pitch + global_parameter.line_width);
                // std::cout << "RDL capacity: " << capacity << std::endl;

                if (it == sortedPins.begin()) {
                    for (IndexType layerIdx = 0; layerIdx < numLayer; layerIdx++) {
                        Constraint& con = cons.addConstraint();
                        for (Pin* pin : it->second) {
                            con.varId_coeff_pair.emplace_back(VarIdCoeffPairType(PhysicalMsg2varId(pin->id_tmp()+idx*numPinPair, layerIdx, 0), LocType(1)));
                        }
                        con.contype = ConType::LessEq;
                        con.objective = capacity / 2;
                    }
                }
                if (it_next == sortedPins.end()) {
                    for (IndexType layerIdx = 0; layerIdx < numLayer; layerIdx++) {
                        Constraint& con = cons.addConstraint();
                        for (Pin* pin : it->second) {
                            con.varId_coeff_pair.emplace_back(VarIdCoeffPairType(PhysicalMsg2varId(pin->id_tmp()+idx*numPinPair, layerIdx, 1), LocType(1)));
                        }
                        con.contype = ConType::LessEq;
                        con.objective = capacity / 2;
                    }
                    break;
                }
                for (IndexType layerIdx = 0; layerIdx < numLayer; layerIdx++) {
                    Constraint& con = cons.addConstraint();
                    // IndexType blockage_cap = 1;
                    for (Pin* pin : it->second) {
                        con.varId_coeff_pair.emplace_back(VarIdCoeffPairType(PhysicalMsg2varId(pin->id_tmp()+idx*numPinPair, layerIdx, 1), LocType(1)));
                    }
                    for (Pin* pin : it_next->second) {
                        con.varId_coeff_pair.emplace_back(VarIdCoeffPairType(PhysicalMsg2varId(pin->id_tmp()+idx*numPinPair, layerIdx, 0), LocType(1)));
                    }
                    con.contype = ConType::LessEq;
                    con.objective = capacity;
                }
            }
        }

        // same layer same numpins
        for (IndexType layerIdx = 0; layerIdx < numLayer; layerIdx++) {
            Constraint& con = cons.addConstraint();
            for (IndexType pinIdx=0; pinIdx<2*numPinPair; pinIdx++) {
                if (pinIdx < numPinPair) {
                    con.varId_coeff_pair.emplace_back(VarIdCoeffPairType(PhysicalMsg2varId(pinIdx, layerIdx, 0), LocType(1)));
                    con.varId_coeff_pair.emplace_back(VarIdCoeffPairType(PhysicalMsg2varId(pinIdx, layerIdx, 1), LocType(1)));
                } else {
                    con.varId_coeff_pair.emplace_back(VarIdCoeffPairType(PhysicalMsg2varId(pinIdx, layerIdx, 0), LocType(-1)));
                    con.varId_coeff_pair.emplace_back(VarIdCoeffPairType(PhysicalMsg2varId(pinIdx, layerIdx, 1), LocType(-1)));
                }
            }
            con.contype = ConType::Equal;
            con.objective = 0;
        }
        // minimize layer
        std::vector<double> obj_coeff;
        for (IndexType pinIdx=0; pinIdx<2*numPinPair; pinIdx++) {
            for (IndexType layerIdx=0; layerIdx<numLayer; layerIdx++) {
                for (IndexType esc=0; esc<2; esc++) {
                    obj_coeff.emplace_back(numLayer-layerIdx);
                }
            }
        }
        // solve problem
        solver.ILPaddConstraints(cons);
        solver.setMaximizeObjective(obj_coeff);
        // solver.setMaximizeObjective(std::vector<double>(2*numPinPair*numLayer*2, 1.0));
        std::cout << "solve ILP poblem... " << std::endl;
        if (!solver.solve()) {
            std::cout << "No solution found!" << std::endl;
            return false;
        }
        std::cout << "Objective Value: " << solver.getObjectiveValue() << std::endl;
        std::vector<IndexType> solution = solver.getSolution();
        for (IndexType solutionIdx=0; solutionIdx<solution.size(); solutionIdx++) { 
            if (solution[solutionIdx] == 0) { continue; }
            std::vector<IndexType> physical_msg = varId2PhysicalMsg(solutionIdx);
            if (physical_msg[0] < numPinPair) {
                Pin* pin = cpair.pins0()[physical_msg[0]]; pin->routeLayer() = physical_msg[1]; pin->escape01() = physical_msg[2];
            } else {
                Pin* pin = cpair.pins1()[physical_msg[0]-numPinPair]; pin->routeLayer() = physical_msg[1]; pin->escape01() = physical_msg[2];
            }
        }
    }
    return true;
}



void PinReDistribution::ReDistribution(bool random_esc) {
    struct updatePins {
        struct updatePin {
            IndexType idx_in_cluster;
            LocType new_x;
            LocType new_y;
            IndexType new_layer;
            IndexType new_esc01;
            updatePin(IndexType idx_in_cluster, LocType new_x, LocType new_y, IndexType new_layer, IndexType new_esc01)
                : idx_in_cluster(idx_in_cluster), new_x(new_x), new_y(new_y), new_layer(new_layer), new_esc01(new_esc01) {}
        };
        std::vector<updatePin> all_update_pins;
    };

    std::mt19937 engine(123);
    std::uniform_int_distribution<int> dist(0, 1); // dist(engine)
    for (auto cpair : _vcluster_pair) {
        bool is_chiplet0_ref = (cpair.chiplet0()->getId() < cpair.chiplet1()->getId());
        std::vector<Pin*> ref_pins = is_chiplet0_ref ? cpair.pins0() : cpair.pins1();
        std::vector<Pin*> mod_pins = is_chiplet0_ref ? cpair.pins1() : cpair.pins0();
        auto ref_func = is_chiplet0_ref ? cpair.judge_front_func0 : cpair.judge_front_func1;
        auto mod_func = is_chiplet0_ref ? cpair.judge_front_func1 : cpair.judge_front_func0;
        SortedPinsType layer_sorted_ref_pins, layer_sorted_mod_pins;
        updatePins update_pins;
        for (auto pin : ref_pins) { layer_sorted_ref_pins[pin->routeLayer()].emplace_back(pin); }
        for (auto pin : mod_pins) { layer_sorted_mod_pins[pin->routeLayer()].emplace_back(pin); }

        for (auto [layer, ref_pins_in_layer] : layer_sorted_ref_pins) {
            auto mod_pins_in_layer = layer_sorted_mod_pins[layer];
            if (ref_pins_in_layer.size() != mod_pins_in_layer.size()) { 
                std::cout << layer << " " << ref_pins_in_layer.size() << mod_pins_in_layer.size() << std::endl;
                std::cerr << "number of pins cluster in A is not the same in B in one layer. "; 
            }
            std::vector<Pin*> sorted_ref_pins_in_layer = ref_pins_in_layer;
            std::vector<Pin*> sorted_mod_pins_in_layer = mod_pins_in_layer;
            if (random_esc) {
                // random escape direction
                for (auto pin : sorted_ref_pins_in_layer) { pin->escape01() = dist(engine); }
                for (auto pin : sorted_mod_pins_in_layer) { pin->escape01() = dist(engine); }
            }
            std::sort(sorted_ref_pins_in_layer.begin(), sorted_ref_pins_in_layer.end(), [&] (Pin* p0, Pin* p1) { return ref_func(p0, p1, p0->escape01(), p1->escape01()); });
            std::sort(sorted_mod_pins_in_layer.begin(), sorted_mod_pins_in_layer.end(), [&] (Pin* p0, Pin* p1) { return mod_func(p0, p1, p0->escape01(), p1->escape01()); });
            for (IndexType idx = 0; idx < sorted_ref_pins_in_layer.size(); idx++) {
                IndexType ref_pin_id = sorted_ref_pins_in_layer[idx]->getId();
                auto it = std::find_if(ref_pins.begin(), ref_pins.end(), [&] (Pin* p) { return p->getId() == ref_pin_id; });
                if (it == ref_pins.end()) { std::cerr << "no such pin in pins" << std::endl; }
                IndexType distance = std::distance(ref_pins.begin(), it); // mod_pins[distance]
                update_pins.all_update_pins.emplace_back(distance, \
                                                         sorted_mod_pins_in_layer[idx]->getXOffset(), \
                                                         sorted_mod_pins_in_layer[idx]->getYOffset(), \
                                                         sorted_mod_pins_in_layer[idx]->routeLayer(), \
                                                         sorted_mod_pins_in_layer[idx]->escape01());
            }
        }
        for (auto update_pin : update_pins.all_update_pins) {
            auto pin = mod_pins[update_pin.idx_in_cluster];
            pin->setOffset(update_pin.new_x, update_pin.new_y);
            pin->routeLayer() = update_pin.new_layer;
            pin->escape01() = update_pin.new_esc01;
        }
    }
}

/*
            // std::vector<std::pair<LocType, LocType>> update_mod_xy;
            // std::vector<std::pair<IndexType, IndexType>> update_mod_layer_esc01;
            for (IndexType idx = 0; idx < ref_pins.size(); idx++) {
                IndexType ref_pin_id = ref_pins[idx]->getId();
                auto it = std::find_if(sorted_ref_pins_in_layer.begin(), sorted_ref_pins_in_layer.end(), [&] (Pin* p) { return p->getId() == ref_pin_id; });
                if (it == sorted_ref_pins_in_layer.end()) {
                    std::cerr << "err" << std::endl;
                }
                IndexType distance = std::distance(sorted_ref_pins_in_layer.begin(), it);
                update_mod_xy.emplace_back(std::pair(sorted_mod_pins_in_layer[distance]->getXOffset(), sorted_mod_pins_in_layer[distance]->getYOffset()));
                update_mod_layer_esc01.emplace_back(std::pair(sorted_mod_pins_in_layer[distance]->routeLayer(), sorted_mod_pins_in_layer[distance]->escape01()));
            }
            for (IndexType idx = 0; idx < mod_pins.size(); idx++) {
                mod_pins[idx]->setOffset(update_mod_xy[idx].first, update_mod_xy[idx].second);
                mod_pins[idx]->routeLayer() = update_mod_layer_esc01[idx].first;
                mod_pins[idx]->escape01()   = update_mod_layer_esc01[idx].second;
            }


*/