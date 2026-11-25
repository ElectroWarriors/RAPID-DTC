#ifndef ROUTER_GLOBAL_H_
#define ROUTER_GLOBAL_H_

#include <iostream>
#include <utility>
#include <vector>
#include <cstdint>
#include <string>
#include <sstream>
#include <algorithm>
#include <functional>
#include <map>

using IndexType  = std::uint32_t;
using IntType    = std::int32_t;
using RealType   = double;
using Byte       = std::uint8_t;
using LocType    = std::int64_t; // Location/design unit // Location/design unit
using BoolType   = Byte;
using NameType   = std::string;

constexpr IndexType INDEX_TYPE_MAX  = UINT32_MAX;
constexpr IndexType LAYER_NUM_MAX = 100;
constexpr IntType INT_TYPE_MAX      = INT32_MAX;
constexpr IntType INT_TYPE_MIN      = INT32_MIN;
constexpr RealType REAL_TYPE_MAX    = 1e100;
constexpr RealType REAL_TYPE_MIN    = -1e100;
constexpr RealType REAL_TYPE_TOL    = 1e-6;
constexpr LocType LOC_TYPE_MAX      = INT64_MAX;
constexpr LocType LOC_TYPE_MIN      = INT64_MIN;

extern bool RUN_RDL;
extern bool CONSIDER_DTC_BLKG;

class Chiplet;
class Pin;
class Net;
class Track;

enum class Direction {Default, U, D, L, R};
enum class ConType { LessEq, GreaterEq, Equal };

typedef std::pair<LocType, LocType> LocXYType;
typedef std::pair<IndexType, IndexType> ChipletPinType;
typedef std::pair<IndexType, LocType> VarIdCoeffPairType;
typedef std::map<IndexType, std::vector<Pin*>> SortedPinsType;
typedef std::map<IndexType, std::vector<std::pair<Pin*, Pin*>>> SortedPinPairType;
typedef std::pair<SortedPinsType, Direction> PinEscType;
typedef std::pair<PinEscType, PinEscType> PinEscPairType;

class GlobalParameter {
    public:
        static inline RealType line_width = 2;
        static inline RealType line_pitch = 3;
        static inline RealType dtc_size = 10;
};

class Track {
    public:
        Track() {}
        void addPoint(LocType x, LocType y) { _xTrack.emplace_back(x); _yTrack.emplace_back(y); }
        std::vector<LocType>& xTrack() { return _xTrack; }
        std::vector<LocType>& yTrack() { return _yTrack; }
        void clear() { _xTrack.clear(); _yTrack.clear(); }
    private:
        std::vector<LocType> _xTrack;
        std::vector<LocType> _yTrack;
};

class Chiplet {
    public:
        Chiplet() {}
        Chiplet(IndexType idx) : _id(idx) {}
        Chiplet(IndexType idx, NameType name) : _id(idx), _name(name) {}

        void setId(IndexType idx) { _id = idx; _name = "chiplet_"+std::to_string(idx); }
        void setName(NameType name) { _name = name; }
        void setShape(IndexType w, IndexType h) { _size[0] = w; _size[1] = h; }
        void setXYNumPins(IndexType nx_pins, IndexType ny_pins) {_xynumPins[0] = nx_pins; _xynumPins[1] = ny_pins; }
        void setLocation(IndexType x, IndexType y) {_loc[0] = x; _loc[1] = y; }
        void addPin(Pin* pin) { _vpins.emplace_back(pin); }

        IndexType& getId() {return _id; }
        NameType& getName() {return _name; }
        LocType& getXLoc() {return _loc[0]; }
        LocType& getYLoc() {return _loc[1]; }
        LocType& getWidth() {return _size[0]; }
        LocType& getHeight() {return _size[1]; }
        IndexType& getXNumPins() {return _xynumPins[0]; }
        IndexType& getYNumPins() {return _xynumPins[1]; }
        std::vector<Pin*> getPins() {return _vpins; }

    private:
        IndexType _id;
        NameType _name;
        LocType _loc[2];
        LocType _size[2];
        IndexType _xynumPins[2];
        std::vector<Pin*> _vpins;
};

class Pin {
    public:
        Pin() {}
        Pin(IndexType idx) : _id(idx) {}
        Pin(IndexType idx, NameType name) : _id(idx), _name(name) {}

