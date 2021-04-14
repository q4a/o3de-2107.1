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

#include <Atom/RHI/RayTracingAccelerationStructure.h>
#include <Atom/RHI/Buffer.h>
#include <Atom/RHI/Factory.h>

namespace AZ
{
    namespace RHI
    {
        RayTracingBlasDescriptor* RayTracingBlasDescriptor::Build()
        {
            return this;
        }

        RayTracingBlasDescriptor* RayTracingBlasDescriptor::Geometry()
        {
            m_geometries.emplace_back();
            m_buildContext = &m_geometries.back();
            return this;
        }

        RayTracingBlasDescriptor* RayTracingBlasDescriptor::VertexBuffer(const RHI::StreamBufferView& vertexBuffer)
        {
            AZ_Assert(m_buildContext, "VertexBuffer property can only be added to a Geometry entry");
            m_buildContext->m_vertexBuffer = vertexBuffer;
            return this;
        }

        RayTracingBlasDescriptor* RayTracingBlasDescriptor::VertexFormat(RHI::Format vertexFormat)
        {
            AZ_Assert(m_buildContext, "VertexFormat property can only be added to a Geometry entry");
            m_buildContext->m_vertexFormat = vertexFormat;
            return this;
        }

        RayTracingBlasDescriptor* RayTracingBlasDescriptor::IndexBuffer(const RHI::IndexBufferView& indexBuffer)
        {
            AZ_Assert(m_buildContext, "IndexBuffer property can only be added to a Geometry entry");
            m_buildContext->m_indexBuffer = indexBuffer;
            return this;
        }

        RayTracingTlasDescriptor* RayTracingTlasDescriptor::Build()
        {
            return this;
        }
        
        RayTracingTlasDescriptor* RayTracingTlasDescriptor::Instance()
        {
            AZ_Assert(m_instancesBuffer == nullptr, "Instance cannot be combined with an instances buffer");
            m_instances.emplace_back();
            m_buildContext = &m_instances.back();
            return this;
        }
        
        RayTracingTlasDescriptor* RayTracingTlasDescriptor::InstanceID(uint32_t instanceID)
        {
            AZ_Assert(m_buildContext, "InstanceID property can only be added to an Instance entry");
            m_buildContext->m_instanceID = instanceID;
            return this;
        }
        
        RayTracingTlasDescriptor* RayTracingTlasDescriptor::HitGroupIndex(uint32_t hitGroupIndex)
        {
            AZ_Assert(m_buildContext, "HitGroupIndex property can only be added to an Instance entry");
            m_buildContext->m_hitGroupIndex = hitGroupIndex;
            return this;
        }
        
        RayTracingTlasDescriptor* RayTracingTlasDescriptor::Matrix3x4(const AZ::Matrix3x4& matrix3x4)
        {
            AZ_Assert(m_buildContext, "Matrix3x4 property can only be added to an Instance entry");
            m_buildContext->m_matrix3x4 = matrix3x4;
            return this;
        }
        
        RayTracingTlasDescriptor* RayTracingTlasDescriptor::Blas(RHI::Ptr<RHI::RayTracingBlas>& blas)
        {
            AZ_Assert(m_buildContext, "Blas property can only be added to an Instance entry");
            m_buildContext->m_blas = blas;
            return this;
        }

        RayTracingTlasDescriptor* RayTracingTlasDescriptor::InstancesBuffer(RHI::Ptr<RHI::Buffer>& instancesBuffer)
        {
            AZ_Assert(!m_buildContext, "InstancesBuffer property can only be added to the top level");
            AZ_Assert(m_instances.size() == 0, "InstancesBuffer cannot exist with instance entries");
            m_instancesBuffer = instancesBuffer;
            return this;
        }

        RayTracingTlasDescriptor* RayTracingTlasDescriptor::NumInstances(uint32_t numInstancesInBuffer)
        {
            AZ_Assert(m_instancesBuffer.get(), "NumInstances property can only be added to the InstancesBuffer entry");
            m_numInstancesInBuffer = numInstancesInBuffer;
            return this;
        }

        RHI::Ptr<RHI::RayTracingBlas> RayTracingBlas::CreateRHIRayTracingBlas()
        {
            RHI::Ptr<RHI::RayTracingBlas> rayTracingBlas = RHI::Factory::Get().CreateRayTracingBlas();
            AZ_Error("RayTracingBlas", rayTracingBlas.get(), "Failed to create RHI::RayTracingBlas");
            return rayTracingBlas;
        }

        ResultCode RayTracingBlas::CreateBuffers(Device& device, const RayTracingBlasDescriptor* descriptor, const RayTracingBufferPools& rayTracingBufferPools)
        {
            ResultCode resultCode = CreateBuffersInternal(device, descriptor, rayTracingBufferPools);
            if (resultCode == ResultCode::Success)
            {
                DeviceObject::Init(device);
            }
            return resultCode;
        }

        RHI::Ptr<RHI::RayTracingTlas> RayTracingTlas::CreateRHIRayTracingTlas()
        {
            RHI::Ptr<RHI::RayTracingTlas> rayTracingTlas = RHI::Factory::Get().CreateRayTracingTlas();
            AZ_Error("RayTracingTlas", rayTracingTlas.get(), "Failed to create RHI::RayTracingTlas");
            return rayTracingTlas;
        }

        ResultCode RayTracingTlas::CreateBuffers(Device& device, const RayTracingTlasDescriptor* descriptor, const RayTracingBufferPools& rayTracingBufferPools)
        {
            ResultCode resultCode = CreateBuffersInternal(device, descriptor, rayTracingBufferPools);
            if (resultCode == ResultCode::Success)
            {
                DeviceObject::Init(device);
            }
            return resultCode;
        }
    }
}
