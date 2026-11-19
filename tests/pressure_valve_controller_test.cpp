#include "control/PressureValveController.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <deque>
#include <iostream>

namespace {

PressureValveController::Parameters a1Parameters() {
    PressureValveController::Parameters parameters;
    parameters.deadbandPercentOfFullScale = 0.20;
    parameters.fineBandPercentOfFullScale = 2.0;
    parameters.maxRatePercentPerSecond = 4.0;
    return parameters;
}

void checkA1OnePaTolerance(bool raises, double target) {
    PressureValveController controller;
    controller.setParameters(a1Parameters());
    controller.reset(50.0);

    // The full allowed range, including both endpoints, must hold the valve.
    for (int repeat = 0; repeat < 10; ++repeat) {
        for (double offset : {-1.0, -0.99, -0.5, 0.0, 0.5, 0.99, 1.0}) {
            assert(controller.update(target, target + offset, 500.0, 0.5, raises) == 50.0);
        }
    }
    // Brief excursions beyond either boundary must not cause valve chatter.
    for (double offset : {-1.1, -1.0, 1.1, 1.0}) {
        assert(controller.update(target, target + offset, 500.0, 0.5, raises) == 50.0);
    }
    // Sustained deviations on either side must resume correction in the
    // correct direction, rather than remaining permanently in hold.
    for (double offset : {-2.0, 2.0}) {
        controller.reset(50.0);
        controller.update(target, target, 500.0, 0.5, raises);
        assert(controller.update(target, target + offset, 500.0, 0.5, raises) == 50.0);
        assert(controller.update(target, target + offset, 500.0, 0.5, raises) == 50.0);
        const double resumed = controller.update(target, target + offset, 500.0, 0.5, raises);
        const bool shouldIncrease = (offset < 0.0) == raises;
        assert(shouldIncrease ? resumed > 50.0 : resumed < 50.0);
    }
    // Large deviations must bypass the short-excursion delay.
    for (double offset : {-11.0, 11.0}) {
        controller.reset(50.0);
        controller.update(target, target, 500.0, 0.5, raises);
        const double resumed = controller.update(target, target + offset, 500.0, 0.5, raises);
        const bool shouldIncrease = (offset < 0.0) == raises;
        assert(shouldIncrease ? resumed > 50.0 : resumed < 50.0);
    }
}

double checkDelayedPlant(double gain, int delayTicks, bool raises,
                         double initialOpening, bool disturbance,
                         bool a1ZeroStart = false) {
    PressureValveController controller;
    if (a1ZeroStart) controller.setParameters(a1Parameters());
    controller.reset(initialOpening);
    const double target = a1ZeroStart ? 200.0 : 100.0;
    const double tolerance = 1.0;
    const double settlingBand = 2.0;
    double applied = initialOpening;
    double pressure = std::clamp(
        target + (raises ? 1.0 : -1.0) * gain * (applied - 60.0), 0.0, 500.0);
    double measured = pressure;
    std::deque<double> delayedOutput(delayTicks + 1, applied);
    double minPressure = 500.0, maxPressure = 0.0;
    double minOpening = 100.0, maxOpening = 0.0;
    double lastOutsideBandSeconds = 0.0;
    for (int tick = 0; tick < 4000; ++tick) {
        if (tick % 5 == 0) {
            const double requested = controller.update(target, measured, 500.0, 0.5, raises);
            if (std::abs(requested - applied) >= 0.05) {
                // 0.01 mA command resolution corresponds to about 0.0625%.
                applied = std::round(requested * 16.0) / 16.0;
            }
        }
        delayedOutput.push_back(applied);
        const double effectiveOpening = delayedOutput.front();
        delayedOutput.pop_front();
        const double offset = disturbance && tick >= 1200 ? (a1ZeroStart ? 50.0 : 12.0) : 0.0;
        const double equilibrium = std::clamp(
            target + offset + (raises ? 1.0 : -1.0) * gain * (effectiveOpening - 60.0),
            0.0, 500.0);
        pressure += (equilibrium - pressure) * (1.0 - std::exp(-0.1 / 0.4));
        // Approximate the sensor's 0.1 s reporting, EMA, and pump ripple.
        measured = 0.2 * (pressure + 0.5 * std::sin(tick * 0.9)) + 0.8 * measured;
        const int recoveryStart = disturbance ? 1200 : 0;
        if (tick >= recoveryStart && std::abs(pressure - target) > settlingBand) {
            lastOutsideBandSeconds = (tick - recoveryStart) * 0.1;
        }
        if (tick >= 3000) {
            minPressure = std::min(minPressure, pressure);
            maxPressure = std::max(maxPressure, pressure);
            minOpening = std::min(minOpening, applied);
            maxOpening = std::max(maxOpening, applied);
        }
    }
    // Allow 0.6 Pa for the modeled sensor ripple and actuator quantization.
    if (minPressure < target - tolerance - 0.6 ||
        maxPressure > target + tolerance + 0.6 || maxOpening - minOpening > 0.25) {
        std::cerr << "Delayed plant failed: gain=" << gain << " delay=" << delayTicks
                  << " raises=" << raises << " initial=" << initialOpening
                  << " disturbance=" << disturbance << " pressure=" << minPressure
                  << ',' << maxPressure << " opening_span=" << maxOpening - minOpening << '\n';
        assert(false);
    }
    return lastOutsideBandSeconds;
}

} // namespace

