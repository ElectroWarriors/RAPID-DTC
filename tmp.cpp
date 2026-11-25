// class ClusterType {
//     public:
//         ClusterType(ChipletPinType cp) : _chipletIdx(cp.first) { _vpinsIdx.emplace_back(cp.second); }
//         ClusterType(IndexType chipletIdx) : _chipletIdx(chipletIdx) {}
//         ClusterType(IndexType chipletIdx, std::vector<IndexType> vpinsIdx) : _chipletIdx(chipletIdx), _vpinsIdx(vpinsIdx) {}

//         IndexType chipletId() const {return _chipletIdx; }
//         std::vector<IndexType> vpinsIdx() const {return _vpinsIdx; }

//         bool add(ClusterType& clu) {
//             if (_chipletIdx != clu.chipletId()) {
//                 return false;
//             }
//             for (IndexType pinIdx : clu.vpinsIdx()) { 
//                 if (std::find(_vpinsIdx.begin(), _vpinsIdx.end(), pinIdx) == _vpinsIdx.end()) { 
//                     _vpinsIdx.emplace_back(pinIdx); 
//                 } else {
//                     std::cout << "warning: pin" << pinIdx << " exist." << std::endl;
//                 }
//             }
//             return true;
//         }

//         bool add(ChipletPinType cp) {
//             if (_chipletIdx != cp.first) {
//                 return false;
//             }
//             if (std::find(_vpinsIdx.begin(), _vpinsIdx.end(), cp.second) == _vpinsIdx.end()) {
//                 _vpinsIdx.emplace_back(cp.second); 
//                 return true;
//             }
//             std::cout << "warning: pin" << cp.second << " exist." << std::endl;
//             return true;
//         }

//         void print() {
//             std::cout << "chipletIdx: " << _chipletIdx << ", \tpinsIdx: ";
//             for (IndexType pinIdx : _vpinsIdx) { std::cout << pinIdx << " "; }
//             std::cout << std::endl;
//         }
//     private:
//         IndexType _chipletIdx;
//         std::vector<IndexType> _vpinsIdx;
// };

// class ClusterPairType {
//     public:
//         inline ClusterPairType(IndexType chipletIdx0, IndexType chipletIdx1) : 
//             _cluster0(chipletIdx0 < chipletIdx1 ? chipletIdx0 : chipletIdx1), 
//             _cluster1(chipletIdx0 < chipletIdx1 ? chipletIdx1 : chipletIdx0) {}
//         inline ClusterPairType(ChipletPinType cp0, ChipletPinType cp1) : 
//             _cluster0(cp0.first < cp1.first ? cp0 : cp1),  
//             _cluster1(cp0.first < cp1.first ? cp1 : cp0) {}

//         bool add(ChipletPinType cp0, ChipletPinType cp1) {
//             ChipletPinType cpl = cp0.first < cp1.first ? cp0 : cp1; 
//             ChipletPinType cpu = cp0.first < cp1.first ? cp1 : cp0; 
//             if (_cluster0.chipletId() != cpl.first || _cluster1.chipletId() != cpu.first) {
//                 return false;
//             }
//             return (_cluster0.add(cpl) && _cluster1.add(cpu));
//         }

//         void print() {
//             std::cout << " --------- cluster pair ----------- " << std::endl;
//             _cluster0.print(); _cluster1.print();
//         }

//     private:
//         ClusterType _cluster0;
//         ClusterType _cluster1;
    
// };




//     void BIPGRouter::ClusterPairGenetation() {
//         for (auto net : _jsondata["nets"]) {
//             if (net["id"] == 0 || net["id"] == 1) {
//                 continue;
//             }
//             auto cells_id = net["cells_id"];
//             auto pins_id = net["pins_id"];

//             ChipletPinType cp0(cells_id[0].asInt(), pins_id[0].asInt());
//             ChipletPinType cp1(cells_id[1].asInt(), pins_id[1].asInt());

