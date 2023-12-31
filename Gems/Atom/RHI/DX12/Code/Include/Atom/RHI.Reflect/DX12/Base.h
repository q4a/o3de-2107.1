/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Base.h>
#include <Atom/RHI.Reflect/DX12/Base_Platform.h>

namespace AZ
{
    namespace DX12
    {
        //! For details go to AZ::RHI::Factory::GetAPIUniqueIndex 
        static constexpr uint32_t APIUniqueIndex = static_cast<uint32_t>(RHI::APIIndex::DX12);
    }
}
