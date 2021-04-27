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
#pragma once

#include <Atom/RHI.Reflect/AttachmentId.h>
#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <Atom/RHI.Reflect/AttachmentLoadStoreAction.h>
#include <Atom/RHI.Reflect/BufferViewDescriptor.h>
#include <Atom/RHI.Reflect/ImageViewDescriptor.h>
#include <Atom/RHI.Reflect/ClearValue.h>

namespace AZ
{
    namespace RHI
    {
        /**
         * Describes the binding of an attachment to a scope.
         */
        struct ScopeAttachmentDescriptor
        {
            AZ_TYPE_INFO(ScopeAttachmentDescriptor, "{04BBA36D-9A61-4E24-9CA0-7A4307C6A411}");
            
            static void Reflect(AZ::ReflectContext* context);

            ScopeAttachmentDescriptor() = default;
            virtual ~ScopeAttachmentDescriptor() = default;

            explicit ScopeAttachmentDescriptor(
                const AttachmentId& attachmentId,
                const AttachmentLoadStoreAction& loadStoreAction = AttachmentLoadStoreAction());

            /// The attachment id associated with the binding.
            AttachmentId m_attachmentId;

            /// The load/store action for the scope attachment
            AttachmentLoadStoreAction m_loadStoreAction;
        };
    }
}
