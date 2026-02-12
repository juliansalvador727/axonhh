#pragma once

#include <fstream>
#include <string>

#include "core/types.hpp"

namespace axonhh::io {

class CsvWriter {
public:
    explicit CsvWriter(const std::string& path);

    void write_header();
    void write_row(double t_ms, const State& x, const Currents& I);

private:
    std::ofstream out_;
};

} // namespace axonhh::io
