#include "pybind11_wrapper.hpp"

#include "teqpversion.hpp"
#include "teqp/ideal_eosterms.hpp"
#include "teqp/cpp/derivs.hpp"

namespace py = pybind11;
using namespace py::literals;

#define stringify(A) #A

void add_multifluid(py::module& m);
void add_multifluid_mutant(py::module& m);

template<typename Model, int iT, int iD, typename Class>
void add_ig_deriv_impl(Class& cls) {
    using idx = TDXDerivatives<Model>;
    using RAX = Eigen::Ref<const Eigen::ArrayXd>;
    const std::string fname = "get_Aig" + std::to_string(iT) + std::to_string(iD);
    cls.def(fname.c_str(),
        [](const Model& m, const double T, const double rho, const RAX molefrac) { return idx::template get_Aigxy<iT, iD, ADBackends::autodiff>(m, T, rho, molefrac); },
        py::arg("T"), py::arg("rho"), py::arg("molefrac").noconvert()
    );
}

template<typename Model, typename Class>
void add_ig_derivatives(py::module& m, Class& cls) {
    add_ig_deriv_impl<Model, 0, 0>(cls); add_ig_deriv_impl<Model, 0, 1>(cls); add_ig_deriv_impl<Model, 0, 2>(cls); add_ig_deriv_impl<Model, 0, 3>(cls); add_ig_deriv_impl<Model, 0, 4>(cls);
    add_ig_deriv_impl<Model, 1, 0>(cls); add_ig_deriv_impl<Model, 1, 1>(cls); add_ig_deriv_impl<Model, 1, 2>(cls); add_ig_deriv_impl<Model, 1, 3>(cls); add_ig_deriv_impl<Model, 1, 4>(cls);
    add_ig_deriv_impl<Model, 2, 0>(cls); add_ig_deriv_impl<Model, 2, 1>(cls); add_ig_deriv_impl<Model, 2, 2>(cls); add_ig_deriv_impl<Model, 2, 3>(cls); add_ig_deriv_impl<Model, 2, 4>(cls);
}

template<typename TYPE>
const TYPE& get_typed(py::object& o){
    using namespace teqp::cppinterface;
    return std::get<TYPE>(o.cast<const AbstractModel *>()->get_model());
}

// You cannot know at runtime what is contained in the model so you must iterate
// over possible model types and attach methods accordingly
void attach_model_specific_methods(py::object& obj){
    using namespace teqp::cppinterface;
    const auto& model = obj.cast<AbstractModel *>()->get_model();
    auto setattr = py::getattr(obj, "__setattr__");
    auto MethodType = py::module_::import("types").attr("MethodType");
    
    if (std::holds_alternative<vdWEOS1>(model)){
        setattr("get_a", MethodType(py::cpp_function([](py::object& o){ return get_typed<vdWEOS1>(o).get_a(); }), obj));
        setattr("get_b", MethodType(py::cpp_function([](py::object& o){ return get_typed<vdWEOS1>(o).get_b(); }), obj));
    }
    else if (std::holds_alternative<PCSAFT_t>(model)){
        setattr("get_m", MethodType(py::cpp_function([](py::object& o){ return get_typed<PCSAFT_t>(o).get_m(); }), obj));
        setattr("get_sigma_Angstrom", MethodType(py::cpp_function([](py::object& o){ return get_typed<PCSAFT_t>(o).get_sigma_Angstrom(); }), obj));
        setattr("get_epsilon_over_k_K", MethodType(py::cpp_function([](py::object& o){ return get_typed<PCSAFT_t>(o).get_epsilon_over_k_K(); }), obj));
        setattr("max_rhoN", MethodType(py::cpp_function([](py::object& o, double T, REArrayd& molefrac){ return get_typed<PCSAFT_t>(o).max_rhoN(T, molefrac); }), obj));
    }
    else if (std::holds_alternative<canonical_cubic_t>(model)){
        setattr("get_a", MethodType(py::cpp_function([](py::object& o, double T, REArrayd& molefrac){ return get_typed<canonical_cubic_t>(o).get_a(T, molefrac); }), obj));
        setattr("get_b", MethodType(py::cpp_function([](py::object& o, double T, REArrayd& molefrac){ return get_typed<canonical_cubic_t>(o).get_b(T, molefrac); }), obj));
        setattr("superanc_rhoLV", MethodType(py::cpp_function([](py::object& o, double T){ return get_typed<canonical_cubic_t>(o).superanc_rhoLV(T); }), obj));
    }
};

