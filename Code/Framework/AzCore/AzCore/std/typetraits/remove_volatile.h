/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/typetraits/config.h>

namespace AZStd
{
    using std::remove_volatile;
    template<class T>
    using remove_volatile_t = std::remove_volatile_t<T>;
}
