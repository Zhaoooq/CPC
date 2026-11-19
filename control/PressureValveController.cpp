#include "PressureValveController.h"

#include <algorithm>
#include <cmath>

namespace {

double clamp(double value, double minimum, double maximum) {
    return std::max(minimum, std::min(maximum, value));
}

} // namespace

void PressureValveController::setParameters(const Parameters& parameters) {
    parameters_ = parameters;
    parameters_.kp = std::max(0.0, parameters_.kp);
    parameters_.ki = std::max(0.0, parameters_.ki);
    parameters_.deadbandPercentOfFullScale =
        std::max(0.0, parameters_.deadbandPercentOfFullScale);
    parameters_.maxStepPercent =
        clamp(parameters_.maxStepPercent, 0.1, 100.0);
    parameters_.maxRatePercentPerSecond =
        clamp(parameters_.maxRatePercentPerSecond, 0.01, 100.0);
    parameters_.fineBandPercentOfFullScale =
        std::max(parameters_.deadbandPercentOfFullScale,
                 parameters_.fineBandPercentOfFullScale);
    parameters_.fineProportionalScale = clamp(parameters_.fineProportionalScale, 0.01, 1.0);
    parameters_.fineIntegralScale = clamp(parameters_.fineIntegralScale, 0.01, 1.0);
    parameters_.fineRatePercentPerSecond =
        clamp(parameters_.fineRatePercentPerSecond, 0.01,
              parameters_.maxRatePercentPerSecond);
    parameters_.resumeDelaySeconds =
        clamp(parameters_.resumeDelaySeconds, 0.0, 10.0);
}

void PressureValveController::reset(double openingPercent) {
    openingPercent_ = clamp(openingPercent, 0.0, 100.0);
    previousActionErrorPercent_ = 0.0;
    outsideDeadbandSeconds_ = 0.0;
    fineControlActive_ = false;
    holding_ = false;
    initialized_ = true;
}

double PressureValveController::update(double targetPressurePa,
                                       double measuredPressurePa,
                                       double fullScalePressurePa,
                                       double elapsedSeconds,
                                       bool openingRaisesPressure) {
    if (!initialized_) reset(openingPercent_);
    if (!std::isfinite(targetPressurePa) ||
        !std::isfinite(measuredPressurePa) ||
        !std::isfinite(fullScalePressurePa) ||
        !std::isfinite(elapsedSeconds) ||
        fullScalePressurePa <= 0.0 || elapsedSeconds <= 0.0) {
        return openingPercent_;
    }

    const double errorPercent =
        (targetPressurePa - measuredPressurePa) * 100.0 / fullScalePressurePa;
    const double actionErrorPercent = openingRaisesPressure
        ? errorPercent
        : -errorPercent;
    if (std::abs(errorPercent) <= parameters_.fineBandPercentOfFullScale) {
        // Once near the setpoint, keep the gentler gains for the rest of
        // this run. A transient must not restore the aggressive approach
        // gains while the valve/pressure feedback is still responding.
        fineControlActive_ = true;
    }
    if (std::abs(errorPercent) <= parameters_.deadbandPercentOfFullScale) {
        // Hold the output in tolerance. Forcing the error to zero here
        // produces a proportional kick and can drive pressure out again.
        // Track the real error so leaving tolerance resumes smoothly.
        previousActionErrorPercent_ = actionErrorPercent;
        holding_ = true;
        outsideDeadbandSeconds_ = 0.0;
        return openingPercent_;
    }

    if (holding_) {
        outsideDeadbandSeconds_ += elapsedSeconds;
        // Ignore short excursions such as pump pulsation. Large deviations
        // resume immediately; sustained small errors still get corrected.
        if (std::abs(errorPercent) <= parameters_.fineBandPercentOfFullScale &&
            outsideDeadbandSeconds_ < parameters_.resumeDelaySeconds) {
            previousActionErrorPercent_ = actionErrorPercent;
            return openingPercent_;
        }
        holding_ = false;
        outsideDeadbandSeconds_ = 0.0;
    }

    const double proportionalScale = fineControlActive_
        ? parameters_.fineProportionalScale : 1.0;
    const double integralScale = fineControlActive_
        ? parameters_.fineIntegralScale : 1.0;
    const double proportionalChange =
        parameters_.kp * proportionalScale *
        (actionErrorPercent - previousActionErrorPercent_);
    const double integralChange =
        parameters_.ki * integralScale * actionErrorPercent * elapsedSeconds;
    const double rateLimit = fineControlActive_
        ? parameters_.fineRatePercentPerSecond
        : parameters_.maxRatePercentPerSecond;
    const double maxChange = std::min(parameters_.maxStepPercent,
                                      rateLimit * elapsedSeconds);
    const double requestedChange = clamp(proportionalChange + integralChange,
                                         -maxChange,
                                         maxChange);

    openingPercent_ = clamp(openingPercent_ + requestedChange, 0.0, 100.0);
    previousActionErrorPercent_ = actionErrorPercent;
    return openingPercent_;
}

double PressureValveController::openingPercent() const {
    return openingPercent_;
}
