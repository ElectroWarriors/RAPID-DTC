#include <ilcplex/ilocplex.h>
#include <iostream>
#include <vector>
#include <thread>

class ILP_Solver {
public:
    // 构造函数：初始化CPLEX环境和模型
    ILP_Solver() : _env(), _model(_env), _cplex(_model) {
        _cplex.setOut(_env.getNullStream()); // 关闭冗余输出
        _cplex.setParam(IloCplex::Threads, std::thread::hardware_concurrency()); // 启用多线程支持
        _cplex.setParam(IloCplex::Param::Parallel, 1); // 1: 自动选择, -1: 顺序模式, 0: 确定性并行
    }

    // 添加二进制变量（0-1变量）
    void addBinaryVariables(IndexType numVars) {
        std::cout << "numVars: " << numVars << std::endl;
        _vars = IloBoolVarArray(_env, numVars);
        _model.add(_vars);
    }

    // 设置目标函数（最大化）
    void setMaximizeObjective(const std::vector<double>& coeffs) {
        IloExpr obj(_env);
        for (int i = 0; i < coeffs.size(); ++i) {
            obj += coeffs[i] * _vars[i];
        }
        _model.add(IloMaximize(_env, obj));
        obj.end();
    }

    // 添加线性约束（表达式 <= rhs）
    // void ILPaddConstraint(const std::vector<std::pair<int, double>>& terms, double rhs) {
    //     IloExpr expr(_env);
    //     for (const auto& term : terms) {
    //         expr += term.second * _vars[term.first];
    //     }
    //     _model.add(expr <= rhs);
    //     expr.end();
    // }
    void ILPaddConstraints(Constraints cons) {
        std::cout << "numCons: " << cons.allConstraints().size() << std::endl;
        for (Constraint& con : cons.allConstraints()) { 
            IloExpr expr(_env);
            for (VarIdCoeffPairType var_coeff_pair : con.varId_coeff_pair) {
                expr += var_coeff_pair.second * _vars[var_coeff_pair.first];
            }
            switch (con.contype) {
                case ConType::LessEq   : _model.add(expr <= con.objective); break;
                case ConType::GreaterEq: _model.add(expr >= con.objective); break;
                case ConType::Equal    : _model.add(expr == con.objective); break;
                default                : break;
            }
            expr.end();
        }
    }

    // 求解问题
    bool solve() {
        try {
            return _cplex.solve();
        } catch (const IloException& e) {
            std::cerr << "CPLEX Error: " << e << std::endl;
            return false;
        }
    }

    // 获取目标函数值
    double getObjectiveValue() const {
        return _cplex.getObjValue();
    }

    // 获取变量解值
    std::vector<IndexType> getSolution() const {
        std::vector<IndexType> sol(_vars.getSize());
        for (IndexType i = 0; i < _vars.getSize(); ++i) {
            sol[i] = static_cast<IndexType>(_cplex.getValue(_vars[i]));
        }
        return sol;
    }

    // 析构函数：释放资源
    ~ILP_Solver() {
        _cplex.end();
        _model.end();
        _env.end();
    }

private:
    IloEnv _env;          // CPLEX环境
    IloModel _model;      // 优化模型
    IloCplex _cplex;      // 求解器实例
    IloBoolVarArray _vars; // 二进制变量数组
};

