#include<iostream>
#include"flow.h"
#include"GRouter.h"

bool RUN_RDL = false;
bool CONSIDER_DTC_BLKG = false;

int main(int argc, char* argv[]) {
    std::cout << "Hello World. " << std::endl;
    // BIPGRouter gr("/export/home/jhhuang/Desktop/chiplet-opt-flow/ICCAD25/", "celldata/case_cluster/");
    // BIPGRouter gr(argv[1], argv[2]);
    // gr.run();
    
    for (IndexType i = 3; i < argc; i++) {
        if      (std::strcmp(argv[i], "-RD") == 0) { RUN_RDL = true; std::cout << "run redistribution. " << std::endl; }
        else if (std::strcmp(argv[i], "-CB") == 0) { CONSIDER_DTC_BLKG = true; std::cout << "run DTC blockage. " << std::endl; }
    }

    MainFlow mainflow(argv[1], argv[2]);
    mainflow.runflow();
}