//             bool flag = false;
//             for (ClusterPairType& cpair : _vcluster_pair) {
//                 flag = cpair.add(cp0, cp1);
//                 if (flag) { break; }
//             }
//             if (!flag) {
//                 _vcluster_pair.emplace_back(ClusterPairType(cp0, cp1));
//             }
//         }

//         if (true) {
//             for (ClusterPairType cpair : _vcluster_pair) {
//                 cpair.print();
//             }
//         }
//     }



/////////////////////////////////////////////////////////////////////////////////////////////////////////////

// #include <ilcplex/ilocplex.h>
// #include <iostream>
// #include <vector>

// class ILP_Solver {
// public:
//     // 构造函数：初始化CPLEX环境和模型
//     ILP_Solver() : _env(), _model(_env), _cplex(_model) {
//         _cplex.setOut(_env.getNullStream()); // 关闭冗余输出
//     }

//     // 添加二进制变量（0-1变量）
//     void addBinaryVariables(int num_vars) {
//         _vars = IloBoolVarArray(_env, num_vars);
//         _model.add(_vars);
//     }

//     // 设置目标函数（最大化）
//     void setMaximizeObjective(const std::vector<double>& coeffs) {
//         IloExpr obj(_env);
//         for (int i = 0; i < coeffs.size(); ++i) {
//             obj += coeffs[i] * _vars[i];
//         }
//         _model.add(IloMaximize(_env, obj));
//         obj.end();
//     }

//     // 添加线性约束（表达式 <= rhs）
//     void addConstraint(const std::vector<std::pair<int, double>>& terms, double rhs) {
//         IloExpr expr(_env);
//         for (const auto& term : terms) {
//             expr += term.second * _vars[term.first];
//         }
//         _model.add(expr <= rhs);
//         expr.end();
//     }

//     // 求解问题
//     bool solve() {
//         try {
//             return _cplex.solve();
//         } catch (const IloException& e) {
//             std::cerr << "CPLEX Error: " << e << std::endl;
//             return false;
//         }
//     }

//     // 获取目标函数值
//     double getObjectiveValue() const {
//         return _cplex.getObjValue();
//     }

//     // 获取变量解值
//     std::vector<int> getSolution() const {
//         std::vector<int> sol(_vars.getSize());
//         for (int i = 0; i < _vars.getSize(); ++i) {
//             sol[i] = static_cast<int>(_cplex.getValue(_vars[i]));
//         }
//         return sol;
//     }

//     // 析构函数：释放资源
//     ~ILP_Solver() {
//         _cplex.end();
//         _model.end();
//         _env.end();
//     }

// private:
//     IloEnv _env;          // CPLEX环境
//     IloModel _model;      // 优化模型
//     IloCplex _cplex;      // 求解器实例
//     IloBoolVarArray _vars; // 二进制变量数组
// };

// void ilp_test() {
//     ILP_Solver solver;

//     // 添加3个二进制变量
//     solver.addBinaryVariables(3);

//     // 设置目标函数：最大化 3x0 + 2x1 + 5x2
//     solver.setMaximizeObjective({3, 2, 5});

//     // 添加约束：2x0 + x1 + 4x2 <= 7
//     solver.addConstraint({{0, 2}, {1, 1}, {2, 4}}, 7);

//     // 求解并输出结果
//     if (solver.solve()) {
//         std::cout << "Objective Value: " << solver.getObjectiveValue() << std::endl;
//         auto solution = solver.getSolution();
//         std::cout << "Solution: ";
//         for (int val : solution) {
//             std::cout << val << " ";
//         }
//         std::cout << std::endl;
//     } else {
//         std::cout << "No solution found!" << std::endl;
//     }
// }



// bool PinReDistribution::ILPnonOverFlow(IndexType numLayer){
//     auto PhysicalMsg2varId = [&](IndexType netIdx, IndexType layerIdx, IndexType esc) -> IndexType  {
//         return (numLayer*2)*netIdx + (2)*layerIdx + (1)*esc;
//     };
//     auto varId2PhysicalMsg = [&](IndexType id) -> std::vector<IndexType> {
//         std::vector<IndexType> v_tmp;
//         const std::array<IndexType, 3> divisors = {numLayer*2, 2, 1};
        
