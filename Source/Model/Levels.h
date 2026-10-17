// SPDX-License-Identifier: MIT
// Copyright (c) 2026 OpenGrid
#pragma once

// Gain ledger constants (docs/GAIN_STAGING.md).
//
// Host samples are differential volts at the input and output connectors, scaled so that a full-scale sine
// (peak 1.0) is +22 dBu, the reference unit's converter alignment: 0 dBFS = +22 dBu, -18 dBFS = +4 dBu.
// Internal circuit nodes run about 6 dB below connector level (the line receiver halves the differential
// signal and the output transformer steps it back up by two).

#include <algorithm>
#include <cmath>

namespace bluechannel::model
{
inline constexpr double kDbuVolts = 0.77459666924148;            // 0 dBu, RMS volts
inline constexpr double kFullScaleDbu = 22.0;
inline const double kFullScalePeak = kDbuVolts * std::sqrt(2.0) * std::pow(10.0, kFullScaleDbu / 20.0);  // 13.80 V

inline double dbuToPeakVolts(double dbu) { return kDbuVolts * std::sqrt(2.0) * std::pow(10.0, dbu / 20.0); }
inline double peakVoltsToDbu(double v) { return 20.0 * std::log10(std::max(1e-12, v) / (kDbuVolts * std::sqrt(2.0))); }

// Supply rails at the reference model's settled operating point, and the output swing of the dual low-noise
// op-amps used throughout (2 V from either rail in the reference device model).
inline constexpr double kRailPositive = 16.0718781;
inline constexpr double kRailNegative = -14.7679257;
inline constexpr double kSwingPositive = kRailPositive - 2.0;
inline constexpr double kSwingNegative = kRailNegative + 2.0;

// Internal node to connector level (output transformer step-up), used by meters and thresholds.
inline constexpr double kInternalToConnector = 2.0;
} // namespace bluechannel::model
