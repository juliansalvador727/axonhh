#include "csvwriter.hpp"

#include <iomanip>
#include <stdexcept>

namespace axonhh::io {

CsvWriter::CsvWriter(const std::string& path) : out_(path)
{
    if (!out_) {
        throw std::runtime_error("failed to open csv output: " + path);
    }
    out_ << std::setprecision(10);
}

void CsvWriter::write_header()
{
    out_ << "t_ms,V_mV,m,h,n,Iinj_uAcm2,INa_uAcm2,IK_uAcm2,IL_uAcm2\n";
}

void CsvWriter::write_row(double t_ms, const State& x, const Currents& I)
{
    out_ << t_ms << ','
         << x.V_mV << ','
         << x.m << ','
         << x.h << ','
         << x.n << ','
         << I.Iinj_uA_cm2 << ','
         << I.INa_uA_cm2 << ','
         << I.IK_uA_cm2 << ','
         << I.IL_uA_cm2 << '\n';
}

} // namespace axonhh::io