        void setId(IndexType idx) { _id = idx; _name = "pin_"+std::to_string(idx); }
        void setName(NameType name) { _name = name; }
        void setOffset(IndexType x, IndexType y) { _offset[0] = x; _offset[1] = y; }
        void setShape(IndexType xLo, IndexType yLo, IndexType xHi, IndexType yHi) { _ioShape[0] = xLo; _ioShape[1] = yLo; _ioShape[2] = xHi; _ioShape[3] = yHi; }
        void setChiplet(Chiplet* chiplet) {_chiplet = chiplet; }
        void setNet(Net* net) { _net = net; }

        IndexType& getId() {return _id; }
        NameType& getName() {return _name; }
        LocType& getXOffset() {return _offset[0]; }
        LocType& getYOffset() {return _offset[1]; }
        LocType& ioshape(IndexType idx) { return _ioShape[idx]; }
        LocType getXIOShape() {return _ioShape[2] - _ioShape[0]; }
        LocType getYIOShape() {return _ioShape[3] - _ioShape[1]; }
        LocType getXCenter() { return _offset[0] + (_ioShape[2] - _ioShape[0]) / 2; }
        LocType getYCenter() { return _offset[1] + (_ioShape[3] - _ioShape[1]) / 2; }
        Chiplet* getChiplet() { return _chiplet; }
        Net* getNet() {return _net; }

        IndexType& routeLayer() { return _routLayer; }
        IndexType& escape01() { return _esc01; }
        IndexType& numCap() { return _numCap; }
        IndexType& type_power() { return _type_power; }

        IndexType& id_tmp() { return _id_tmp; }
        Track& escTrack() { return _escTrack; }
        LocType& esc1_spcace() { return _esc1_space; }
        void clear_track_msg() { 
            _routLayer = INDEX_TYPE_MAX; 
            _escDirection = Direction::Default; 
            _esc01 = INDEX_TYPE_MAX; 
            _escTrack.clear();
        }

    private:
        IndexType _id;
        IndexType _id_tmp;
        NameType _name;
        Chiplet* _chiplet;
        Net* _net;
        LocType _offset[2];
        LocType _ioShape[4]; // xLo, yLo, xHi, yHi
        IndexType _type_power;
        // track
        IndexType _routLayer;
        Direction _escDirection;
        IndexType _esc01;
        IndexType _numCap;
        LocType   _esc1_space;
        Track _escTrack;
        
};


class Net {
    public:
        Net() {}
        Net(IndexType idx) : _id(idx) {}
        Net(IndexType idx, NameType name) : _id(idx), _name(name) {}

        void setId(IndexType idx) { _id = idx; _name = "net_"+std::to_string(idx); }
        void setName(NameType name) { _name = name; }
        void setPower(bool isPower) { _isPower = isPower; }
        void addPin(Pin* pin) { _vpins.emplace_back(pin); }
        
        IndexType& getId() {return _id; }
        NameType& getName() {return _name; }
        IndexType ispower() { if (_isPower) return 1; else return 0; }
        std::vector<Pin*> getPins() {return _vpins; }

    private:
        IndexType _id;
        NameType _name;
        bool _isPower = false;
        std::vector<Pin*> _vpins;
};

class TSV {
    public:
        TSV() {}
        TSV(IndexType idx) : _id(idx) {}
        TSV(IndexType idx, NameType name) : _id(idx), _name(name) {}

        void setOffset(IndexType x, IndexType y) { _offset[0] = x; _offset[1] = y; }
        void setSize(IndexType w, IndexType h) { _size[0] = w; _size[1] = h; }
        void setCurrent(RealType Il, RealType Iu) { _current[0] = Il; _current[1] = Iu; }

        LocType&   getXOffset() { return _offset[0]; }
        LocType&   getYOffset() { return _offset[1]; }
        LocType&   getWidth()   { return _size[0]; }
        LocType&   getHeight()  { return _size[1]; }
        bool&      inChiplet()  { return _inChiplet; }
        RealType&  getIlower()  { return _current[0]; }
        RealType&  getIupper()  { return _current[1]; }
        RealType&  capDensity() { return _capdensity; }

    private:
        IndexType _id;
        NameType _name;
        LocType _offset[2];
        LocType _size[2];
        bool _inChiplet;
        RealType _current[2];
        RealType _capdensity; 
};

