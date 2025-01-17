#pragma once

#include "nlohmann/json.hpp"

#include <Eigen/Dense>
#include "teqp/derivs.hpp"
#include "teqp/algorithms/rootfinding.hpp"
#include "teqp/exceptions.hpp"

namespace teqp {
    /**
    * Calculate the criticality conditions for a pure fluid and its Jacobian w.r.t. the temperature and density
    * for additional fine tuning with multi-variate rootfinding
    *
    */
    template<typename Model, typename Scalar, ADBackends backend = ADBackends::autodiff>
    auto get_pure_critical_conditions_Jacobian(const Model& model, const Scalar T, const Scalar rho,
        int alternative_pure_index = -1, int alternative_length = 2) {

        using tdx = TDXDerivatives<Model>;
        Eigen::ArrayXd z;
        if (alternative_pure_index == -1) {
            z = (Eigen::ArrayXd(1) << 1.0).finished();
        }
        else {
            z = Eigen::ArrayXd(alternative_length); z.setZero();
            z(alternative_pure_index) = 1.0;
        }
        auto R = model.R(z);

        auto ders = tdx::template get_Ar0n<4, backend>(model, T, rho, z);

        auto dpdrho = R * T * (1 + 2 * ders[1] + ders[2]); // Should be zero at critical point
        auto d2pdrho2 = R * T / rho * (2 * ders[1] + 4 * ders[2] + ders[3]); // Should be zero at critical point

        auto resids = (Eigen::ArrayXd(2) << dpdrho, d2pdrho2).finished();

        /*  Sympy code for derivatives:
        import sympy as sy
        rho, R, Trecip,T = sy.symbols('rho,R,(1/T),T')
        alphar = sy.symbols('alphar', cls=sy.Function)(Trecip, rho)
        p = rho*R/Trecip*(1 + rho*sy.diff(alphar,rho))
        dTrecip_dT = -1/T**2

        sy.simplify(sy.diff(p,rho,3).replace(Trecip,1/T))

        sy.simplify(sy.diff(sy.diff(p,rho,1),Trecip)*dTrecip_dT)

        sy.simplify(sy.diff(sy.diff(p,rho,2),Trecip)*dTrecip_dT)
        */

        // Note: these derivatives are expressed in terms of 1/T and rho as independent variables
        auto Ar11 = tdx::template get_Arxy<1, 1, backend>(model, T, rho, z);
        auto Ar12 = tdx::template get_Arxy<1, 2, backend>(model, T, rho, z);
        auto Ar13 = tdx::template get_Arxy<1, 3, backend>(model, T, rho, z);

        auto d3pdrho3 = R * T / (rho * rho) * (6 * ders[2] + 6 * ders[3] + ders[4]);
        auto d_dpdrho_dT = R * (-(Ar12 + 2 * Ar11) + ders[2] + 2 * ders[1] + 1);
        auto d_d2pdrho2_dT = R / rho * (-(Ar13 + 4 * Ar12 + 2 * Ar11) + ders[3] + 4 * ders[2] + 2 * ders[1]);

        // Jacobian of resid terms w.r.t. T and rho
        Eigen::MatrixXd J(2, 2);
        J(0, 0) = d_dpdrho_dT; // d(dpdrho)/dT
        J(0, 1) = d2pdrho2; // d2pdrho2
        J(1, 0) = d_d2pdrho2_dT; // d(d2pdrho2)/dT
        J(1, 1) = d3pdrho3; // d3pdrho3

        return std::make_tuple(resids, J);
    }

    template<typename Model, typename Scalar, ADBackends backend = ADBackends::autodiff>
    auto solve_pure_critical(const Model& model, const Scalar T0, const Scalar rho0, const nlohmann::json& flags = {}) {
        auto x = (Eigen::ArrayXd(2) << T0, rho0).finished();
        int maxsteps = (flags.contains("maxsteps")) ? flags.at("maxsteps").get<int>() : 10;
        int alternative_pure_index = (flags.contains("alternative_pure_index")) ? flags.at("alternative_pure_index").get<int>() : -1;
        int alternative_length = (flags.contains("alternative_length")) ? flags.at("alternative_length").get<int>() : 2;
        // A convenience method to make linear system solving more concise with Eigen datatypes
        // All arguments are converted to matrices, the solve is done, and an array is returned
        auto linsolve = [](const auto& a, const auto& b) {
            return a.matrix().colPivHouseholderQr().solve(b.matrix()).array().eval();
        };
        for (auto counter = 0; counter < maxsteps; ++counter) {
            auto [resids, Jacobian] = get_pure_critical_conditions_Jacobian<Model, Scalar, backend>(model, x[0], x[1], alternative_pure_index, alternative_length);
            auto v = linsolve(Jacobian, -resids);
            x += v;
        }
        return std::make_tuple(x[0], x[1]);
    }

    template<typename Model, typename Scalar>
    Eigen::ArrayXd extrapolate_from_critical(const Model& model, const Scalar& Tc, const Scalar& rhoc, const Scalar& T) {

        using tdx = TDXDerivatives<Model, Scalar>;
        auto z = (Eigen::ArrayXd(1) << 1.0).finished();
        auto R = model.R(z);
        auto ders = tdx::template get_Ar0n<4>(model, Tc, rhoc, z);
        //auto dpdrho = R*Tc*(1 + 2 * ders[1] + ders[2]); // Should be zero
        //auto d2pdrho2 = R*Tc/rhoc*(2 * ders[1] + 4 * ders[2] + ders[3]); // Should be zero
        auto d3pdrho3 = R * Tc / (rhoc * rhoc) * (6 * ders[2] + 6 * ders[3] + ders[4]);
        auto Ar11 = tdx::template get_Ar11(model, Tc, rhoc, z);
        auto Ar12 = tdx::template get_Ar12(model, Tc, rhoc, z);
        auto d2pdrhodT = R * (1 + 2 * ders[1] + ders[2] - 2 * Ar11 - Ar12);
        auto Brho = sqrt(6 * d2pdrhodT * Tc / d3pdrho3);

        auto drhohat_dT = Brho / Tc;
        auto dT = T - Tc;

        auto drhohat = dT * drhohat_dT;
        auto rholiq = -drhohat / sqrt(1 - T / Tc) + rhoc;
        auto rhovap = drhohat / sqrt(1 - T / Tc) + rhoc;
        return (Eigen::ArrayXd(2) << rholiq, rhovap).finished();
    }
};