/**
* This is a minimal example of the use of the C++ interface around teqp
* 
* The point of the interface is to reduce compilation time, as compilation 
* of this file should be MUCH faster than compilation with the full set of 
* algorithms and models because the template instantiations are all included in the 
* library that this file is linked against and the compiler does not need to resolve
* all the templates every time this file is compiled
*/

#include "teqpcpp.hpp"
#include <iostream>

int main() {
    nlohmann::json j = { 
        {"kind", "multifluid"}, 
        {"model", {
            {"components", {"../mycp/dev/fluids/Methane.json","../mycp/dev/fluids/Ethane.json"}},
            {"BIP", "../mycp/dev/mixtures/mixture_binary_pairs.json"},
            {"departure", "../mycp/dev/mixtures/mixture_departure_functions.json"}
        }
    }};
    //std::cout << j.dump(2);
    auto am = teqp::cppinterface::make_model(j);

    auto z = (Eigen::ArrayXd(2) << 0.5, 0.5).finished();
    double Ar01 = am->get_Arxy(0, 1, 300, 3, z);
    std::cout << Ar01 << std::endl;
}