class DTC {
    public:
        DTC() {}
        DTC(IndexType idx) : _id(idx) {}
        DTC(IndexType idx, NameType name) : _id(idx), _name(name) {}
        void setLocation(LocType xc, LocType yc) {
            _loc_center[0] = xc; _loc_center[1] = yc;
        }
        void setShape(LocType w, LocType h) {
            _size[0] = w; _size[1] = h;
        }
        LocType& xLoc() { return _loc_center[0]; }
        LocType& yLoc() { return _loc_center[1]; }
        LocType& width() { return _size[0]; }
        LocType& height() { return _size[1]; }

    private:
        IndexType _id;
        NameType _name;
        LocType _loc_center[2];
        LocType _size[2];
};

class Global {
    public:
        Global() {}

        LocType& TSVXDistance() { return _tsvDistance[0]; }
        LocType& TSVYDistance() { return _tsvDistance[1]; }

    private:
        LocType _tsvDistance[2];
};


class Database {
    public:
        Database() {}
        Chiplet* allocateChiplet() { _vchiplets.emplace_back(Chiplet()); return &_vchiplets[_vchiplets.size()-1]; }
        Chiplet* allocateChiplet(IndexType idx) { _vchiplets.emplace_back(Chiplet(idx)); return &_vchiplets[_vchiplets.size()-1]; }
        Chiplet* allocateChiplet(IndexType idx, NameType name) { _vchiplets.emplace_back(Chiplet(idx, name)); return &_vchiplets[_vchiplets.size()-1]; }
        Pin*     allocatePin()     { _vpins.emplace_back(Pin()); return &_vpins[_vpins.size()-1]; }
        Pin*     allocatePin(IndexType idx)     { _vpins.emplace_back(Pin(idx)); return &_vpins[_vpins.size()-1]; }
        Pin*     allocatePin(IndexType idx, NameType name)     { _vpins.emplace_back(Pin(idx, name)); return &_vpins[_vpins.size()-1]; }
        Net*     allocateNet()     { _vnets.emplace_back(Net()); return &_vnets[_vnets.size()-1]; }
        Net*     allocateNet(IndexType idx)     { _vnets.emplace_back(Net(idx)); return &_vnets[_vnets.size()-1]; }
        Net*     allocateNet(IndexType idx, NameType name)     { _vnets.emplace_back(Net(idx, name)); return &_vnets[_vnets.size()-1]; }
        TSV*     allocateTSV()     { _vtsvs.emplace_back(TSV()); return &_vtsvs[_vtsvs.size()-1]; }
        TSV*     allocateTSV(IndexType idx)     { _vtsvs.emplace_back(TSV(idx)); return &_vtsvs[_vtsvs.size()-1]; }
        TSV*     allocateTSV(IndexType idx, NameType name)     { _vtsvs.emplace_back(TSV(idx, name)); return &_vtsvs[_vtsvs.size()-1]; }

        std::vector<Chiplet>& chiplets() { return _vchiplets; }
        std::vector<Pin>&     pins()     { return _vpins; }
        std::vector<Net>&     nets()     { return _vnets; }
        std::vector<TSV>&     tsvs()     { return _vtsvs; }
        Global& global() { return _global; }
        
        // 以下三个getElementwithId()可用template优化，Chiplet，Pin和Net可用class继承出来（都有name，id等，初始化方式也一致）
        Chiplet* getChipletwithId(IndexType idx) {
            if (_vchiplets[idx].getId() == idx) { return &_vchiplets[idx]; }
            for (auto& chiplet : _vchiplets) {
                if (chiplet.getId() == idx) { return &chiplet; }
            }
            return nullptr;
        }
        Pin* getPinwithId(IndexType idx) {
            if (_vpins[idx].getId() == idx) { return &_vpins[idx]; }
            for (auto& pin : _vpins) {
                if (pin.getId() == idx) { return &pin; }
            }
            return nullptr;
        }
        Net* getNetwithId(IndexType idx) {
            if (_vnets[idx].getId() == idx) { return &_vnets[idx]; }
            for (auto& net : _vnets) {
                if (net.getId() == idx) { return &net; }
            }
            return nullptr;
        }

        std::pair<LocType, LocType> overlapXY(IndexType idx0, IndexType idx1) {
            Chiplet* ch0 = getChipletwithId(idx0);
            Chiplet* ch1 = getChipletwithId(idx1);
            if (ch0 == nullptr || ch1 == nullptr) {
                return std::pair<LocType, LocType>(LOC_TYPE_MAX, LOC_TYPE_MAX);
            }
            LocType ox = std::max(ch0->getXLoc()+ch0->getWidth(), ch1->getXLoc()+ch1->getWidth()) \
                       - std::min(ch0->getXLoc(), ch1->getXLoc()) - (ch0->getWidth() + ch1->getWidth());
            LocType oy = std::max(ch0->getYLoc()+ch0->getHeight(), ch1->getYLoc()+ch1->getHeight()) \
                       - std::min(ch0->getYLoc(), ch1->getYLoc()) - (ch0->getHeight() + ch1->getHeight());
            return std::pair<LocType, LocType>(ox, oy);
        }

