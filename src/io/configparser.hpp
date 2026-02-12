#pragma once

#include <istream>
#include <string>

#include "core/types.hpp"

namespace axonhh::io {

struct RunConfig {
    SimConfig sim;
    Params params;
};

RunConfig default_run_config();

RunConfig parse_config_stream(std::istream& in, const RunConfig& base = default_run_config());
RunConfig parse_config_file(const std::string& path, const RunConfig& base = default_run_config());

} // namespace axonhh::io
