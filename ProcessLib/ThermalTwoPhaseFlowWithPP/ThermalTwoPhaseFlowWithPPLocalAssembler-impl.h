/**
 * \file
 * \copyright
 * Copyright (c) 2012-2022, OpenGeoSys Community (http://www.opengeosys.org)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.org/project/license
 *
 */

/**
 * Common convenitions for naming:
 * X_gas_nonwet           mass fraction of gas component(e.g air) in nonwetting
 * phase (gas component doesn't include water vapour, same for the following)
 * x_gas_nonwet           molar fraction of gas component in nonwetting phase
 * x_vapour_nonwet         molar fraction of vapour in nonwetting phase
 * p_vapour_nonwet         water vapour pressure
 * p_gas_nonwet           partial pressure of gas component
 * mol_density_nonwet         molar density of nonwetting phase
 * mol_density_water          molar density of water
 * density_water              mass density of water
 * density_nonwet_gas         mass density of gas component in the nonwetting
 * phase density_nonwet_vapour       mass density of vapour in the nonwetting
 * phase density_nonwet             mass density of the nonwetting phase
 * density_wet                mass density of wetting pahse
 * density_solid              mass density of the solid phase
 * velocity_nonwet              velocity of nonwetting phase
 * velocity_wet                 velocity of wetting phase
 * heat_capacity_dry_gas        heat capacity of dry gas
 * heat_capacity_water_vapour    heat capacity of water vapour
 * heat_capacity_water          heat capacity of liquid water
 * heat_capacity_solid          heat capacity of soil grain
 * latent_heat_evaporation      latent heat for evaporation(water to vapour)
 * enthalpy_nonwet_gas          enthalpy of gas component in the nonwetting
 * phase enthalpy_nonwet_vapour        enthalpy of water vapour in the
 * nonwetting phase enthalpy_wet                 enthalpy of wetting phase
 * enthalpy_nonwet                 enthalpy of the nonwetting phase
 * internal_energy_nonwet        specific internal energy for the nonwetting
 * phase internal_energy_wet           specific internal energy for the wetting
 * phase heat_conductivity_dry_solid   heat conductivity of the dry porous
 * medium heat_conductivity_wet_solid   heat conductivity of the fully saturated
 * porous medium heat_conductivity_unsaturated   heat conductivity of the
 * unsaturated porous medium
 */
#pragma once

#include "MaterialLib/MPL/Medium.h"
#include "MaterialLib/MPL/Property.h"
#include "MaterialLib/MPL/Utils/FormEffectiveThermalConductivity.h"
#include "MaterialLib/MPL/Utils/FormEigenTensor.h"
#include "MathLib/InterpolationAlgorithms/PiecewiseLinearInterpolation.h"
#include "NumLib/Function/Interpolation.h"
#include "ThermalTwoPhaseFlowWithPPLocalAssembler.h"
#include "ThermalTwoPhaseFlowWithPPProcessData.h"