    private:
        std::vector<Chiplet> _vchiplets;
        std::vector<Pin> _vpins;
        std::vector<Net> _vnets;
        std::vector<TSV> _vtsvs;
        std::vector<DTC> _vdtcs;
        Global _global; 
};



class ClusterPairType {
    public:
        ClusterPairType(Chiplet* chiplet0, Chiplet* chiplet1) {
            if (chiplet0->getId() == chiplet1->getId()) {
                throw std::invalid_argument(" there are some bug to be fixed in clusterpairtype");
            }
            _chiplet0 = chiplet0->getId() < chiplet1->getId() ? chiplet0 : chiplet1;
            _chiplet1 = chiplet0->getId() > chiplet1->getId() ? chiplet0 : chiplet1;

            auto left_esc = [](Pin* p0, Pin* p1, IndexType idx0, IndexType idx1) -> bool {
                if (p0->getYOffset() != p1->getYOffset()) { return p0->getYOffset() > p1->getYOffset(); }
                if (idx0 != idx1) { return idx0 > idx1; }
                return idx0 == 1 ? p0->getXOffset() > p1->getXOffset() : p0->getXOffset() < p1->getXOffset();
            };
            auto right_esc = [](Pin* p0, Pin* p1, IndexType idx0, IndexType idx1) -> bool {
                if (p0->getYOffset() != p1->getYOffset()) { return p0->getYOffset() > p1->getYOffset(); }
                if (idx0 != idx1) { return idx0 > idx1; }
                return idx0 == 1 ? p0->getXOffset() < p1->getXOffset() : p0->getXOffset() > p1->getXOffset();
            };
            auto up_esc = [](Pin* p0, Pin* p1, IndexType idx0, IndexType idx1) -> bool {
                if (p0->getXOffset() != p1->getXOffset()) { return p0->getXOffset() > p1->getXOffset(); }
                if (idx0 != idx1) { return idx0 > idx1; }
                return idx0 == 1 ? p0->getYOffset() < p1->getYOffset() : p0->getYOffset() > p1->getYOffset();
            };
            auto down_esc = [](Pin* p0, Pin* p1, IndexType idx0, IndexType idx1) -> bool {
                if (p0->getXOffset() != p1->getXOffset()) { return p0->getXOffset() > p1->getXOffset(); }
                if (idx0 != idx1) { return idx0 > idx1; }
                return idx0 == 1 ? p0->getYOffset() > p1->getYOffset() : p0->getYOffset() < p1->getYOffset();
            };

            std::pair<LocType, LocType> ovl_xy = overlapXY(_chiplet0, _chiplet1);
            if (ovl_xy.first >= 0 && ovl_xy.second <= 0) {
                if (_chiplet0->getXLoc() < _chiplet1->getXLoc()) {
                    _direction0 = Direction::R; _direction1 = Direction::L;
                    judge_front_func0 = [&](Pin* p0, Pin* p1, IndexType idx0, IndexType idx1) -> bool { return right_esc(p0, p1, idx0, idx1); };
                    judge_front_func1 = [&](Pin* p0, Pin* p1, IndexType idx0, IndexType idx1) -> bool { return left_esc(p0, p1, idx0, idx1); };
                } else {
                    _direction0 = Direction::L; _direction1 = Direction::R;
                    judge_front_func0 = [&](Pin* p0, Pin* p1, IndexType idx0, IndexType idx1) -> bool { return left_esc(p0, p1, idx0, idx1); };
                    judge_front_func1 = [&](Pin* p0, Pin* p1, IndexType idx0, IndexType idx1) -> bool { return right_esc(p0, p1, idx0, idx1); };
                }
            } else if (ovl_xy.first <= 0 && ovl_xy.second >= 0) {
                if (_chiplet0->getYLoc() < _chiplet1->getYLoc()) {
                    _direction0 = Direction::U; _direction1 = Direction::D;
                    judge_front_func0 = [&](Pin* p0, Pin* p1, IndexType idx0, IndexType idx1) -> bool { return up_esc(p0, p1, idx0, idx1); };
                    judge_front_func1 = [&](Pin* p0, Pin* p1, IndexType idx0, IndexType idx1) -> bool { return down_esc(p0, p1, idx0, idx1); };
                } else {
                    _direction0 = Direction::D; _direction1 = Direction::U;
                    judge_front_func0 = [&](Pin* p0, Pin* p1, IndexType idx0, IndexType idx1) -> bool { return down_esc(p0, p1, idx0, idx1); };
                    judge_front_func1 = [&](Pin* p0, Pin* p1, IndexType idx0, IndexType idx1) -> bool { return up_esc(p0, p1, idx0, idx1); };
                }
            }else if (ovl_xy.first >= 0 && ovl_xy.second >= 0) {
                if (_chiplet0->getXLoc() < _chiplet1->getXLoc() && _chiplet0->getYLoc() < _chiplet1->getYLoc()) {
                    _direction0 = Direction::U; _direction1 = Direction::L;
                    judge_front_func0 = [&](Pin* p0, Pin* p1, IndexType idx0, IndexType idx1) -> bool { return up_esc(p0, p1, idx0, idx1); };
                    judge_front_func1 = [&](Pin* p0, Pin* p1, IndexType idx0, IndexType idx1) -> bool { return !left_esc(p0, p1, idx0, idx1); };
                } else if (_chiplet0->getXLoc() < _chiplet1->getXLoc() && _chiplet0->getYLoc() > _chiplet1->getYLoc()) {
                    _direction0 = Direction::D; _direction1 = Direction::L;
                    judge_front_func0 = [&](Pin* p0, Pin* p1, IndexType idx0, IndexType idx1) -> bool { return down_esc(p0, p1, idx0, idx1); };
                    judge_front_func1 = [&](Pin* p0, Pin* p1, IndexType idx0, IndexType idx1) -> bool { return left_esc(p0, p1, idx0, idx1); };
                } else if (_chiplet0->getXLoc() > _chiplet1->getXLoc() && _chiplet0->getYLoc() > _chiplet1->getYLoc()) {
                    _direction0 = Direction::L; _direction1 = Direction::U;
                    judge_front_func0 = [&](Pin* p0, Pin* p1, IndexType idx0, IndexType idx1) -> bool { return left_esc(p0, p1, idx0, idx1); };
                    judge_front_func1 = [&](Pin* p0, Pin* p1, IndexType idx0, IndexType idx1) -> bool { return !up_esc(p0, p1, idx0, idx1); };
                } else if (_chiplet0->getXLoc() > _chiplet1->getXLoc() && _chiplet0->getYLoc() < _chiplet1->getYLoc()) {
                    _direction0 = Direction::L; _direction1 = Direction::D;
                    judge_front_func0 = [&](Pin* p0, Pin* p1, IndexType idx0, IndexType idx1) -> bool { return left_esc(p0, p1, idx0, idx1); };
                    judge_front_func1 = [&](Pin* p0, Pin* p1, IndexType idx0, IndexType idx1) -> bool { return down_esc(p0, p1, idx0, idx1); };
                }
            } else {
                std::cout << ovl_xy.first << ", " << ovl_xy.second << std::endl;
                throw std::invalid_argument("clusterpairtype");
            }
        }

