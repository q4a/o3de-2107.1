/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <Vegetation/Ebuses/AreaConfigRequestBus.h>

namespace Vegetation
{
    class SpawnerRequests
        : public AreaConfigRequests
    {
    public:
        /**
         * Overrides the default AZ::EBusTraits handler policy to allow one
         * listener only.
         */
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual bool GetInheritBehavior() const = 0;
        virtual void SetInheritBehavior(bool value) = 0;

        virtual bool GetAllowEmptyMeshes() const = 0;
        virtual void SetAllowEmptyMeshes(bool value) = 0;

        virtual FilterStage GetFilterStage() const = 0;
        virtual void SetFilterStage(FilterStage filterStage) = 0;
    };

    using SpawnerRequestBus = AZ::EBus<SpawnerRequests>;
}
