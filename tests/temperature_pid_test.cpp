#include "control/TemperaturePid.h"

#include <cassert>
#include <cmath>

namespace {

PredictiveHeatingPID makeSaturationController() {
    PredictiveHeatingPID pid;
    pid.target = 40.0;
    pid.setTunings(6.0, 0.05, 120.0);
    pid.prediction_seconds = 45.0;
    pid.full_power_error = 8.0;
    pid.max_output = 60.0;
    pid.approach_max_output = 20.0;
    return pid;
}

PredictiveHeatingPID makeOpcController() {
    PredictiveHeatingPID pid;
    pid.target = 40.0;
    pid.setTunings(8.0, 0.10, 60.0);
    pid.prediction_seconds = 30.0;
    pid.full_power_error = 6.0;
    pid.max_output = 50.0;
    pid.approach_max_output = 30.0;
    pid.integral_band = 2.0;
    pid.preserve_integral_while_coasting = true;
    return pid;
}

} // namespace

int main() {
    {
        auto pid = makeSaturationController();
        // The pair of saturation heaters must no longer receive 100% during
        // warm-up, even when the measured temperature is far below target.
        assert(std::abs(pid.compute(25.0, 0.5) - 60.0) < 1e-9);
    }
    {
        auto pid = makeSaturationController();
        pid.compute(30.0, 0.5);
        // A sustained 1 C/s rise represents substantial stored heat. The
        // 45-second prediction must command coasting immediately.
        assert(std::abs(pid.compute(30.5, 0.5)) < 1e-9);
    }
    {
        auto pid = makeSaturationController();
        pid.setTunings(100.0, 20.0, 0.0);
        // Manual PID gains cannot bypass the hardware-specific approach cap.
        assert(pid.compute(35.0, 0.5) <= 20.0);
        assert(std::abs(pid.compute(40.0, 0.5)) < 1e-9);
        assert(std::abs(pid.compute(44.0, 0.5)) < 1e-9);
    }
    {
        auto pid = makeOpcController();
        assert(std::abs(pid.compute(25.0, 0.5) - 50.0) < 1e-9);
    }
    {
        auto pid = makeOpcController();
        pid.compute(30.0, 0.5);
        // A fast sustained rise makes the coast prediction cross the
        // target, so heat must be removed before the measured setpoint.
        assert(std::abs(pid.compute(30.5, 0.5)) < 1e-9);
    }
    {
        auto pid = makeOpcController();
        double output = 0.0;
        // A slow 0.02 C/s rise near the target must retain useful heat instead
        // of being held more than one degree below the setpoint.
        for (int i = 0; i <= 20; ++i) {
            output = pid.compute(38.35 + (0.01 * i), 0.5);
        }
        assert(output > 5.0);
    }
    {
        auto pid = makeOpcController();
        double output = 0.0;
        // At a steady 39 C, integral action must build enough holding power to
        // remove the former one-degree steady-state error.
        for (int i = 0; i < 120; ++i) {
            output = pid.compute(39.0, 0.5);
        }
        assert(output > 13.0);

        const double accumulatedIntegral = pid.integral;
        assert(std::abs(pid.compute(39.5, 0.5)) < 1e-9);
        assert(std::abs(pid.integral - accumulatedIntegral) < 1e-9);
        assert(std::abs(pid.compute(40.0, 0.5)) < 1e-9);
        assert(std::abs(pid.integral) < 1e-9);
    }
    {
        auto pid = makeOpcController();
        // The controller must never heat at or above the target.
        assert(std::abs(pid.compute(40.0, 0.5)) < 1e-9);
        assert(std::abs(pid.compute(45.0, 0.5)) < 1e-9);
    }
    {
        auto pid = makeOpcController();
        // Even with aggressive user tunings, the approach cap remains a hard
        // constraint on the 30 W heater.
        pid.setTunings(100.0, 20.0, 0.0);
        assert(pid.compute(35.0, 0.5) <= 30.0);
    }
    return 0;
}