//         for (const auto& divisor : divisors) {
//             IndexType tmp = id / divisor;
//             id %= divisor;
//             v_tmp.emplace_back(tmp);
//         }
//         return v_tmp;
//     };

//     for (auto cpair : _vcluster_pair) {
//         for (IndexType idx = 0; idx < 2; idx++) {
//             Chiplet*       chiplet = idx == 0 ? cpair.chiplet0()   : cpair.chiplet1();
//             std::vector<Pin*> pins = idx == 0 ? cpair.pins0()      : cpair.pins1();
//             Direction    direction = idx == 0 ? cpair.direction0() : cpair.direction1();
//             SortedPinsType& sortedPins = idx == 0 ? cpair.sorted_pins0() : cpair.sorted_pins1(); 

//             ILP_Solver solver;
//             solver.addBinaryVariables(pins.size() * numLayer * 2);
//             std::cout << "generate constraints... " << std::endl;
//             Constraints cons;
//             // one net constraints
//             for (IndexType pinIdx=0; pinIdx<pins.size(); pinIdx++) {
//                 Constraint& con = cons.addConstraint();
//                 for (IndexType layerIdx=0; layerIdx<numLayer; layerIdx++) {
//                     for (IndexType esc=0; esc<2; esc++) {
//                         con.varId_coeff_pair.emplace_back(VarIdCoeffPairType(PhysicalMsg2varId(pinIdx, layerIdx, esc), LocType(1)));
//                     }
//                 }
//                 con.contype = ConType::Equal;
//                 con.objective = 1;
//             }

//             // capacitor constraints
//             for (const auto& pin : pins) {
//                 if (direction == Direction::U || direction == Direction::D) {
//                     sortedPins[pin->getXOffset()].emplace_back(pin);
//                 } else {
//                     sortedPins[pin->getYOffset()].emplace_back(pin);
//                 }
//             }
//             for (auto it = sortedPins.begin(); it != sortedPins.end(); ++it) {
//                 auto it_next = std::next(it);
//                 if (it == sortedPins.begin()) {
//                     for (IndexType layerIdx = 0; layerIdx < numLayer; layerIdx++) {
//                         Constraint& con = cons.addConstraint();
//                         for (Pin* pin : it->second) {
//                             con.varId_coeff_pair.emplace_back(VarIdCoeffPairType(PhysicalMsg2varId(pin->id_tmp(), layerIdx, 0), LocType(1)));
//                         }
//                         con.contype = ConType::LessEq;
//                         con.objective = 2;
//                     }
//                 }
//                 if (it_next == sortedPins.end()) {
//                     for (IndexType layerIdx = 0; layerIdx < numLayer; layerIdx++) {
//                         Constraint& con = cons.addConstraint();
//                         for (Pin* pin : it->second) {
//                             con.varId_coeff_pair.emplace_back(VarIdCoeffPairType(PhysicalMsg2varId(pin->id_tmp(), layerIdx, 1), LocType(1)));
//                         }
//                         con.contype = ConType::LessEq;
//                         con.objective = 2;
//                     }
//                     break;
//                 }
//                 for (IndexType layerIdx = 0; layerIdx < numLayer; layerIdx++) {
//                     Constraint& con = cons.addConstraint();
//                     for (Pin* pin : it->second) {
//                         con.varId_coeff_pair.emplace_back(VarIdCoeffPairType(PhysicalMsg2varId(pin->id_tmp(), layerIdx, 1), LocType(1)));
//                     }
//                     for (Pin* pin : it_next->second) {
//                         con.varId_coeff_pair.emplace_back(VarIdCoeffPairType(PhysicalMsg2varId(pin->id_tmp(), layerIdx, 0), LocType(1)));
//                     }
//                     con.contype = ConType::LessEq;
//                     con.objective = 5;
//                 }
//             }
//             solver.ILPaddConstraints(cons);
//             solver.setMaximizeObjective(std::vector<double>(pins.size()*numLayer*2, 1.0));
//             std::cout << "solve ILP poblem... " << std::endl;
//             if (!solver.solve()) {
//                 std::cout << "No solution found!" << std::endl;
//                 return false;
//             }
//             std::cout << "Objective Value: " << solver.getObjectiveValue() << std::endl;
//             std::vector<IndexType> solution = solver.getSolution();
//             for (IndexType solutionIdx=0; solutionIdx<solution.size(); solutionIdx++) { 
//                 if (solution[solutionIdx] == 0) { continue; }
//                 std::vector<IndexType> physical_msg = varId2PhysicalMsg(solutionIdx);
//                 Pin* pin = pins[physical_msg[0]]; pin->routeLayer() = physical_msg[1]; pin->escape01() = physical_msg[2];
//             }
//         }
//     }
//     return true;
// }