        void powerPinsInit(IndexType type_cl0, IndexType type_cl1) {
            for (IndexType idx = 0; idx < 2; idx++) {
                Chiplet* chiplet  = idx == 0 ? _chiplet0 : _chiplet1;
                IndexType type_cl = idx == 0 ? type_cl0 : type_cl1;
                std::vector<Pin*>& _vppins = idx == 0 ? _vppins0 : _vppins1;

                for (auto pin : chiplet->getPins()) {
                    if (pin->getNet()->getId() != 0 && pin->getNet()->getId() != 1) { continue; }
                    if (pin->type_power() == type_cl) {
                        _vppins.emplace_back(pin);
                    }
                }
            }
        }

        std::pair<LocType, LocType> overlapXY(Chiplet* ch0, Chiplet* ch1) {
            // ovl > 0: non overlap; ovl < 0: overlap
            LocType ox = std::max(ch0->getXLoc()+ch0->getWidth(), ch1->getXLoc()+ch1->getWidth()) \
                       - std::min(ch0->getXLoc(), ch1->getXLoc()) - (ch0->getWidth() + ch1->getWidth());
            LocType oy = std::max(ch0->getYLoc()+ch0->getHeight(), ch1->getYLoc()+ch1->getHeight()) \
                       - std::min(ch0->getYLoc(), ch1->getYLoc()) - (ch0->getHeight() + ch1->getHeight());
            return std::pair<LocType, LocType>(ox, oy);
        }

