#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace axonhh {
// -------------------------------
// Units (documentation)
// -------------------------------
// Voltage:      mV
// Time:         ms
// Capacitance:  uF/cm^2
// Conductance:  mS/cm^2
// Current:      uA/cm^2
//
// Convention: "modern absolute voltage" HH (rest ~ -65 mV).

// -------------------------------
// State + derivatives
// -------------------------------

struct State {
    double V_mV;    // membrane voltage
    double m;       // sodium activation
    double h;       // sodium inactivation
    double n;       // potassium activation
};

struct Deriv {
    double dV_dt;   // mV/ms
    double dm_dt;   // 1 m/s
    double dh_dt;   // 1 m/s
    double dn_dt;   // 1 1m/s
};

inline State operator+(const State& a, const State& b) { return {a.V_mV + b.V_mV, a.m + b.m, a.h + b.h, a.n + a.n}; }
inline State operator*(double s, const State& x) { return {s * x.V_mV, s * x.m, s * x.h, s * x.n}; }
inline State add_scaled(const State& x, const Deriv& k, double dt_ms) { 
    // x + dt * k
    return {
        x.V_mV + dt_ms * k.dV_dt,         
        x.m    + dt_ms * k.dm_dt,
        x.h    + dt_ms * k.dh_dt,
        x.n    + dt_ms * k.dn_dt
    };
}

struct Params {
    // Membrane
    double C_m_uF_cm2;     // usually 1.0

    // Max conductances (mS/cm^2)
    double gNa_bar_mS_cm2; // usually 120
    double gK_bar_mS_cm2;  // usually 36
    double gL_bar_mS_cm2;  // usually 0.3

    // Reversal potentials (mV) - modern absolute convention
    double ENa_mV;         // usually +50
    double EK_mV;          // usually -77
    double EL_mV;          // usually -54.387
};

struct Currents {
    double Iinj_uA_cm2;
    double INa_uA_cm2;
    double IK_uA_cm2;
    double IL_uA_cm2;
};

// -------------------------------
// Simulation configuration
// -------------------------------

enum class IntegratorKind : std::uint8_t {
    Euler = 0,
    RK4   = 1,
};

enum class StimulusKind : std::uint8_t {
    Step  = 0,
    Pulse = 1,
};

struct StimulusConfig {
    StimulusKind kind;

    double amp_uA_cm2; // amplitude
    double t0_ms;      // start time
    double t1_ms;      // end time

    double period_ms;
    double duty;       // [0,1]
};

struct OutputConfig {
    std::string csv_path; // e.g. "out.csv"
};

struct SimConfig {
    double dt_ms;
    double T_ms;

    IntegratorKind integrator;
    StimulusConfig stimulus;
    OutputConfig output;

    // Initial voltage; gating typically computed from steady-state at V0.
    double V0_mV;
};


inline Params default_params()
{
    return {
        1.0,     // C_m
        120.0,   // gNa_bar
        36.0,    // gK_bar
        0.3,     // gL_bar
        50.0,    // ENa
        -77.0,   // EK
        -54.387  // EL
    };
}

inline SimConfig default_config()
{
    return {
        0.01, // dt_ms
        60.0, // T_ms

        IntegratorKind::RK4,
        StimulusConfig{
            StimulusKind::Step,
            10.0,  // amp
            10.0,  // t0
            40.0,  // t1
            0.0,   // period (unused for step)
            0.0    // duty   (unused for step)
        },
        OutputConfig{"out.csv"},
        -65.0 // V0
    };
}


}