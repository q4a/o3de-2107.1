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
#include <Atom/RPI.Reflect/Shader/ShaderVariantAsset.h>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/algorithm.h>

#include <Atom/RHI.Reflect/ShaderStageFunction.h>

namespace AZ
{
    namespace RPI
    {
        uint32_t ShaderVariantAsset::MakeAssetProductSubId(
            uint32_t rhiApiUniqueIndex, ShaderVariantStableId variantStableId, uint32_t subProductType)
        {
            static constexpr uint32_t RhiIndexBitPosition = 30;
            static constexpr uint32_t RhiIndexNumBits = 32 - RhiIndexBitPosition;
            static constexpr uint32_t RhiIndexMaxValue = (1 << RhiIndexNumBits) - 1;

            static constexpr uint32_t SubProductTypeBitPosition = 17;
            static constexpr uint32_t SubProductTypeNumBits = RhiIndexBitPosition - SubProductTypeBitPosition;
            static constexpr uint32_t SubProductTypeMaxValue = (1 << SubProductTypeNumBits) - 1;

            static constexpr uint32_t StableIdBitPosition = 0;
            static constexpr uint32_t StableIdNumBits = SubProductTypeBitPosition - StableIdBitPosition;
            static constexpr uint32_t StableIdMaxValue = (1 << StableIdNumBits) - 1;

            static_assert(RhiIndexMaxValue == RHI::Limits::APIType::PerPlatformApiUniqueIndexMax);

            // The 2 Most significant bits encode the the RHI::API unique index.
            AZ_Assert(rhiApiUniqueIndex <= RhiIndexMaxValue, "Invalid rhiApiUniqueIndex [%u]", rhiApiUniqueIndex);
            AZ_Assert(subProductType <= SubProductTypeMaxValue, "Invalid subProductType [%u]", subProductType);
            AZ_Assert(variantStableId.GetIndex() <= StableIdMaxValue, "Invalid variantStableId [%u]", variantStableId.GetIndex());

            const uint32_t assetProductSubId = (rhiApiUniqueIndex << RhiIndexBitPosition) |
                (subProductType << SubProductTypeBitPosition) |
                (variantStableId.GetIndex() << StableIdBitPosition);
            return assetProductSubId;
        }

        void ShaderVariantAsset::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ShaderVariantAsset, AZ::Data::AssetData>()
                    ->Version(3)
                    ->Field("StableId", &ShaderVariantAsset::m_stableId)
                    ->Field("ShaderVariantId", &ShaderVariantAsset::m_shaderVariantId)
                    ->Field("IsFullyBaked", &ShaderVariantAsset::m_isFullyBaked)
                    ->Field("InputContract", &ShaderVariantAsset::m_inputContract)
                    ->Field("OutputContract", &ShaderVariantAsset::m_outputContract)
                    ->Field("RenderStates", &ShaderVariantAsset::m_renderStates)
                    ->Field("FunctionsByStage", &ShaderVariantAsset::m_functionsByStage)
                    ->Field("shaderAssetBuildTimestamp", &ShaderVariantAsset::m_shaderAssetBuildTimestamp)
                    ;
            }
        }

        AZStd::sys_time_t ShaderVariantAsset::GetShaderAssetBuildTimestamp() const
        {
            return m_shaderAssetBuildTimestamp;
        }

        const RHI::ShaderStageFunction* ShaderVariantAsset::GetShaderStageFunction(RHI::ShaderStage shaderStage) const
        {
            return m_functionsByStage[static_cast<size_t>(shaderStage)].get();
        }

        const ShaderInputContract& ShaderVariantAsset::GetInputContract() const
        {
            return m_inputContract;
        }

        const ShaderOutputContract& ShaderVariantAsset::GetOutputContract() const
        {
            return m_outputContract;
        }

        const RHI::RenderStates& ShaderVariantAsset::GetRenderStates() const
        {
            return m_renderStates;
        }

        bool ShaderVariantAsset::IsFullyBaked() const
        {
            return m_isFullyBaked;
        }

        void ShaderVariantAsset::SetReady()
        {
            m_status = AssetStatus::Ready;
        }

        bool ShaderVariantAsset::FinalizeAfterLoad()
        {
            return true;
        }

        ShaderVariantAssetHandler::LoadResult ShaderVariantAssetHandler::LoadAssetData(const Data::Asset<Data::AssetData>& asset, AZStd::shared_ptr<Data::AssetDataStream> stream, const AZ::Data::AssetFilterCB& assetLoadFilterCB)
        {
            if (Base::LoadAssetData(asset, stream, assetLoadFilterCB) == LoadResult::LoadComplete)
            {
                return PostLoadInit(asset) ? LoadResult::LoadComplete : LoadResult::Error;
            }
            return LoadResult::Error;
        }

        bool ShaderVariantAssetHandler::PostLoadInit(const Data::Asset<Data::AssetData>& asset)
        {
            if (ShaderVariantAsset* shaderVariantAsset = asset.GetAs<ShaderVariantAsset>())
            {
                if (!shaderVariantAsset->FinalizeAfterLoad())
                {
                    AZ_Error("ShaderVariantAssetHandler", false, "Shader asset failed to finalize.");
                    return false;
                }
                return true;
            }
            return false;
        }   
    } // namespace RPI
} // namespace AZ