namespace ProcessLib
{
namespace ThermalTwoPhaseFlowWithPP
{
template <typename ShapeFunction, typename IntegrationMethod, int GlobalDim>
void ThermalTwoPhaseFlowWithPPLocalAssembler<
    ShapeFunction, IntegrationMethod,
    GlobalDim>::assemble(double const t, double const dt,
                         std::vector<double> const& local_x,
                         std::vector<double> const& /*local_xdot*/,
                         std::vector<double>& local_M_data,
                         std::vector<double>& local_K_data,
                         std::vector<double>& local_b_data)
{
    using MaterialLib::PhysicalConstant::CelsiusZeroInKelvin;
    using MaterialLib::PhysicalConstant::IdealGasConstant;

    auto const local_matrix_size = local_x.size();

    assert(local_matrix_size == ShapeFunction::NPOINTS * NUM_NODAL_DOF);

    auto local_M = MathLib::createZeroedMatrix<LocalMatrixType>(
        local_M_data, local_matrix_size, local_matrix_size);
    auto local_K = MathLib::createZeroedMatrix<LocalMatrixType>(
        local_K_data, local_matrix_size, local_matrix_size);
    auto local_b = MathLib::createZeroedVector<LocalVectorType>(
        local_b_data, local_matrix_size);

    auto Mgp =
        local_M.template block<nonwet_pressure_size, nonwet_pressure_size>(
            nonwet_pressure_matrix_index, nonwet_pressure_matrix_index);
    auto Mgpc = local_M.template block<nonwet_pressure_size, cap_pressure_size>(
        nonwet_pressure_matrix_index, cap_pressure_matrix_index);
    auto Mgt = local_M.template block<nonwet_pressure_size, temperature_size>(
        nonwet_pressure_matrix_index, temperature_matrix_index);

    auto Mlp = local_M.template block<cap_pressure_size, nonwet_pressure_size>(
        cap_pressure_matrix_index, nonwet_pressure_matrix_index);
    auto Mlpc = local_M.template block<cap_pressure_size, cap_pressure_size>(
        cap_pressure_matrix_index, cap_pressure_matrix_index);
    auto Mlt = local_M.template block<cap_pressure_size, temperature_size>(
        cap_pressure_matrix_index, temperature_matrix_index);

    auto Mep = local_M.template block<temperature_size, nonwet_pressure_size>(
        temperature_matrix_index, nonwet_pressure_matrix_index);
    auto Mepc = local_M.template block<temperature_size, cap_pressure_size>(
        temperature_matrix_index, cap_pressure_matrix_index);
    auto Met = local_M.template block<temperature_size, temperature_size>(
        temperature_matrix_index, temperature_matrix_index);

    NodalMatrixType laplace_operator =
        NodalMatrixType::Zero(ShapeFunction::NPOINTS, ShapeFunction::NPOINTS);

    auto Kgp =
        local_K.template block<nonwet_pressure_size, nonwet_pressure_size>(
            nonwet_pressure_matrix_index, nonwet_pressure_matrix_index);
    auto Kgpc = local_K.template block<nonwet_pressure_size, cap_pressure_size>(
        nonwet_pressure_matrix_index, cap_pressure_matrix_index);
    auto Kgt = local_K.template block<nonwet_pressure_size, temperature_size>(
        nonwet_pressure_matrix_index, temperature_matrix_index);

    auto Klp = local_K.template block<cap_pressure_size, nonwet_pressure_size>(
        cap_pressure_matrix_index, nonwet_pressure_matrix_index);
    auto Klpc = local_K.template block<cap_pressure_size, cap_pressure_size>(
        cap_pressure_matrix_index, cap_pressure_matrix_index);
    auto Klt = local_K.template block<cap_pressure_size, temperature_size>(
        cap_pressure_matrix_index, temperature_matrix_index);

    auto Kep = local_K.template block<temperature_size, nonwet_pressure_size>(
        temperature_matrix_index, nonwet_pressure_matrix_index);
    auto Kepc = local_K.template block<temperature_size, cap_pressure_size>(
        temperature_matrix_index, cap_pressure_matrix_index);
    auto Ket = local_K.template block<temperature_size, temperature_size>(
        temperature_matrix_index, temperature_matrix_index);

    auto Bg = local_b.template segment<nonwet_pressure_size>(
        nonwet_pressure_matrix_index);
    auto Bl =
        local_b.template segment<cap_pressure_size>(cap_pressure_matrix_index);
    auto Be =
        local_b.template segment<temperature_size>(temperature_matrix_index);

    unsigned const n_integration_points =
        _integration_method.getNumberOfPoints();

    ParameterLib::SpatialPosition pos;
    pos.setElementID(_element.getID());

    auto const num_nodes = ShapeFunction::NPOINTS;
    auto const pg_nodal_values =
        Eigen::Map<const NodalVectorType>(&local_x[0], num_nodes);
    auto const pc_nodal_values =
        Eigen::Map<const NodalVectorType>(&local_x[num_nodes], num_nodes);

    MaterialPropertyLib::VariableArray vars;
    for (unsigned ip = 0; ip < n_integration_points; ip++)
    {
        auto const& ip_data = _ip_data[ip];
        auto const& N = ip_data.N;
        auto const& dNdx = ip_data.dNdx;
        auto const& w = ip_data.integration_weight;
        auto const& mass_operator = ip_data.mass_operator;
        auto const& diffusion_operator = ip_data.diffusion_operator;
        double pg_int_pt = 0.;
        double pc_int_pt = 0.;
        double T_int_pt = 0.0;
        NumLib::shapeFunctionInterpolate(local_x, N, pg_int_pt, pc_int_pt,
                                         T_int_pt);
        double const ideal_gas_constant_times_T_int_pt =
            IdealGasConstant * T_int_pt;
        vars[static_cast<int>(MaterialPropertyLib::Variable::temperature)] =
            T_int_pt;
        vars[static_cast<int>(
            MaterialPropertyLib::Variable::capillary_pressure)] = pc_int_pt;
        vars[static_cast<int>(MaterialPropertyLib::Variable::phase_pressure)] =
            pg_int_pt;

        auto const& medium =
            *_process_data.media_map->getMedium(this->_element.getID());
        auto const& liquid_phase = medium.phase("AqueousLiquid");
        auto const& solid_phase = medium.phase("Solid");
        auto const& gas_phase = medium.phase("Gas");

        auto const& vapour_component = gas_phase.component("w");
        auto const& dry_air_component = gas_phase.component("a");

        auto const water_mol_mass =
            vapour_component
                .property(MaterialPropertyLib::PropertyType::molar_mass)
                .template value<double>(vars, pos, t, dt);
        auto const air_mol_mass =
            dry_air_component
                .property(MaterialPropertyLib::PropertyType::molar_mass)
                .template value<double>(vars, pos, t, dt);

        auto const density_water =
            liquid_phase.property(MaterialPropertyLib::PropertyType::density)
                .template value<double>(vars, pos, t, dt);
        double const mol_density_water = density_water / water_mol_mass;

        double const Sw =
            medium.property(MaterialPropertyLib::PropertyType::saturation)
                .template value<double>(vars, pos, t, dt);

        _saturation[ip] = Sw;
        vars[static_cast<int>(
            MaterialPropertyLib::Variable::liquid_saturation)] = Sw;

        double dSwdpc =
            medium.property(MaterialPropertyLib::PropertyType::saturation)
                .template dValue<double>(
                    vars, MaterialPropertyLib::Variable::capillary_pressure,
                    pos, t, dt);

        // specific latent heat of evaporation
        double const latent_heat_evaporation =
            vapour_component
                .property(
                    MaterialPropertyLib::PropertyType::specific_latent_heat)
                .template value<double>(vars, pos, t, dt);

        vars[static_cast<int>(
            MaterialPropertyLib::Variable::enthalpy_of_evaporation)] =
            latent_heat_evaporation;

        // saturated vapour pressure
        double const p_sat =
            vapour_component
                .property(MaterialPropertyLib::PropertyType::vapour_pressure)
                .template value<double>(vars, pos, t, dt);
        double const dp_sat_dT =
            vapour_component
                .property(MaterialPropertyLib::PropertyType::vapour_pressure)
                .template dValue<double>(
                    vars, MaterialPropertyLib::Variable::temperature, pos, t,
                    dt);

        // Kelvin-Laplace correction for menisci
        double const K = std::exp(-pc_int_pt / mol_density_water /
                                  ideal_gas_constant_times_T_int_pt);
        double const dK_dT = pc_int_pt / mol_density_water /
                             ideal_gas_constant_times_T_int_pt / T_int_pt * K;

        // vapour pressure inside pore space (water partial pressure in gas
        // phase)
        double const p_vapour_nonwet = p_sat * K;
        double const d_p_vapour_nonwet_d_T = dp_sat_dT * K + p_sat * dK_dT;
        double const d_p_vapour_nonwet_d_pc =
            p_vapour_nonwet *
            (-1 / mol_density_water / ideal_gas_constant_times_T_int_pt);
        // partial pressure of gas component
        double const p_gas_nonwet = pg_int_pt - p_vapour_nonwet;
        // molar fraction of gas component in nonwet phase
        double const x_gas_nonwet = p_gas_nonwet / pg_int_pt;
        // molar fraction of water vapour in nonwet phase
        double const x_vapour_nonwet = p_vapour_nonwet / pg_int_pt;
        // mass fraction of gas component in the nonwet phase
        double const X_gas_nonwet =
            x_gas_nonwet /
            (x_gas_nonwet + x_vapour_nonwet * water_mol_mass / air_mol_mass);
        double const mol_density_nonwet =
            pg_int_pt / ideal_gas_constant_times_T_int_pt;

        double const d_mol_density_nonwet_d_pg =
            1 / ideal_gas_constant_times_T_int_pt;
        double const d_mol_density_nonwet_d_T =
            -pg_int_pt / ideal_gas_constant_times_T_int_pt / T_int_pt;
        double const d_x_gas_nonwet_d_pg =
            p_vapour_nonwet / pg_int_pt / pg_int_pt;
        double const d_x_gas_nonwet_d_pc = -d_p_vapour_nonwet_d_pc / pg_int_pt;
        double const d_x_gas_nonwet_d_T = -d_p_vapour_nonwet_d_T / pg_int_pt;

        double const density_nonwet_gas =
            p_gas_nonwet * air_mol_mass / ideal_gas_constant_times_T_int_pt;
        double const density_nonwet_vapour = p_vapour_nonwet * water_mol_mass /
                                             ideal_gas_constant_times_T_int_pt;
        double const density_nonwet =
            density_nonwet_gas + density_nonwet_vapour;
        double const density_wet = density_water;
        auto const density_solid =
            solid_phase.property(MaterialPropertyLib::PropertyType::density)
                .template value<double>(vars, pos, t, dt);
        // Derivative of nonwet phase density in terms of T
        double const d_density_nonwet_d_T =
            -((p_gas_nonwet * air_mol_mass + p_vapour_nonwet * water_mol_mass) /
              ideal_gas_constant_times_T_int_pt / T_int_pt) +
            (water_mol_mass - air_mol_mass) * d_p_vapour_nonwet_d_T /
                ideal_gas_constant_times_T_int_pt;

        _pressure_wetting[ip] = pg_int_pt - pc_int_pt;
        // heat capacity of nonwet phase
        double const heat_capacity_dry_gas =
            dry_air_component
                .property(
                    MaterialPropertyLib::PropertyType::specific_heat_capacity)
                .template value<double>(vars, pos, t, dt);
        const double heat_capacity_water_vapour =
            vapour_component
                .property(
                    MaterialPropertyLib::PropertyType::specific_heat_capacity)
                .template value<double>(vars, pos, t, dt);

        auto const heat_capacity_water =
            liquid_phase
                .property(
                    MaterialPropertyLib::PropertyType::specific_heat_capacity)
                .template value<double>(vars, pos, t, dt);
        auto const heat_capacity_solid =
            solid_phase
                .property(
                    MaterialPropertyLib::PropertyType::specific_heat_capacity)
                .template value<double>(vars, pos, t, dt);

        double const enthalpy_nonwet_gas =
            heat_capacity_dry_gas * (T_int_pt - CelsiusZeroInKelvin) +
            IdealGasConstant * (T_int_pt - CelsiusZeroInKelvin) / air_mol_mass;

        double const enthalpy_wet =
            heat_capacity_water * (T_int_pt - CelsiusZeroInKelvin);

        double const enthalpy_nonwet_vapour =
            heat_capacity_water_vapour * (T_int_pt - CelsiusZeroInKelvin) +
            latent_heat_evaporation;
        double const enthalpy_nonwet =
            enthalpy_nonwet_gas * X_gas_nonwet +
            enthalpy_nonwet_vapour * (1 - X_gas_nonwet);
        double const internal_energy_nonwet =
            enthalpy_nonwet - pg_int_pt / density_nonwet;
        double const internal_energy_wet = enthalpy_wet;
        /// Derivative
        double const d_enthalpy_gas_nonwet_d_T =
            heat_capacity_dry_gas + IdealGasConstant / air_mol_mass;
        double const d_enthalpy_nonwet_d_T =
            heat_capacity_water * (1 - X_gas_nonwet) +
            d_enthalpy_gas_nonwet_d_T * X_gas_nonwet;
        // Assemble M matrix
        // nonwetting
        auto const porosity =
            medium.property(MaterialPropertyLib::PropertyType::porosity)
                .template value<double>(vars, pos, t, dt);

        Mgp.noalias() +=
            porosity *
            ((1 - Sw) * (mol_density_nonwet * d_x_gas_nonwet_d_pg +
                         x_gas_nonwet * d_mol_density_nonwet_d_pg)) *
            mass_operator;
        Mgpc.noalias() += porosity *
                          ((1 - Sw) * mol_density_nonwet * d_x_gas_nonwet_d_pc -
                           mol_density_nonwet * x_gas_nonwet * dSwdpc) *
                          mass_operator;
        Mgt.noalias() +=
            porosity *
            ((1 - Sw) * (mol_density_nonwet * d_x_gas_nonwet_d_T +
                         x_gas_nonwet * d_mol_density_nonwet_d_T)) *
            mass_operator;

        Mlpc.noalias() += porosity *
                          ((1 - Sw) * d_p_vapour_nonwet_d_pc /
                               ideal_gas_constant_times_T_int_pt +
                           mol_density_nonwet * x_vapour_nonwet * (-dSwdpc) +
                           dSwdpc * mol_density_water) *
                          mass_operator;
        Mlt.noalias() +=
            porosity *
            ((1 - Sw) / ideal_gas_constant_times_T_int_pt *
             (d_p_vapour_nonwet_d_T - p_vapour_nonwet / T_int_pt)) *
            mass_operator;

        Mep.noalias() +=
            porosity *
            ((x_gas_nonwet * air_mol_mass + x_vapour_nonwet * water_mol_mass) *
                 d_mol_density_nonwet_d_pg * enthalpy_nonwet -
             mol_density_nonwet * (water_mol_mass - air_mol_mass) *
                 d_x_gas_nonwet_d_pg * enthalpy_nonwet -
             1) *
            (1 - Sw) * mass_operator;
        Mepc.noalias() +=
            porosity *
                (density_wet * internal_energy_wet -
                 density_nonwet * internal_energy_nonwet) *
                dSwdpc * mass_operator +
            porosity *
                ((water_mol_mass - air_mol_mass) * enthalpy_nonwet /
                 ideal_gas_constant_times_T_int_pt) *
                (1 - Sw) * d_p_vapour_nonwet_d_pc * mass_operator;
        Met.noalias() +=
            ((1 - porosity) * density_solid * heat_capacity_solid +
             porosity * ((1 - Sw) * (d_density_nonwet_d_T * enthalpy_nonwet +
                                     density_nonwet * d_enthalpy_nonwet_d_T) +
                         Sw * density_wet * heat_capacity_water)) *
            mass_operator;

        // nonwet
        double const k_rel_nonwet =
            medium
                .property(MaterialPropertyLib::PropertyType::
                              relative_permeability_nonwetting_phase)
                .template value<double>(vars, pos, t, dt);
        auto const mu_nonwet =
            gas_phase.property(MaterialPropertyLib::PropertyType::viscosity)
                .template value<double>(vars, pos, t, dt);
        double const lambda_nonwet = k_rel_nonwet / mu_nonwet;
        double const diffusion_coeff_component_gas =
            vapour_component
                .property(MaterialPropertyLib::PropertyType::diffusion)
                .template value<double>(vars, pos, t, dt);

        // wet
        double const k_rel_wet =
            medium
                .property(
                    MaterialPropertyLib::PropertyType::relative_permeability)
                .template value<double>(vars, pos, t, dt);
        auto const mu_wet =
            liquid_phase.property(MaterialPropertyLib::PropertyType::viscosity)
                .template value<double>(vars, pos, t, dt);
        double const lambda_wet = k_rel_wet / mu_wet;

        auto const permeability =
            MaterialPropertyLib::formEigenTensor<GlobalDim>(
                medium.property(MaterialPropertyLib::PropertyType::permeability)
                    .value(vars, pos, t, dt));

        GlobalDimVectorType const velocity_nonwet =
            -lambda_nonwet * permeability * (dNdx * pg_nodal_values);
        GlobalDimVectorType const velocity_wet =
            -lambda_wet * permeability *
            (dNdx * (pg_nodal_values - pc_nodal_values));

        laplace_operator.noalias() = dNdx.transpose() * permeability * dNdx * w;

        Ket.noalias() += w * N.transpose() *
                             (d_density_nonwet_d_T * enthalpy_nonwet +
                              density_nonwet * d_enthalpy_nonwet_d_T) *
                             velocity_nonwet.transpose() * dNdx +
                         w * N.transpose() * heat_capacity_water *
                             density_water * velocity_wet.transpose() * dNdx;

        // Laplace
        Kgp.noalias() += (mol_density_nonwet * x_gas_nonwet * lambda_nonwet) *
                             laplace_operator +
                         ((1 - Sw) * porosity * diffusion_coeff_component_gas *
                          mol_density_nonwet * d_x_gas_nonwet_d_pg) *
                             diffusion_operator;
        Kgpc.noalias() += ((1 - Sw) * porosity * diffusion_coeff_component_gas *
                           mol_density_nonwet * d_x_gas_nonwet_d_pc) *
                          diffusion_operator;
        Kgt.noalias() += ((1 - Sw) * porosity * diffusion_coeff_component_gas *
                          mol_density_nonwet * d_x_gas_nonwet_d_T) *
                         diffusion_operator;

        Klp.noalias() +=
            (mol_density_nonwet * x_vapour_nonwet * lambda_nonwet) *
                laplace_operator +
            mol_density_water * lambda_wet * laplace_operator -
            ((1 - Sw) * porosity * diffusion_coeff_component_gas *
             mol_density_nonwet * d_x_gas_nonwet_d_pg) *
                diffusion_operator;
        Klpc.noalias() += (-mol_density_water * lambda_wet * laplace_operator) -
                          ((1 - Sw) * porosity * diffusion_coeff_component_gas *
                           mol_density_nonwet * d_x_gas_nonwet_d_pc) *
                              diffusion_operator;
        Klt.noalias() += -((1 - Sw) * porosity * diffusion_coeff_component_gas *
                           mol_density_nonwet * d_x_gas_nonwet_d_T) *
                         diffusion_operator;

        Kep.noalias() += (lambda_nonwet * density_nonwet * enthalpy_nonwet +
                          lambda_wet * density_wet * enthalpy_wet) *
                             laplace_operator +
                         (1 - Sw) * porosity * diffusion_coeff_component_gas *
                             mol_density_nonwet *
                             (air_mol_mass * enthalpy_nonwet_gas -
                              water_mol_mass * enthalpy_nonwet_vapour) *
                             d_x_gas_nonwet_d_pg * diffusion_operator;
        Kepc.noalias() +=
            -lambda_wet * enthalpy_wet * density_wet * laplace_operator +
            (1 - Sw) * porosity * diffusion_coeff_component_gas *
                mol_density_nonwet *
                (air_mol_mass * enthalpy_nonwet_gas -
                 water_mol_mass * enthalpy_nonwet_vapour) *
                d_x_gas_nonwet_d_pc * diffusion_operator;

        if (medium.hasProperty(
                MaterialPropertyLib::PropertyType::thermal_conductivity))
        {
            auto const lambda =
                medium
                    .property(
                        MaterialPropertyLib::PropertyType::thermal_conductivity)
                    .value(vars, pos, t, dt);

            GlobalDimMatrixType const heat_conductivity_unsaturated =
                MaterialPropertyLib::formEigenTensor<GlobalDim>(lambda);

            Ket.noalias() +=
                dNdx.transpose() * heat_conductivity_unsaturated * dNdx * w +
                (1 - Sw) * porosity * diffusion_coeff_component_gas *
                    mol_density_nonwet *
                    (air_mol_mass * enthalpy_nonwet_gas -
                     water_mol_mass * enthalpy_nonwet_vapour) *
                    d_x_gas_nonwet_d_T * diffusion_operator;
        }
        else
        {
            auto const thermal_conductivity_solid =
                solid_phase
                    .property(
                        MaterialPropertyLib::PropertyType::thermal_conductivity)
                    .value(vars, pos, t, dt);

            auto const thermal_conductivity_fluid =
                liquid_phase
                    .property(
                        MaterialPropertyLib::PropertyType::thermal_conductivity)
                    .template value<double>(vars, pos, t, dt) *
                Sw;

            GlobalDimMatrixType const heat_conductivity_unsaturated =
                MaterialPropertyLib::formEffectiveThermalConductivity<
                    GlobalDim>(thermal_conductivity_solid,
                               thermal_conductivity_fluid, porosity);

            Ket.noalias() +=
                dNdx.transpose() * heat_conductivity_unsaturated * dNdx * w +
                (1 - Sw) * porosity * diffusion_coeff_component_gas *
                    mol_density_nonwet *
                    (air_mol_mass * enthalpy_nonwet_gas -
                     water_mol_mass * enthalpy_nonwet_vapour) *
                    d_x_gas_nonwet_d_T * diffusion_operator;
        }

        if (_process_data.has_gravity)
        {
            auto const& b = _process_data.specific_body_force;
            NodalVectorType gravity_operator =
                dNdx.transpose() * permeability * b * w;
            Bg.noalias() += (mol_density_nonwet * x_gas_nonwet * lambda_nonwet *
                             density_nonwet) *
                            gravity_operator;
            Bl.noalias() += (mol_density_water * lambda_wet * density_wet +
                             mol_density_nonwet * x_vapour_nonwet *
                                 lambda_nonwet * density_nonwet) *
                            gravity_operator;
            Be.noalias() +=
                (lambda_nonwet * density_nonwet * density_nonwet *
                     enthalpy_nonwet +
                 lambda_wet * density_wet * density_wet * enthalpy_wet) *
                gravity_operator;
        }  // end of has gravity
    }
    if (_process_data.has_mass_lumping)
    {
        for (unsigned row = 0; row < Mgp.cols(); row++)
        {
            for (unsigned column = 0; column < Mgp.cols(); column++)
            {
                if (row != column)
                {
                    Mgp(row, row) += Mgp(row, column);
                    Mgp(row, column) = 0.0;
                    Mgpc(row, row) += Mgpc(row, column);
                    Mgpc(row, column) = 0.0;
                    Mgt(row, row) += Mgt(row, column);
                    Mgt(row, column) = 0.0;
                    Mlp(row, row) += Mlp(row, column);
                    Mlp(row, column) = 0.0;
                    Mlpc(row, row) += Mlpc(row, column);
                    Mlpc(row, column) = 0.0;
                    Mlt(row, row) += Mlt(row, column);
                    Mlt(row, column) = 0.0;
                    Mep(row, row) += Mep(row, column);
                    Mep(row, column) = 0.0;
                    Mepc(row, row) += Mepc(row, column);
                    Mepc(row, column) = 0.0;
                    Met(row, row) += Met(row, column);
                    Met(row, column) = 0.0;
                }
            }
        }
    }  // end of mass-lumping
}

}  // namespace ThermalTwoPhaseFlowWithPP
}  // namespace ProcessLib