// void PinReDistribution::ReDistribution(bool random_esc) {
//     std::mt19937 engine(123);
//     std::uniform_int_distribution<int> dist(0, 1); // dist(engine)
//     for (auto cpair : _vcluster_pair) {
//         bool is_chiplet0_ref = (cpair.chiplet0()->getId() < cpair.chiplet1()->getId());
//         std::vector<Pin*> ref_pins = is_chiplet0_ref ? cpair.pins0() : cpair.pins1();
//         std::vector<Pin*> mod_pins = is_chiplet0_ref ? cpair.pins1() : cpair.pins0();
//         auto ref_func = is_chiplet0_ref ? cpair.judge_front_func0 : cpair.judge_front_func1;
//         auto mod_func = is_chiplet0_ref ? cpair.judge_front_func1 : cpair.judge_front_func0;
//         std::vector<Pin*> sorted_ref_pins = ref_pins, sorted_mod_pins = mod_pins;
//         if (random_esc) {
//             // random escape direction
//             for (auto pin : sorted_ref_pins) { pin->escape01() = dist(engine); }
//             for (auto pin : sorted_mod_pins) { pin->escape01() = dist(engine); }
//         }
//         std::sort(sorted_ref_pins.begin(), sorted_ref_pins.end(), [&] (Pin* p0, Pin* p1) { return ref_func(p0, p1, p0->escape01(), p1->escape01()); });
//         std::sort(sorted_mod_pins.begin(), sorted_mod_pins.end(), [&] (Pin* p0, Pin* p1) { return mod_func(p0, p1, p0->escape01(), p1->escape01()); });
//         std::vector<std::pair<LocType, LocType>> update_mod_xy;
//         std::vector<std::pair<IndexType, IndexType>> update_mod_layer_esc01;
//         for (IndexType idx = 0; idx < ref_pins.size(); idx++) {
//             IndexType ref_pin_id = ref_pins[idx]->getId();
//             auto it = std::find_if(sorted_ref_pins.begin(), sorted_ref_pins.end(), [&] (Pin* p) { return p->getId() == ref_pin_id; });
//             if (it == sorted_ref_pins.end()) {
//                 std::cerr << "err" << std::endl;
//             }
//             IndexType distance = std::distance(sorted_ref_pins.begin(), it);
//             update_mod_xy.emplace_back(std::pair(sorted_mod_pins[distance]->getXOffset(), sorted_mod_pins[distance]->getYOffset()));
//             update_mod_layer_esc01.emplace_back(std::pair(sorted_mod_pins[distance]->routeLayer(), sorted_mod_pins[distance]->escape01()));
//         }
//         for (IndexType idx = 0; idx < mod_pins.size(); idx++) {
//             mod_pins[idx]->setOffset(update_mod_xy[idx].first, update_mod_xy[idx].second);
//             mod_pins[idx]->routeLayer() = update_mod_layer_esc01[idx].first;
//             mod_pins[idx]->escape01()   = update_mod_layer_esc01[idx].second;
//         }
//     }
// }