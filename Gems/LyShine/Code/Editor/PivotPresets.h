/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>
#include <AzCore/Math/Vector2.h>

namespace PivotPresets
{
    static const int PresetIndexCount = 9;

    int PivotToPresetIndex(const AZ::Vector2& v);
    const AZ::Vector2& PresetIndexToPivot(int i);
}   // namespace PivotPresets