        Chiplet* chiplet0() { return _chiplet0; }
        Chiplet* chiplet1() { return _chiplet1; }
        std::vector<Pin*> pins0() { return _vpins0; }
        std::vector<Pin*> pins1() { return _vpins1; }
        std::vector<Pin*> ppins0() { return _vppins0; }
        std::vector<Pin*> ppins1() { return _vppins1; }
        SortedPinsType& sorted_pins0() { return _sorted_pins0; }
        SortedPinsType& sorted_pins1() { return _sorted_pins1; }
        Direction direction0() { return _direction0; }
        Direction direction1() { return _direction1; }
        std::vector<IndexType>& solution() { return _solution; }

        bool addPinPair(Pin* pin0, Pin* pin1) {
            if (pin0->getChiplet()->getId() == _chiplet0->getId() && pin1->getChiplet()->getId() == _chiplet1->getId()) {
                _vpins0.emplace_back(pin0); _vpins1.emplace_back(pin1);
                return true;
            } else if (pin0->getChiplet()->getId() == _chiplet1->getId() && pin1->getChiplet()->getId() == _chiplet0->getId()) {
                _vpins0.emplace_back(pin1); _vpins1.emplace_back(pin0);
                return true;
            } else {
                return false;
            }
        }

        void pin_tmpId_setting() {
            for (IndexType idx = 0; idx < _vpins0.size(); idx++) {
                _vpins0[idx]->id_tmp() = idx; 
                _vpins1[idx]->id_tmp() = idx; 
            }
        }

        void print() {
            std::cout << " ------------- chiplet cluster pair ------------- " << std::endl; 
            for (IndexType idx=0; idx<2; idx++) {
                Chiplet*       chiplet = idx == 0 ? _chiplet0 : _chiplet1;
                std::vector<Pin*> pins = idx == 0 ?   _vpins0 : _vpins1;

                std::cout << "chiplet " << chiplet->getId() << ":\t";
                for (Pin* pin : pins) { std::cout << pin->getId() << "\t"; }
                std::cout << std::endl;
            }
        }

        void print_solution() {
            std::cout << " ------------- chiplet " << _chiplet0->getId() << " and " << _chiplet1->getId() << " ------------- " << std::endl; 
            std::cout << "Solution: ";
            for (IndexType val : _solution) {
                std::cout << val << " ";
            }
            std::cout << std::endl;
        }

    public:
        std::function<bool(Pin*, Pin*, IndexType, IndexType)> judge_front_func0;
        std::function<bool(Pin*, Pin*, IndexType, IndexType)> judge_front_func1;

    private:
        Chiplet* _chiplet0;
        Chiplet* _chiplet1;
        std::vector<Pin*> _vpins0;
        std::vector<Pin*> _vpins1;
        std::vector<Pin*> _vppins0;
        std::vector<Pin*> _vppins1;
        SortedPinsType _sorted_pins0;
        SortedPinsType _sorted_pins1;
        Direction _direction0;
        Direction _direction1;
        std::vector<IndexType> _solution;

};

class Constraint {
    public:
        Constraint() { id = next_id++; };
        IndexType id;
        std::vector<VarIdCoeffPairType> varId_coeff_pair;
        ConType contype;
        RealType objective;

    protected:
        static inline IndexType next_id = 0;
};

class Constraints {
    public:
        Constraints() {}

        Constraint& addConstraint() {
            return _constraints.emplace_back(Constraint());
        }
        std::vector<Constraint>& allConstraints() { return _constraints; }
    private:
        std::vector<Constraint> _constraints;
};

class Channel {
    public:
        Channel(Direction direction) {}
};

class Layer {
    public:
        Layer() {}
        Layer(IndexType id) : _id(id) {}

        IndexType& id() { return _id; }
        PinEscPairType& new_pin_esc_pair() { return _pin_esc_pair.emplace_back(); }
        std::vector<PinEscPairType>& all_pin_esc_pair() { return _pin_esc_pair; }
        Track& new_track() { return _lines.emplace_back(); }
        std::vector<Track>& all_track() { return _lines; }

    private:
        IndexType _id;
        std::vector<PinEscPairType> _pin_esc_pair;
        std::vector<Track> _lines;
};



#endif
