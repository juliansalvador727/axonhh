#include <algorithm>
#include <cmath>
#include <exception>
#include <iostream>
#include <stdexcept>

#include "io/configparser.hpp"
#include "io/csvwriter.hpp"
#include "model/hodgkinhuxley.hpp"
#include "numerics/integrator.hpp"
#include "stim/stimulus.hpp"

namespace {

void validate_config(const axonhh::io::RunConfig& cfg)
{
    if (cfg.sim.dt_ms <= 0.0) {
        throw std::invalid_argument("dt_ms must be > 0");
    }
    if (cfg.sim.T_ms < 0.0) {
        throw std::invalid_argument("T_ms must be >= 0");
    }
    if (cfg.sim.stimulus.t1_ms < cfg.sim.stimulus.t0_ms) {
        throw std::invalid_argument("stimulus.t1_ms must be >= stimulus.t0_ms");
    }
}

} // namespace

int main(int argc, char** argv)
{
    try {
        if (argc > 2) {
            std::cerr << "usage: axonhh [config_path]\n";
            return 1;
        }

        axonhh::io::RunConfig run_cfg = axonhh::io::default_run_config();
        if (argc == 2) {
            run_cfg = axonhh::io::parse_config_file(argv[1], run_cfg);
        }
        validate_config(run_cfg);

        const axonhh::HodgkinHuxley hh(run_cfg.params);
        auto integrator = axonhh::numerics::make_integrator(run_cfg.sim.integrator);
        axonhh::io::CsvWriter writer(run_cfg.sim.output.csv_path);
        writer.write_header();

        axonhh::State x = hh.steady_state(run_cfg.sim.V0_mV);

        const double dt_ms = run_cfg.sim.dt_ms;
        const double T_ms = run_cfg.sim.T_ms;
        const std::size_t n_steps = static_cast<std::size_t>(std::ceil(T_ms / dt_ms));

        const auto rhs = [&hh, &run_cfg](double t_ms, const axonhh::State& state) {
            const double iinj = axonhh::stim::current_from_config(t_ms, run_cfg.sim.stimulus);
            return hh.rhs(t_ms, state, iinj);
        };

        for (std::size_t i = 0; i <= n_steps; ++i) {
            const double t_ms = std::min(T_ms, static_cast<double>(i) * dt_ms);
            const double iinj = axonhh::stim::current_from_config(t_ms, run_cfg.sim.stimulus);
            const axonhh::Currents currents = hh.currents(t_ms, x, iinj);

            writer.write_row(t_ms, x, currents);

            if (i == n_steps) {
                break;
            }
            x = integrator->step(rhs, t_ms, x, dt_ms);
        }

        std::cout << "wrote csv: " << run_cfg.sim.output.csv_path << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
