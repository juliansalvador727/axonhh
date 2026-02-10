# Tests for (`test_rates.cpp`)

This document describes the validation strategy for the Hodgkin–Huxley rate functions implemented in `rates.cpp`.

The goal of these tests is to ensure numerical correctness, stability, and physical plausibility of the voltage-dependent transition rates before they are used in the full neuron model.

All tests are dependency-free and run as part of the standalone `test_rates` executable.

## Scope

The following six rate functions are tested:

**Sodium activation:**

- $\alpha_m(V)$, $\beta_m(V)$

**Sodium inactivation:**

- $\alpha_h(V)$, $\beta_h(V)$

**Potassium activation:**

- $\alpha_n(V)$, $\beta_n(V)$

All voltages are expressed in millivolts (mV) and rates in $1/\text{ms}$.

## TEST 1 — Finiteness Over Voltage Sweep

### Purpose

Ensure all rate functions are numerically well-behaved over a physiologically reasonable voltage range.

### Method

1. Sweep membrane voltage over:
   $$V \in [-100\ \text{mV}, +60\ \text{mV}]$$
2. For each voltage, evaluate all six rate functions.
3. Assert that each returned value:
   - is **finite**
   - is **not NaN**
   - is **non-negative**

### Failure Modes Caught

- Division by zero
- Exponential overflow / underflow
- Broken singularity handling (`vtrap`)
- Sign errors in formulas

## TEST 2 — Correct Singularity Limits

### Purpose

Verify correct handling of removable singularities in the Hodgkin–Huxley rate equations.

### Known Mathematical Limits

The rate functions contain removable singularities where the denominator approaches zero. The implementation must handle these limits explicitly (often via L'Hôpital's rule).

- **$\alpha_m(V)$** has a removable singularity at $V = -40\ \text{mV}$.
  - **Expected value:** $1.0$

- **$\alpha_n(V)$** has a removable singularity at $V = -55\ \text{mV}$.
  - **Expected value:** $0.1$

### Method

1. Evaluate $\alpha_m(-40)$ and $\alpha_n(-55)$.
2. Assert values match expected limits within a tolerance of $10^{-6}$.

### What This Validates

- Correct `vtrap` implementation
- Correct voltage shifts
- Correct constants and scaling

## TEST 3 — Sanity Check at Resting Potential

### Purpose

Ensure rate magnitudes at the resting membrane potential are physically reasonable.

### Conditions

- Voltage fixed at $V = -65\ \text{mV}$ (typical resting potential).

### Expected Qualitative Behavior

At rest:

- $\alpha_m \to \text{small}$
- $\beta_m \to \text{large}$
- $\alpha_h \to \text{moderate}$
- $\beta_h \to \text{moderate}$
- $\alpha_n \to \text{small}$
- $\beta_n \to \text{moderate}$

_Note: Exact numerical values are not asserted. Only relative magnitudes and plausibility are checked._

### Failure Modes Caught

- Incorrect signs
- Incorrect voltage offsets
- Incorrect constants
- Swapped or mis-typed rate equations

## TEST 4 — Monotonicity Properties

### Purpose

Validate that rate functions change with voltage in the correct qualitative direction.

### Expected Monotonic Trends

- $\alpha_m(V)$ **increases** with increasing $V$.
- $\beta_m(V)$ **decreases** with increasing $V$.
- $\alpha_n(V)$ **increases** with increasing $V$.

### Method

1. Choose two voltages $V_1 < V_2$.
2. Assert the expected ordering holds.

### Why This Matters

Monotonicity errors often indicate:

- Sign mistakes inside exponentials.
- Incorrect voltage shifts.
- Incorrect numerator/denominator structure.

These errors may not cause crashes but will produce incorrect dynamics.

## Test Philosophy

- **No plotting:** Tests are purely numerical.
- **No logging:** Output is silent unless a failure occurs.
- **No external libraries:** Depends only on the standard math library.
- **Fail fast:** Abort or print clear error messages immediately upon failure.
- **Validate mathematics:** Focus on mathematical properties, not implementation details.

Once all tests pass, the rate layer is considered numerically safe and ready for use in the full Hodgkin–Huxley model.
