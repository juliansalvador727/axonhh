#include "hodgkinhuxley.hpp"
#include "rates.hpp"

namespace axonhh {

HodgkinHuxley::HodgkinHuxley(Params params) : p_(params) {}

const Params& HodgkinHuxley::params() const { return p_; }

Currents HodgkinHuxley::currents(double t_ms, const State& x, double Iinj_uA_cm2) const {
    (void)t_ms;

    const double gNa = p_.gNa_bar_mS_cm2 * (x.m * x.m * x.m) * x.h;
    const double gK = p_.gK_bar_mS_cm2 * (x.n * x.n * x.n * x.n);
    const double gL = p_.gL_bar_mS_cm2;

    const double INa = gNa * (x.V_mV - p_.ENa_mV);
    const double IK  = gK  * (x.V_mV - p_.EK_mV);
    const double IL  = gL  * (x.V_mV - p_.EL_mV);

    return Currents{
        Iinj_uA_cm2,
        INa,
        IK,
        IL
    };
}

Deriv HodgkinHuxley::rhs(double t_ms, const State& x, double Iinj_uA_cm2) const {
    const Currents I = currents(t_ms, x, Iinj_uA_cm2);

    const double dV_dt = (I.Iinj_uA_cm2 - (I.INa_uA_cm2 + I.IK_uA_cm2 + I.IL_uA_cm2)) / p_.C_m_uF_cm2;

    const double am = rates::alpha_m(x.V_mV);
    const double bm = rates::beta_m(x.V_mV);
    const double ah = rates::alpha_h(x.V_mV);
    const double bh = rates::beta_h(x.V_mV);
    const double an = rates::alpha_n(x.V_mV);
    const double bn = rates::beta_n(x.V_mV);

    const double dm_dt = am * (1.0 - x.m) - bm * x.m;
    const double dh_dt = ah * (1.0 - x.h) - bh * x.h;
    const double dn_dt = an * (1.0 - x.n) - bn * x.n;

    return Deriv{ dV_dt, dm_dt, dh_dt, dn_dt };
}

State HodgkinHuxley::steady_state(double V0_mV) const {
    const double am = rates::alpha_m(V0_mV);
    const double bm = rates::beta_m(V0_mV);
    const double ah = rates::alpha_h(V0_mV);
    const double bh = rates::beta_h(V0_mV);
    const double an = rates::alpha_n(V0_mV);
    const double bn = rates::beta_n(V0_mV);

    const double m0 = rates::x_inf(am, bm);
    const double h0 = rates::x_inf(ah, bh);
    const double n0 = rates::x_inf(an, bn);

    return State{ V0_mV, m0, h0, n0 };
}

}
