/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*/


#include <RHI.Builders/ShaderPlatformInterface.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <Atom/RHI.Edit/Utils.h>
#include <Atom/RHI.Reflect/Null/PipelineLayoutDescriptor.h>
#include <Atom/RHI.Reflect/Null/ShaderStageFunction.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace AZ
{
    namespace Null
    {

        static const char* WindowsAzslShaderHeader = "Builders/ShaderHeaders/Platform/Windows/Null/AzslcHeader.azsli";
        static const char* MacAzslShaderHeader = "Builders/ShaderHeaders/Platform/Mac/Null/AzslcHeader.azsli";

        ShaderPlatformInterface::ShaderPlatformInterface(uint32_t apiUniqueIndex)
            : RHI::ShaderPlatformInterface(apiUniqueIndex)
        {
        }

        RHI::APIType ShaderPlatformInterface::GetAPIType() const
        {
            return Null::RHIType;
        }

        AZ::Name ShaderPlatformInterface::GetAPIName() const
        {
            return m_apiName;
        }

        RHI::Ptr <RHI::PipelineLayoutDescriptor> ShaderPlatformInterface::CreatePipelineLayoutDescriptor()
        {
            return AZ::Null::PipelineLayoutDescriptor::Create();
        }
 
        bool ShaderPlatformInterface::BuildPipelineLayoutDescriptor(
            [[maybe_unused]] RHI::Ptr<RHI::PipelineLayoutDescriptor> pipelineLayoutDescriptorBase,
            [[maybe_unused]] const ShaderResourceGroupInfoList& srgInfoList,
            [[maybe_unused]] const RootConstantsInfo& rootConstantsInfo,
            [[maybe_unused]] const RHI::ShaderCompilerArguments& shaderCompilerArguments)
        {
            PipelineLayoutDescriptor* pipelineLayoutDescriptor = azrtti_cast<PipelineLayoutDescriptor*>(pipelineLayoutDescriptorBase.get());
            AZ_Assert(pipelineLayoutDescriptor, "PipelineLayoutDescriptor should have been created by now");
            return pipelineLayoutDescriptor->Finalize() == RHI::ResultCode::Success;
        }
    
        RHI::Ptr<RHI::ShaderStageFunction> ShaderPlatformInterface::CreateShaderStageFunction(const StageDescriptor& stageDescriptor)
        {
            RHI::Ptr<ShaderStageFunction> newShaderStageFunction = ShaderStageFunction::Create(RHI::ToRHIShaderStage(stageDescriptor.m_stageType));
            newShaderStageFunction->Finalize();
            return newShaderStageFunction;
        }

        bool ShaderPlatformInterface::IsShaderStageForRaster(RHI::ShaderHardwareStage shaderStageType) const
        {
            bool hasRasterProgram = false;

            hasRasterProgram |= shaderStageType == RHI::ShaderHardwareStage::Vertex;
            hasRasterProgram |= shaderStageType == RHI::ShaderHardwareStage::Fragment;

            return hasRasterProgram;
        }

        bool ShaderPlatformInterface::IsShaderStageForCompute(RHI::ShaderHardwareStage shaderStageType) const
        {
            return (shaderStageType == RHI::ShaderHardwareStage::Compute);
        }

        bool ShaderPlatformInterface::IsShaderStageForRayTracing(RHI::ShaderHardwareStage shaderStageType) const
        {
            return (shaderStageType == RHI::ShaderHardwareStage::RayTracing);
        }

        AZStd::string ShaderPlatformInterface::GetAzslCompilerParameters(const RHI::ShaderCompilerArguments& shaderCompilerArguments) const
        {
            return shaderCompilerArguments.MakeAdditionalAzslcCommandLineString() + " --use-spaces --namespace=dx --root-const=128";
        }

        AZStd::string ShaderPlatformInterface::GetAzslCompilerWarningParameters(const RHI::ShaderCompilerArguments& shaderCompilerArguments) const
        {
            return shaderCompilerArguments.MakeAdditionalAzslcWarningCommandLineString();
        }

        bool ShaderPlatformInterface::BuildHasDebugInfo([[maybe_unused]] const RHI::ShaderCompilerArguments& shaderCompilerArguments) const
        {
            return "";
        }

        const char* ShaderPlatformInterface::GetAzslHeader([[maybe_unused]] const AssetBuilderSDK::PlatformInfo& platform) const
        {
            if (platform.m_identifier == "pc")
            {
                return WindowsAzslShaderHeader;
            }
            else if (platform.m_identifier == "osx_gl")
            {
                return MacAzslShaderHeader;
            }
            else
            {
                return WindowsAzslShaderHeader;
            }
        }

        bool ShaderPlatformInterface::CompilePlatformInternal(
            [[maybe_unused]] const AssetBuilderSDK::PlatformInfo& platform, [[maybe_unused]] const AZStd::string& shaderSourcePath,
            [[maybe_unused]] const AZStd::string& functionName, [[maybe_unused]] RHI::ShaderHardwareStage shaderStage,
            [[maybe_unused]] const AZStd::string& tempFolderPath, [[maybe_unused]] StageDescriptor& outputDescriptor,
            [[maybe_unused]]  const RHI::ShaderCompilerArguments& shaderCompilerArguments) const
        {
            outputDescriptor.m_stageType = shaderStage;
            return true;
        }
    }
}