## (`test_integrators.cpp` + `test_rates.cpp`)

This document describes the validation strategy for the numerical core of the Hodgkin–Huxley implementation.

It covers:

- Numerical time integration layer (`euler.cpp`, `rk4.cpp`, `integrator.hpp`)
- Voltage-dependent rate functions (`rates.cpp`)

All tests are:

- Dependency-free
- Standard library only
- Standalone executables
- Purely numerical (no plotting)
- Fail-fast

Units:

- Voltage: millivolts (mV)
- Time: milliseconds (ms)
- Rates: 1/ms

## Part I — Integrator Validation (`test_integrators.cpp`)

### Scope

The following components are tested:

- Integrator interface contract (`Integrator::step`)
- Euler single-step update (`EulerIntegrator`)
- RK4 single-step update (`RK4Integrator`)
- Integrator factory dispatch (`make_integrator`)

### Test 1 — Zero-RHS Invariance

**Purpose**  
Verify that both integrators preserve state when the ODE derivative is identically zero.

**Method**

1. Define a fixed initial state.
2. Use an RHS function that returns zero for all derivatives.
3. Advance one timestep with Euler and RK4.
4. Assert output state equals input state within tight tolerance.

**Failure Modes Caught**

- Incorrect timestep scaling
- Incorrect state accumulation
- RK4 stage-combination errors

### Test 2 — Constant-RHS Exactness (Single Step)

**Purpose**  
Confirm both integrators are exact for a constant derivative over one step.

**Method**

1. Define a constant derivative vector `c`.
2. Compute analytical one-step result:
   - `x(t + dt) = x(t) + dt * c`
3. Advance one timestep using Euler and RK4.
4. Assert both match analytical result within floating-point tolerance.

**What This Validates**

- Correct Euler implementation
- Correct RK4 reduction for constant derivatives
- Correct use of `dt` across all state components

### Test 3 — Accuracy Ordering on Exponential Growth

**Purpose**  
Validate that RK4 provides higher accuracy than Euler for a nonlinear-in-time trajectory.

**Method**

1. Use scalar dynamics embedded in all state components: `y' = y`.
2. Integrate one coarse step (`dt = 1`).
3. Compare each method against exact solution `e^dt`.
4. Assert RK4 absolute error is smaller than Euler absolute error.

**Why This Matters**
This catches subtle RK4 implementation mistakes that can still pass constant-derivative checks.

### Test 4 — Hodgkin–Huxley RHS Finite Single Step

**Purpose**  
Ensure both integrators produce finite states when stepping the actual HH model RHS.

**Method**

1. Build HH model with default parameters.
2. Initialize at steady state near rest (`V0 = -65 mV`).
3. Apply one integration step with nonzero injected current.
4. Assert all resulting state components are finite.

**Failure Modes Caught**

- NaN/Inf propagation from RHS coupling
- Numerically unstable stage evaluation in RK4

### Test 5 — Factory Dispatch and Error Handling

**Purpose**  
Verify the integrator factory returns valid concrete types and rejects invalid enum values.

**Method**

1. Request Euler and RK4 via `make_integrator`.
2. Assert returned pointers are non-null.
3. Request an invalid enum value.
4. Assert `std::invalid_argument` is thrown.

**What This Validates**

- Enum-to-implementation wiring
- Defensive behavior for invalid configuration

### Integrator Test Philosophy

- No plotting: tests are purely numerical.
- No external libraries: standard library only.
- Fail fast: emit clear failure message and exit immediately.
- Property-oriented checks: prefer mathematical invariants over implementation details.

Once all tests pass, the integrator layer is considered functionally correct for baseline HH simulation usage.

## Part II — Rate Function Validation (`test_rates.cpp`)

### Scope

The following six rate functions are tested:

**Sodium activation**

- `alpha_m(V)`, `beta_m(V)`

**Sodium inactivation**

- `alpha_h(V)`, `beta_h(V)`

**Potassium activation**

- `alpha_n(V)`, `beta_n(V)`

All voltages are expressed in millivolts (mV) and rates in 1/ms.

### Test 6 — Finiteness Over Voltage Sweep

**Purpose**  
Ensure all rate functions are numerically well-behaved over a reasonable voltage range.

**Method**

1. Sweep membrane voltage over:
   - `V in [-100 mV, +60 mV]`
2. For each voltage, evaluate all six rate functions.
3. Assert that each returned value:
   - is finite
   - is not NaN
   - is non-negative

**Failure Modes Caught**

- Division by zero
- Exponential overflow / underflow
- Broken singularity handling (`vtrap`)
- Sign errors in formulas

### Test 7 — Correct Singularity Limits

**Purpose**  
Verify correct handling of removable singularities in the Hodgkin–Huxley rate equations.

**Known Mathematical Limits**
The rate functions contain removable singularities where the denominator approaches zero. The implementation must handle these limits explicitly.

- `alpha_m(V)` has a removable singularity at `V = -40 mV`
  - Expected value: `1.0`

- `alpha_n(V)` has a removable singularity at `V = -55 mV`
  - Expected value: `0.1`

**Method**

1. Evaluate `alpha_m(-40)` and `alpha_n(-55)`.
2. Assert values match expected limits within a tolerance of `1e-6`.

**What This Validates**

- Correct `vtrap` implementation
- Correct voltage shifts
- Correct constants and scaling

### Test 8 — Sanity Check at Resting Potential

**Purpose**  
Ensure rate magnitudes at the resting membrane potential are physically reasonable.

**Conditions**

- Voltage fixed at `V = -65 mV` (typical resting potential).

**Expected Qualitative Behavior**
At rest:

- `alpha_m` is small
- `beta_m` is large
- `alpha_h` is moderate
- `beta_h` is moderate
- `alpha_n` is small
- `beta_n` is moderate

Note: exact numerical values are not asserted, only relative magnitudes and plausibility.

**Failure Modes Caught**

- Incorrect signs
- Incorrect voltage offsets
- Incorrect constants
- Swapped or mis-typed rate equations

### Test 9 — Monotonicity Properties

**Purpose**  
Validate that rate functions change with voltage in the correct qualitative direction.

**Expected Monotonic Trends**

- `alpha_m(V)` increases as `V` increases
- `beta_m(V)` decreases as `V` increases
- `alpha_n(V)` increases as `V` increases

**Method**

1. Choose two voltages `V1 < V2`.
2. Assert the expected ordering holds.

**Why This Matters**
Monotonicity errors often indicate:

- Sign mistakes inside exponentials
- Incorrect voltage shifts
- Incorrect numerator/denominator structure

These errors may not cause crashes but will produce incorrect dynamics.

### Rate Test Philosophy

- No plotting: tests are purely numerical.
- No logging: output is silent unless a failure occurs.
- No external libraries: depends only on the standard math library.
- Fail fast: abort or print clear error messages immediately upon failure.
- Validate mathematics: focus on mathematical properties, not implementation details.

Once all tests pass, the rate layer is considered numerically safe and ready for use in the full Hodgkin–Huxley model.

## Final Acceptance Criteria

When all tests pass:

- The integrator layer is mathematically correct for baseline stepping.
- The rate layer is numerically safe across physiological voltages.
- A single HH step produces finite state values (no NaN/Inf).
- Factory dispatch is correct and rejects invalid configuration.
