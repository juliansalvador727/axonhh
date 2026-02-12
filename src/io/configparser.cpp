#include "configparser.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <stdexcept>
#include <string>

namespace axonhh::io {
namespace {

std::string trim(const std::string& s)
{
    std::size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start])) != 0) {
        ++start;
    }

    std::size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1])) != 0) {
        --end;
    }

    return s.substr(start, end - start);
}

std::string to_lower_copy(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return s;
}

double parse_double(const std::string& key, const std::string& value)
{
    std::size_t idx = 0;
    const double x = std::stod(value, &idx);
    if (idx != value.size()) {
        throw std::invalid_argument("invalid numeric value for key '" + key + "': '" + value + "'");
    }
    return x;
}

IntegratorKind parse_integrator_kind(const std::string& value)
{
    const std::string v = to_lower_copy(value);
    if (v == "euler") {
        return IntegratorKind::Euler;
    }
    if (v == "rk4") {
        return IntegratorKind::RK4;
    }
    throw std::invalid_argument("invalid integrator kind: '" + value + "'");
}

StimulusKind parse_stimulus_kind(const std::string& value)
{
    const std::string v = to_lower_copy(value);
    if (v == "step") {
        return StimulusKind::Step;
    }
    if (v == "pulse") {
        return StimulusKind::Pulse;
    }
    throw std::invalid_argument("invalid stimulus kind: '" + value + "'");
}

void assign_key_value(RunConfig& cfg, const std::string& key, const std::string& value)
{
    if (key == "dt_ms") {
        cfg.sim.dt_ms = parse_double(key, value);
    } else if (key == "T_ms") {
        cfg.sim.T_ms = parse_double(key, value);
    } else if (key == "V0_mV") {
        cfg.sim.V0_mV = parse_double(key, value);
    } else if (key == "integrator") {
        cfg.sim.integrator = parse_integrator_kind(value);
    } else if (key == "output.csv_path") {
        cfg.sim.output.csv_path = value;
    } else if (key == "stimulus.kind") {
        cfg.sim.stimulus.kind = parse_stimulus_kind(value);
    } else if (key == "stimulus.amp_uA_cm2") {
        cfg.sim.stimulus.amp_uA_cm2 = parse_double(key, value);
    } else if (key == "stimulus.t0_ms") {
        cfg.sim.stimulus.t0_ms = parse_double(key, value);
    } else if (key == "stimulus.t1_ms") {
        cfg.sim.stimulus.t1_ms = parse_double(key, value);
    } else if (key == "stimulus.period_ms") {
        cfg.sim.stimulus.period_ms = parse_double(key, value);
    } else if (key == "stimulus.duty") {
        cfg.sim.stimulus.duty = parse_double(key, value);
    } else if (key == "params.C_m_uF_cm2") {
        cfg.params.C_m_uF_cm2 = parse_double(key, value);
    } else if (key == "params.gNa_bar_mS_cm2") {
        cfg.params.gNa_bar_mS_cm2 = parse_double(key, value);
    } else if (key == "params.gK_bar_mS_cm2") {
        cfg.params.gK_bar_mS_cm2 = parse_double(key, value);
    } else if (key == "params.gL_bar_mS_cm2") {
        cfg.params.gL_bar_mS_cm2 = parse_double(key, value);
    } else if (key == "params.ENa_mV") {
        cfg.params.ENa_mV = parse_double(key, value);
    } else if (key == "params.EK_mV") {
        cfg.params.EK_mV = parse_double(key, value);
    } else if (key == "params.EL_mV") {
        cfg.params.EL_mV = parse_double(key, value);
    } else {
        throw std::invalid_argument("unknown config key: '" + key + "'");
    }
}

} // namespace

RunConfig default_run_config()
{
    return {default_config(), default_params()};
}

RunConfig parse_config_stream(std::istream& in, const RunConfig& base)
{
    RunConfig cfg = base;

    std::string line;
    std::size_t line_no = 0;
    while (std::getline(in, line)) {
        ++line_no;

        const std::size_t comment_pos = line.find('#');
        if (comment_pos != std::string::npos) {
            line = line.substr(0, comment_pos);
        }

        const std::string trimmed = trim(line);
        if (trimmed.empty()) {
            continue;
        }

        const std::size_t eq = trimmed.find('=');
        if (eq == std::string::npos) {
            throw std::invalid_argument(
                "config parse error on line " + std::to_string(line_no) + ": missing '='");
        }

        const std::string key = trim(trimmed.substr(0, eq));
        const std::string value = trim(trimmed.substr(eq + 1));
        if (key.empty()) {
            throw std::invalid_argument(
                "config parse error on line " + std::to_string(line_no) + ": empty key");
        }
        if (value.empty()) {
            throw std::invalid_argument(
                "config parse error on line " + std::to_string(line_no) + ": empty value");
        }

        try {
            assign_key_value(cfg, key, value);
        } catch (const std::invalid_argument& e) {
            throw std::invalid_argument(
                "config parse error on line " + std::to_string(line_no) + ": " + e.what());
        }
    }

    return cfg;
}

RunConfig parse_config_file(const std::string& path, const RunConfig& base)
{
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("failed to open config file: " + path);
    }

    return parse_config_stream(in, base);
}

} // namespace axonhh::io