/// Instantiate "instances" of models (really wrapped Python versions of the models), and then attach all derivative methods
void init_teqp(py::module& m) {

    // The options class for critical tracer, not tied to a particular model
    py::class_<TCABOptions>(m, "TCABOptions")
        .def(py::init<>())
        .def_readwrite("abs_err", &TCABOptions::abs_err)
        .def_readwrite("rel_err", &TCABOptions::rel_err)
        .def_readwrite("init_dt", &TCABOptions::init_dt)
        .def_readwrite("init_c", &TCABOptions::init_c)
        .def_readwrite("max_dt", &TCABOptions::max_dt)
        .def_readwrite("T_tol", &TCABOptions::T_tol)
        .def_readwrite("small_T_count", &TCABOptions::small_T_count)
        .def_readwrite("max_step_count", &TCABOptions::max_step_count)
        .def_readwrite("skip_dircheck_count", &TCABOptions::skip_dircheck_count)
        .def_readwrite("integration_order", &TCABOptions::integration_order)
        .def_readwrite("calc_stability", &TCABOptions::calc_stability)
        .def_readwrite("stability_rel_drho", &TCABOptions::stability_rel_drho)
        .def_readwrite("verbosity", &TCABOptions::verbosity)
        .def_readwrite("polish", &TCABOptions::polish)
        .def_readwrite("polish_reltol_rho", &TCABOptions::polish_reltol_rho)
        .def_readwrite("polish_reltol_T", &TCABOptions::polish_reltol_T)
        .def_readwrite("pure_endpoint_polish", &TCABOptions::pure_endpoint_polish)
        .def_readwrite("polish_exception_on_fail", &TCABOptions::polish_exception_on_fail)
        ;

    // The options class for isotherm tracer, not tied to a particular model
    py::class_<TVLEOptions>(m, "TVLEOptions")
        .def(py::init<>())
        .def_readwrite("abs_err", &TVLEOptions::abs_err)
        .def_readwrite("rel_err", &TVLEOptions::rel_err)
        .def_readwrite("init_dt", &TVLEOptions::init_dt)
        .def_readwrite("init_c", &TVLEOptions::init_c)
        .def_readwrite("max_dt", &TVLEOptions::max_dt)
        .def_readwrite("max_steps", &TVLEOptions::max_steps)
        .def_readwrite("integration_order", &TVLEOptions::integration_order)
        .def_readwrite("polish", &TVLEOptions::polish)
        .def_readwrite("calc_criticality", &TVLEOptions::calc_criticality)
        .def_readwrite("terminate_unstable", &TVLEOptions::terminate_unstable)
        ;

    // The options class for isobar tracer, not tied to a particular model
    py::class_<PVLEOptions>(m, "PVLEOptions")
        .def(py::init<>())
        .def_readwrite("abs_err", &PVLEOptions::abs_err)
        .def_readwrite("rel_err", &PVLEOptions::rel_err)
        .def_readwrite("init_dt", &PVLEOptions::init_dt)
        .def_readwrite("init_c", &PVLEOptions::init_c)
        .def_readwrite("max_dt", &PVLEOptions::max_dt)
        .def_readwrite("max_steps", &PVLEOptions::max_steps)
        .def_readwrite("integration_order", &PVLEOptions::integration_order)
        .def_readwrite("polish", &PVLEOptions::polish)
        .def_readwrite("calc_criticality", &PVLEOptions::calc_criticality)
        .def_readwrite("terminate_unstable", &PVLEOptions::terminate_unstable)
        ;

    // The options class for the finder of VLLE solutions from VLE tracing, not tied to a particular model
    py::class_<VLLEFinderOptions>(m, "VLLEFinderOptions")
        .def(py::init<>())
        .def_readwrite("max_steps", &VLLEFinderOptions::max_steps)
        .def_readwrite("rho_trivial_threshold", &VLLEFinderOptions::rho_trivial_threshold)
        ;

    py::class_<MixVLETpFlags>(m, "MixVLETpFlags")
        .def(py::init<>())
        .def_readwrite("atol", &MixVLETpFlags::atol)
        .def_readwrite("reltol", &MixVLETpFlags::reltol)
        .def_readwrite("axtol", &MixVLETpFlags::axtol)
        .def_readwrite("relxtol", &MixVLETpFlags::relxtol)
        .def_readwrite("maxiter", &MixVLETpFlags::maxiter)
        ;

    py::class_<MixVLEpxFlags>(m, "MixVLEpxFlags")
        .def(py::init<>())
        .def_readwrite("atol", &MixVLEpxFlags::atol)
        .def_readwrite("reltol", &MixVLEpxFlags::reltol)
        .def_readwrite("axtol", &MixVLEpxFlags::axtol)
        .def_readwrite("relxtol", &MixVLEpxFlags::relxtol)
        .def_readwrite("maxiter", &MixVLEpxFlags::maxiter)
        ;
    
    using namespace teqp::cppinterface;
    // The Jacobian and value matrices for Newton-Raphson
    py::class_<IterationMatrices>(m, "IterationMatrices")
        .def(py::init<>())
        .def_readonly("J", &IterationMatrices::J)
        .def_readonly("v", &IterationMatrices::v)
        .def_readonly("vars", &IterationMatrices::vars)
        ;

    py::enum_<VLE_return_code>(m, "VLE_return_code")
        .value("unset", VLE_return_code::unset)
        .value("xtol_satisfied", VLE_return_code::xtol_satisfied)
        .value("functol_satisfied", VLE_return_code::functol_satisfied)
        .value("maxiter_met", VLE_return_code::maxiter_met)
        .value("maxfev_met", VLE_return_code::maxfev_met)
        .value("notfinite_step", VLE_return_code::notfinite_step)
        ;

    py::class_<MixVLEReturn>(m, "MixVLEReturn")
        .def(py::init<>())
        .def_readonly("success", &MixVLEReturn::success)
        .def_readonly("message", &MixVLEReturn::message)
        .def_readonly("rhovecL", &MixVLEReturn::rhovecL)
        .def_readonly("rhovecV", &MixVLEReturn::rhovecV)
        .def_readonly("return_code", &MixVLEReturn::return_code)
        .def_readonly("num_iter", &MixVLEReturn::num_iter)
        .def_readonly("T", &MixVLEReturn::T)
        .def_readonly("num_fev", &MixVLEReturn::num_fev)
        .def_readonly("r", &MixVLEReturn::r)
        .def_readonly("initial_r", &MixVLEReturn::initial_r)
        ;
    
    using namespace teqp::PCSAFT;
    py::class_<SAFTCoeffs>(m, "SAFTCoeffs")
    .def(py::init<>())
    .def_readwrite("name", &SAFTCoeffs::name)
    .def_readwrite("m", &SAFTCoeffs::m)
    .def_readwrite("sigma_Angstrom", &SAFTCoeffs::sigma_Angstrom)
    .def_readwrite("epsilon_over_k", &SAFTCoeffs::epsilon_over_k)
    .def_readwrite("BibTeXKey", &SAFTCoeffs::BibTeXKey)
    ;

    // The ideal gas Helmholtz energy class
    auto alphaig = py::class_<IdealHelmholtz>(m, "IdealHelmholtz").def(py::init<const nlohmann::json&>());
    alphaig.def_static("convert_CoolProp_format", [](const std::string &path, int index){return convert_CoolProp_idealgas(path, index);});
    add_ig_derivatives<IdealHelmholtz>(m, alphaig);
    alphaig.def("get_deriv_mat2", [](const IdealHelmholtz &ig, double T, double rho, const Eigen::ArrayXd& z){return DerivativeHolderSquare<2, AlphaWrapperOption::idealgas>(ig, T, rho, z).derivs;});

    add_multifluid(m);
    add_multifluid_mutant(m);
    
    using am = teqp::cppinterface::AbstractModel;
    py::class_<AbstractModel, std::shared_ptr<AbstractModel>>(m, "AbstractModel", py::dynamic_attr())
    
        .def("get_R", &am::get_R, "molefrac"_a.noconvert())
    
        .def("get_B2vir", &am::get_B2vir, "T"_a, "molefrac"_a.noconvert())
        .def("get_Bnvir", &am::get_Bnvir, "Nderiv"_a, "T"_a, "molefrac"_a.noconvert())
        .def("get_dmBnvirdTm", &am::get_dmBnvirdTm, "Nderiv"_a, "NTderiv"_a, "T"_a, "molefrac"_a.noconvert())
        .def("get_B12vir", &am::get_B12vir, "T"_a, "molefrac"_a.noconvert())
    
        .def("get_Arxy", &am::get_Arxy, "NT"_a, "ND"_a, "T"_a, "rho"_a, "molefrac"_a.noconvert())
        // Here X-Macros are used to create functions like get_Ar00, get_Ar01, ....
        #define X(i,j) .def(stringify(get_Ar ## i ## j), &am::get_Ar ## i ## j, "T"_a, "rho"_a, "molefrac"_a.noconvert())
            ARXY_args
        #undef X
        // And like get_Ar01n, get_Ar02n, ....
        #define X(i) .def(stringify(get_Ar0 ## i ## n), &am::get_Ar0 ## i ## n, "T"_a, "rho"_a, "molefrac"_a.noconvert())
            AR0N_args
        #undef X
        .def("get_neff", &am::get_neff, "T"_a, "rho"_a, "molefrac"_a.noconvert())
    
        // Methods that come from the isochoric derivatives formalism
        .def("get_pr", &am::get_pr, "T"_a, "rhovec"_a.noconvert())
        .def("get_splus", &am::get_splus, "T"_a, "rhovec"_a.noconvert())
        .def("build_Psir_Hessian_autodiff", &am::build_Psir_Hessian_autodiff, "T"_a, "rhovec"_a.noconvert())
        .def("build_Psi_Hessian_autodiff", &am::build_Psi_Hessian_autodiff, "T"_a, "rhovec"_a.noconvert())
        .def("build_Psir_gradient_autodiff", &am::build_Psir_gradient_autodiff, "T"_a, "rhovec"_a.noconvert())
        .def("build_d2PsirdTdrhoi_autodiff", &am::build_d2PsirdTdrhoi_autodiff, "T"_a, "rhovec"_a.noconvert())
        .def("get_chempotVLE_autodiff", &am::get_chempotVLE_autodiff, "T"_a, "rhovec"_a.noconvert())
        .def("get_dchempotdT_autodiff", &am::get_dchempotdT_autodiff, "T"_a, "rhovec"_a.noconvert())
        .def("get_fugacity_coefficients", &am::get_fugacity_coefficients, "T"_a, "rhovec"_a.noconvert())
        .def("get_partial_molar_volumes", &am::get_partial_molar_volumes, "T"_a, "rhovec"_a.noconvert())
    
        .def("get_deriv_mat2", &am::get_deriv_mat2, "T"_a, "rho"_a, "molefrac"_a.noconvert())
    
        // Routines related to pure fluid critical point calculation
        .def("get_pure_critical_conditions_Jacobian", &am::get_pure_critical_conditions_Jacobian, "T"_a, "rho"_a, py::arg_v("alternative_pure_index", -1), py::arg_v("alternative_length", 2))
        .def("solve_pure_critical", &am::solve_pure_critical, "T"_a, "rho"_a, py::arg_v("flags", std::nullopt, "None"))
        .def("extrapolate_from_critical", &am::extrapolate_from_critical, "Tc"_a, "rhoc"_a, "T"_a)
    
        // Routines related to binary mixture critical curve tracing
        .def("trace_critical_arclength_binary", &am::trace_critical_arclength_binary, "T0"_a, "rhovec0"_a, py::arg_v("path", std::nullopt, "None"), py::arg_v("options", std::nullopt, "None"))
        .def("get_criticality_conditions", &am::get_criticality_conditions, "T"_a, "rhovec"_a.noconvert())
        .def("eigen_problem", &am::eigen_problem, "T"_a, "rhovec"_a, py::arg_v("alignment_v0", std::nullopt, "None"))
        .def("get_minimum_eigenvalue_Psi_Hessian", &am::get_minimum_eigenvalue_Psi_Hessian, "T"_a, "rhovec"_a.noconvert())
        .def("get_drhovec_dT_crit", &am::get_drhovec_dT_crit, "T"_a, "rhovec"_a.noconvert())
        .def("get_dp_dT_crit", &am::get_dp_dT_crit, "T"_a, "rhovec"_a.noconvert())

        .def("pure_VLE_T", &am::pure_VLE_T, "T"_a, "rhoL"_a, "rhoV"_a, "max_iter"_a)

        .def("get_drhovecdp_Tsat", &am::get_drhovecdp_Tsat, "T"_a, "rhovecL"_a.noconvert(), "rhovecV"_a.noconvert())
        .def("get_drhovecdT_psat", &am::get_drhovecdT_psat, "T"_a, "rhovecL"_a.noconvert(), "rhovecV"_a.noconvert())
        .def("get_dpsat_dTsat_isopleth", &am::get_dpsat_dTsat_isopleth, "T"_a, "rhovecL"_a.noconvert(), "rhovecV"_a.noconvert())
    
        .def("trace_VLE_isotherm_binary", &am::trace_VLE_isotherm_binary, "T"_a, "rhovecL0"_a.noconvert(), "rhovecV0"_a.noconvert(), py::arg_v("options", std::nullopt, "None"))
        .def("trace_VLE_isobar_binary", &am::trace_VLE_isobar_binary, "p"_a, "T0"_a, "rhovecL0"_a.noconvert(), "rhovecV0"_a.noconvert(), py::arg_v("options", std::nullopt, "None"))
        .def("mix_VLE_Tx", &am::mix_VLE_Tx, "T"_a, "rhovecL0"_a.noconvert(), "rhovecV0"_a.noconvert(), "xspec"_a.noconvert(), "atol"_a, "reltol"_a, "axtol"_a, "relxtol"_a, "maxiter"_a)
        .def("mix_VLE_Tp", &am::mix_VLE_Tp, "T"_a, "p_given"_a, "rhovecL0"_a.noconvert(), "rhovecV0"_a.noconvert(), py::arg_v("options", std::nullopt, "None"))
        .def("mixture_VLE_px", &am::mixture_VLE_px, "p_spec"_a, "xmolar_spec"_a.noconvert(), "T0"_a, "rhovecL0"_a.noconvert(), "rhovecV0"_a.noconvert(), py::arg_v("options", std::nullopt, "None"))
    
//        .def("mix_VLLE_T", &mix_VLLE_T<Model, double, RAX>);
//        .def("find_VLLE_T_binary", &am::find_VLLE_T_binary, "traces"_a, py::arg_v("options", std::nullopt, "None"));
    ;
    
    m.def("_make_model", &teqp::cppinterface::make_model);
    m.def("attach_model_specific_methods", &attach_model_specific_methods);
    
//    // Some functions for timing overhead of interface
//    m.def("___mysummer", [](const double &c, const Eigen::ArrayXd &x) { return c*x.sum(); });
//    using RAX = Eigen::Ref<const Eigen::ArrayXd>;
//    using namespace pybind11::literals; // for "arg"_a
//    m.def("___mysummerref", [](const double& c, const RAX x) { return c * x.sum(); }, "c"_a, "x"_a.noconvert());
//    m.def("___myadder", [](const double& c, const double& d) { return c+d; });
}

PYBIND11_MODULE(teqp, m) {
    m.doc() = "TEQP: Templated Equation of State Package";
    m.attr("__version__") = TEQPVERSION;
    init_teqp(m);
}
