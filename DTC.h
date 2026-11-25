#ifndef DTC_H_
#define DTC_H_

#include<iostream>
#include<vector>
#include <algorithm>
#include <map>
#include <tuple>
#include<vector>
#include<random>
#include<algorithm>

#include"global.h"

class RectInsert {
    public:
        typedef std::pair<LocType, LocType> LocLUType;
        RectInsert() {}
        void setBoundary(std::vector<LocType> x, std::vector<LocType> yl, std::vector<LocType> yu) {
            _vx = x; _vyL = yl; _vyH = yu;
        }

        void setRectShape(LocType w, LocType h) {
            _rectWidth = w; _rectHeight = h;
        }

        LocLUType XcalculateYBdy(LocType x) { // return lower, upper
            if (x < _vx.front() || x > _vx.back()) {
                return LocLUType(LOC_TYPE_MIN, LOC_TYPE_MAX); 
            }
            auto linear_func = [&]() {};
            IndexType idx;
            for (idx = 0; idx < _vx.size() - 1; idx++) {
                if (x >= _vx[idx] && x <= _vx[idx + 1]) { break; }
            }
            LocType yL, yH;
            if (_vx[idx] == _vx[idx+1]) {
                yL = std::min(_vyL[idx], _vyL[idx+1]);
                yH = std::max(_vyH[idx], _vyH[idx+1]);
            } else {
                yL = _vyL[idx] + (x - _vx[idx]) * (_vyL[idx+1] - _vyL[idx]) / (_vx[idx+1] - _vx[idx]);
                yH = _vyH[idx] + (x - _vx[idx]) * (_vyH[idx+1] - _vyH[idx]) / (_vx[idx+1] - _vx[idx]);
            }
            return LocLUType(yL, yH);
        }

        LocLUType vXcalculateYBdy(LocType xs, LocType w) {
            LocLUType ybdy = XcalculateYBdy(xs);
            std::vector<LocType> vx = _vx; vx.emplace_back(xs + w);
            for (auto x : vx) {
                if (x > xs && x <= xs + w) {
                    LocLUType ylu = XcalculateYBdy(x);
                    ybdy.first  = std::max(ybdy.first , ylu.first );
                    ybdy.second = std::min(ybdy.second, ylu.second);
                }
            }
            return ybdy;
        }


        void Sweep() {
            LocType xs = _vx.front();
            while (xs + _rectWidth < _vx.back()) {
                auto ylu_w = vXcalculateYBdy(xs, _rectWidth);
                if (ylu_w.second - ylu_w.first < _rectHeight) { 
                    xs += _rectWidth / 10;
                    continue; 
                }
                IndexType numIns = (ylu_w.second - ylu_w.first) / _rectHeight;
                LocType delta = (ylu_w.second - ylu_w.first) / numIns;
                for (IndexType idx = 0; idx < numIns; idx++) {
                    _vrectCLoc.emplace_back(LocXYType(xs + _rectWidth/2, ylu_w.first + delta/2 + idx*delta));
                }
                xs += _rectWidth;
            }
        }

        // std::vector<LocXYType> flipXaxes() {
        //     auto min_it = std::min_element(_vyL.begin(), _vyL.end());
        //     auto max_it = std::max_element(_vyH.begin(), _vyH.end());
        //     if (min_it == _vyL.end() || max_it == _vyH.end()) {
        //         throw std::invalid_argument("cannot find min/max value???");
        //     }
        //     LocType yl = *min_it, yu = *max_it;
        //     std::vector<LocXYType> vrectCLoc =_vrectCLoc;
        //     for (auto& xy : vrectCLoc) {
        //         xy.second = yl + (yu - xy.second);
        //     }
        //     return vrectCLoc;
        // }
        std::vector<LocXYType> flipXaxes() {
            std::vector<LocXYType> vrectCLoc = _vrectCLoc;
            for (auto& xy : vrectCLoc) {
                xy.second = - xy.second;
            }
            return vrectCLoc;
        }

        // std::vector<LocXYType> flipYaxes() {
        //     LocType xl = _vx.front(), xu = _vx.back();
        //     std::vector<LocXYType> vrectCLoc = _vrectCLoc;
        //     for (auto& xy : vrectCLoc) {
        //         xy.first = xl + (xu - xy.first);
        //     }
        //     return vrectCLoc;
        // }

        std::vector<LocXYType> invRotate90() {
            std::vector<LocXYType> vrectCLoc = _vrectCLoc;
            for (auto& xy : vrectCLoc) {
                LocType tmp;
                tmp = -xy.second; xy.second = xy.first; xy.first = tmp;
            }
            return vrectCLoc;
        }

