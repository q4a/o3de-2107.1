/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Module/Module.h>

#include "AtomViewportDisplayInfoSystemComponent.h"

namespace AZ
{
    namespace Render
    {
        class AtomViewportDisplayInfoModule
            : public AZ::Module
        {
        public:
            AZ_RTTI(AtomViewportDisplayInfoModule, "{B10C0E55-03A1-4A46-AE3E-D3615AEAA659}", AZ::Module);
            AZ_CLASS_ALLOCATOR(AtomViewportDisplayInfoModule, AZ::SystemAllocator, 0);

            AtomViewportDisplayInfoModule()
                : AZ::Module()
            {
                m_descriptors.insert(m_descriptors.end(), {
                        AtomViewportDisplayInfoSystemComponent::CreateDescriptor(),
                    });
            }

            AZ::ComponentTypeList GetRequiredSystemComponents() const override
            {
                return AZ::ComponentTypeList{
                    azrtti_typeid<AtomViewportDisplayInfoSystemComponent>(),
                };
            }
        };
    } // namespace Render
} // namespace AZ

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_AtomViewportDisplayInfo, AZ::Render::AtomViewportDisplayInfoModule)
