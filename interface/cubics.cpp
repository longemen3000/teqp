#include "pybind11_wrapper.hpp"

#include "teqp/models/cubics.hpp"

void add_cubics(py::module& m) {

    using va = std::valarray<double>;
    
    m.def("canonical_PR", &canonical_PR<va,va,va>, py::arg("Tc_K"), py::arg("pc_Pa"), py::arg("acentric"), py::arg_v("kmat", Eigen::ArrayXXd(0, 0), "None"));
    m.def("canonical_SRK", &canonical_SRK<va, va, va>, py::arg("Tc_K"), py::arg("pc_Pa"), py::arg("acentric"), py::arg_v("kmat", Eigen::ArrayXXd(0, 0), "None"));

    using cub = decltype(canonical_PR(va{}, va{}, va{}));
    auto wcub = py::class_<cub>(m, "GenericCubic")
        .def("get_meta", &cub::get_meta)
        .def("superanc_rhoLV", &cub::superanc_rhoLV)
        .def("get_a", &cub::get_a<double, Eigen::ArrayXd>, py::arg("T"), py::arg("molefrac").noconvert())
        .def("get_b", &cub::get_b<double, Eigen::ArrayXd>, py::arg("T"), py::arg("molefrac").noconvert())
        ;
    add_derivatives<cub>(m, wcub);
}