        std::vector<LocXYType> invRotate90andFlipYaxes() {
            std::vector<LocXYType> vrectCLoc = _vrectCLoc;
            for (auto& xy : vrectCLoc) {
                LocType tmp;
                tmp = xy.second; xy.second = xy.first; xy.first = tmp;
            }
            return vrectCLoc;
        }

        std::vector<LocXYType>& allRect() { return _vrectCLoc; }

        std::vector<LocXYType> selectRandomElements(std::vector<LocXYType> vec, IndexType num) {
            num = std::min(num, static_cast<IndexType>(vec.size()));
            std::vector<LocXYType> result;
            result.reserve(num); // 预分配空间
        
            std::random_device rd;
            std::default_random_engine rng(rd());
        
            // 从 vec 中随机采样 n 个元素到 result
            std::sample(vec.begin(), vec.end(), std::back_inserter(result), num, rng);
            // std::cout << "result: " << result.size() << std::endl;
        
            return result;
        }

    private:
        std::vector<LocType> _vx;
        std::vector<LocType> _vyL;
        std::vector<LocType> _vyH;
        LocType _rectWidth;
        LocType _rectHeight;
        // output
        std::vector<LocXYType> _vrectCLoc;
};


class DTCTemplateDatabase {
    public:
        DTCTemplateDatabase() {}

        bool CreateTemplate(LocType pin_distance, LocType pin_size, LocType blockage, LocType rectshape) {
            auto key = std::make_tuple(pin_distance, pin_size, blockage);
            RectInsert value;
            auto result = _all_dtc_template.emplace(key, value);
            if (result.second) {
                // std::cout << "Insert successful!" << std::endl;
                ;
            } else {
                std::cout << "Key already exists!" << std::endl;
                return false;
            }
            // auto& it = result.first;
            // up as example
            std::vector<LocType> xx, yl, yu;
            if (blockage == -1) {
                xx = {0, pin_size, pin_size, pin_distance};
                yl = {pin_size, pin_size, 0, 0};
                yu = {pin_distance, pin_distance, pin_distance, pin_distance};
            } else {
                xx = {0, pin_size, pin_size, pin_size+blockage};
                yl = {0, 0, 0, blockage};
                yu = {pin_distance-pin_size, pin_distance-pin_size, pin_distance, pin_distance+blockage};
            }
            result.first->second.setRectShape(rectshape, rectshape);
            result.first->second.setBoundary(xx, yl, yu);
            result.first->second.Sweep();
            return true;
        }

        bool CreateTSVTemplate(LocType tsv_xdistance, LocType tsv_ydistance, LocType tsv_size, LocType rectshape) {
            _tsv_template.setRectShape(rectshape, rectshape);
            _tsv_template.setBoundary({0, tsv_xdistance}, {0, 0}, {tsv_ydistance, tsv_ydistance});
            _tsv_template.Sweep();

            LocType tsv_xLo = tsv_xdistance / 2 - tsv_size / 2, tsv_xHi = tsv_xdistance / 2 + tsv_size / 2;
            LocType tsv_yLo = tsv_ydistance / 2 - tsv_size / 2, tsv_yHi = tsv_ydistance / 2 + tsv_size / 2;
            auto is_overlap = [tsv_xLo, tsv_xHi, tsv_yLo, tsv_yHi, rectshape] (LocXYType xy) -> bool {
                if (std::max(xy.first +rectshape/2, tsv_xHi)-std::min(xy.first -rectshape/2, tsv_xLo) < (rectshape) + (tsv_xHi-tsv_xLo) && \
                    std::max(xy.second+rectshape/2, tsv_yHi)-std::min(xy.second-rectshape/2, tsv_yLo) < (rectshape) + (tsv_yHi-tsv_yLo)) {
                        return true;
                }
                return false;
            };
            auto& rects = _tsv_template.allRect();
            // rects.erase(std::remove_if(rects.begin(), rects.end(), is_overlap), rects.end());

            std::vector<LocXYType> new_rects;
            for (const auto& xy : rects) { if ( !is_overlap(xy)) { new_rects.emplace_back(xy); } }
            rects.swap(new_rects);
            
            return true; 
        }

        RectInsert getTemplate(LocType pin_distance, LocType pin_size, LocType blockage) {
            auto key = std::make_tuple(pin_distance, pin_size, blockage);
            auto it = _all_dtc_template.find(key);
            if (it == _all_dtc_template.end()) {
                throw std::invalid_argument("no such key tuple");
            }
            return it->second;
        }

        RectInsert getTSVTemplate() {
            return _tsv_template;
        }
        

    private:
        //            pin distance, pin size, blockage
        std::map<std::tuple<LocType, LocType, LocType>, RectInsert> _all_dtc_template;
        RectInsert _tsv_template;

};






#endif