int main() {
    double totalSettlingSeconds = 0.0;
    double totalZeroStartSeconds = 0.0;
    int plantCases = 0;
    int zeroStartCases = 0;
    for (bool raises : {false, true}) {
        for (double target : {50.0, 100.0, 200.0, 400.0}) {
            checkA1OnePaTolerance(raises, target);
        }
        PressureValveController controller;
        controller.reset(50.0);
        const double adjusted = controller.update(100.0, 98.9, 500.0, 0.5, raises);
        assert(raises ? adjusted > 50.0 : adjusted < 50.0);

        // Entering tolerance must retain the output that reached it.
        assert(controller.update(100.0, 99.1, 500.0, 0.5, raises) == adjusted);
        // Measurement noise inside tolerance must not move the valve.
        for (int sample = 0; sample < 100; ++sample) {
            const double pressure = sample % 2 ? 99.2 : 100.8;
            assert(controller.update(100.0, pressure, 500.0, 0.5, raises) == adjusted);
        }
        controller.update(100.0, 99.01, 500.0, 0.5, raises);
        // A 0.02 Pa boundary crossing must not reapply the entire P term.
        assert(controller.update(100.0, 98.99, 500.0, 0.5, raises) == adjusted);
        assert(controller.update(100.0, 98.99, 500.0, 0.5, raises) == adjusted);
        const double resumed = controller.update(100.0, 98.99, 500.0, 0.5, raises);
        assert(raises ? resumed > adjusted : resumed < adjusted);
        assert(std::abs(resumed - adjusted) < 0.02);

        // Integral action must still move the valve for a sustained error.
        const double beforeIntegral = controller.update(100.0, 98.0, 500.0, 0.5, raises);
        for (int sample = 0; sample < 500; ++sample) {
            controller.update(100.0, 98.0, 500.0, 0.5, raises);
        }
        assert(std::abs(controller.openingPercent() - beforeIntegral) > 1.0);

        // An unreachable target must not delay recovery when error reverses.
        controller.reset(raises ? 100.0 : 0.0);
        for (int sample = 0; sample < 100; ++sample) {
            controller.update(100.0, 50.0, 500.0, 0.5, raises);
        }
        const double saturated = controller.openingPercent();
        const double recovered = controller.update(100.0, 150.0, 500.0, 0.5, raises);
        assert(raises ? recovered < saturated : recovered > saturated);
        assert(std::abs(recovered - saturated) <= 5.0);

        // Brief excursions beyond 1 Pa must not make the valve chase ripple.
        controller.reset(50.0);
        controller.update(100.0, 100.0, 500.0, 0.5, raises);
        for (int sample = 0; sample < 60; ++sample) {
            const double pressure = sample % 3 == 0 ? 100.0 : 101.5;
            assert(controller.update(100.0, pressure, 500.0, 0.5, raises) == 50.0);
        }
        // A sustained error must resume correction instead of widening tolerance.
        for (int sample = 0; sample < 4; ++sample) {
            controller.update(100.0, 102.0, 500.0, 0.5, raises);
        }
        assert(raises ? controller.openingPercent() < 50.0
                      : controller.openingPercent() > 50.0);

        for (double dt : {0.2, 0.5, 1.0}) {
            // Jitter or increased UI gains must not bypass valve speed limits.
            PressureValveController limited;
            PressureValveController::Parameters parameters;
            parameters.kp = 10.0;
            parameters.ki = 10.0;
            limited.setParameters(parameters);
            limited.reset(50.0);
            const double approach = limited.update(100.0, 200.0, 500.0, dt, raises);
            assert(std::abs(approach - 50.0) <= 2.0 * dt + 1e-9);
            const double fine = limited.update(100.0, 109.0, 500.0, dt, raises);
            assert(std::abs(fine - approach) <= 1.0 * dt + 1e-9);
            // Keep slow control after a disturbance takes pressure outside the band.
            const double disturbed = limited.update(100.0, 150.0, 500.0, dt, raises);
            assert(std::abs(disturbed - fine) <= 1.0 * dt + 1e-9);
            // Starting a new run must clear the previous hold/slow-control state.
            limited.reset(50.0);
            const double restarted = limited.update(100.0, 200.0, 500.0, dt, raises);
            assert(std::abs(restarted - approach) < 1e-9);
        }

        for (double gain : {4.0, 12.0, 30.0}) {
            for (int delayTicks : {0, 5, 10, 20}) {
                totalZeroStartSeconds += checkDelayedPlant(
                    gain, delayTicks, raises, 0.0, false, true);
                ++zeroStartCases;
                checkDelayedPlant(gain, delayTicks, raises, 0.0, true, true);
                for (double initialOpening : {20.0, 59.5}) {
                    for (bool disturbance : {false, true}) {
                        totalSettlingSeconds += checkDelayedPlant(
                            gain, delayTicks, raises, initialOpening, disturbance);
                        ++plantCases;
                    }
                }
            }
        }
    }
    // Prevent gaining stability simply by making the controller too slow.
    // The previous settings averaged about 34.6 s to settle within 2 Pa.
    const double meanSettlingSeconds = totalSettlingSeconds / plantCases;
    assert(meanSettlingSeconds < 30.0);
    const double meanZeroStartSeconds = totalZeroStartSeconds / zeroStartCases;
    std::cout << "Pressure controller regression checks passed: " << plantCases
              << " plant cases, mean settling within 2 Pa = "
              << meanSettlingSeconds << " s; A1 0% start mean settling within 2 Pa = "
              << meanZeroStartSeconds << " s\n";
}
