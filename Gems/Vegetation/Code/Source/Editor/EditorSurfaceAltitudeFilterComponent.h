/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Components/SurfaceAltitudeFilterComponent.h>
#include <Vegetation/Editor/EditorVegetationComponentBase.h>

namespace Vegetation
{
    class EditorSurfaceAltitudeFilterComponent
        : public EditorVegetationComponentBase<SurfaceAltitudeFilterComponent, SurfaceAltitudeFilterConfig>
    {
    public:
        using DerivedClassType = EditorSurfaceAltitudeFilterComponent;
        using BaseClassType = EditorVegetationComponentBase<SurfaceAltitudeFilterComponent, SurfaceAltitudeFilterConfig>;
        AZ_EDITOR_COMPONENT(EditorSurfaceAltitudeFilterComponent, EditorSurfaceAltitudeFilterComponentTypeId, BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        static constexpr const char* const s_categoryName = "Vegetation Filters";
        static constexpr const char* const s_componentName = "Vegetation Altitude Filter";
        static constexpr const char* const s_componentDescription = "Limits vegetation to only place within the specified height range";
        static constexpr const char* const s_icon = "Editor/Icons/Components/VegetationFilter.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/VegetationFilter.png";
        static constexpr const char* const s_helpUrl = "https://docs.aws.amazon.com/console/lumberyard/vegetationfilters/vegetation-altitude-filter";

        AZ::u32 ConfigurationChanged() override;
    };
}
