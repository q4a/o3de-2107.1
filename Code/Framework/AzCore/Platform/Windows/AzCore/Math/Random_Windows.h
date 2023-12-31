/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>

namespace AZ
{
    /**
     * Provides a better yet more computationally costly random implementation.
     * To be used in less frequent scenarios, i.e. to get a good seed.
     */
    class BetterPseudoRandom_Windows
    {
    public:
        BetterPseudoRandom_Windows();
        ~BetterPseudoRandom_Windows();

        /// Fills a buffer with random data, if true is returned. Otherwise the random generator fails to generate random numbers.
        bool GetRandom(void* data, size_t dataSize);

        /// Template helper for value types \ref GetRandom
        template<class T>
        bool GetRandom(T& value)    { return GetRandom(&value, sizeof(T)); }

    private:
        AZ::u64 m_generatorHandle;
    };

    using BetterPseudoRandom_Platform = BetterPseudoRandom_Windows;
}
