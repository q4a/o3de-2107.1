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

#include <AzCore/Math/Transform.h>
#include <AzCore/Math/TransformSerializer.h>

namespace AZ
{
    AZ_CLASS_ALLOCATOR_IMPL(JsonTransformSerializer, AZ::SystemAllocator, 0);

    JsonSerializationResult::Result JsonTransformSerializer::Load(
        void* outputValue, [[maybe_unused]] const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
        JsonDeserializerContext& context)
    {
        namespace JSR = JsonSerializationResult;

        if (azrtti_typeid<AZ::Transform>() != outputValueTypeId)
        {
            return context.Report(
                JSR::Tasks::ReadField, JSR::Outcomes::Unsupported,
                "Unable to deserialize Transform from json because the outputValueTypeId isn't a Transform type.");
        }

        AZ::Transform* transformInstance = reinterpret_cast<AZ::Transform*>(outputValue);
        AZ_Assert(transformInstance, "Output value for JsonTransformSerializer can't be null.");

        JSR::ResultCode result(JSR::Tasks::ReadField);

        {
            AZ::Vector3 translation = transformInstance->GetTranslation();

            JSR::ResultCode loadResult = ContinueLoadingFromJsonObjectField(
                &translation, azrtti_typeid<decltype(translation)>(), inputValue, TranslationTag, context);

            result.Combine(loadResult);

            transformInstance->SetTranslation(translation);
        }

        {
            AZ::Quaternion rotation = transformInstance->GetRotation();

            JSR::ResultCode loadResult =
                ContinueLoadingFromJsonObjectField(&rotation, azrtti_typeid<decltype(rotation)>(), inputValue, RotationTag, context);

            result.Combine(loadResult);

            transformInstance->SetRotation(rotation);
        }

        {
            // Scale is transitioning to a single uniform scale value, but since it's still internally represented as a Vector3,
            // we need to pick one number to use for load/store operations.
            float scale = transformInstance->GetUniformScale();

            JSR::ResultCode loadResult =
                ContinueLoadingFromJsonObjectField(&scale, azrtti_typeid<decltype(scale)>(), inputValue, ScaleTag, context);

            result.Combine(loadResult);

            transformInstance->SetScale(AZ::Vector3(scale));
        }

        return context.Report(
            result,
            result.GetProcessing() != JSR::Processing::Halted ? "Succesfully loaded Transform information."
                                                              : "Failed to load Transform information.");
    }

    JsonSerializationResult::Result JsonTransformSerializer::Store(
        rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue, [[maybe_unused]] const Uuid& valueTypeId,
        JsonSerializerContext& context)
    {
        namespace JSR = AZ::JsonSerializationResult;

        if (azrtti_typeid<AZ::Transform>() != valueTypeId)
        {
            return context.Report(
                JSR::Tasks::WriteValue, JSR::Outcomes::Unsupported,
                "Unable to Serialize Transform to json because the valueTypeId isn't a Transform type.");
        }

        const AZ::Transform* transformInstance = reinterpret_cast<const AZ::Transform*>(inputValue);
        AZ_Assert(transformInstance, "Input value for JsonTransformSerializer can't be null.");
        const AZ::Transform* defaultTransformInstance = reinterpret_cast<const AZ::Transform*>(defaultValue);

        JSR::ResultCode result(JSR::Tasks::WriteValue);

        {
            AZ::ScopedContextPath subPathName(context, TranslationTag);
            const AZ::Vector3 translation = transformInstance->GetTranslation();
            const AZ::Vector3 defaultTranslation = defaultTransformInstance ? defaultTransformInstance->GetTranslation() : AZ::Vector3();

            JSR::ResultCode storeResult = ContinueStoringToJsonObjectField(
                outputValue, TranslationTag, &translation, defaultTransformInstance ? &defaultTranslation : nullptr,
                azrtti_typeid<decltype(translation)>(), context);

            result.Combine(storeResult);
        }

        {
            AZ::ScopedContextPath subPathName(context, RotationTag);
            const AZ::Quaternion rotation = transformInstance->GetRotation();
            const AZ::Quaternion defaultRotation = defaultTransformInstance ? defaultTransformInstance->GetRotation() : AZ::Quaternion();

            JSR::ResultCode storeResult = ContinueStoringToJsonObjectField(
                outputValue, RotationTag, &rotation, defaultTransformInstance ? &defaultRotation : nullptr,
                azrtti_typeid<decltype(rotation)>(), context);

            result.Combine(storeResult);
        }

        {
            AZ::ScopedContextPath subPathName(context, ScaleTag);

            // Scale is transitioning to a single uniform scale value, but since it's still internally represented as a Vector3,
            // we need to pick one number to use for load/store operations.
            float scale = transformInstance->GetUniformScale();
            float defaultScale = defaultTransformInstance ? defaultTransformInstance->GetUniformScale() : 0.0f;

            JSR::ResultCode storeResult = ContinueStoringToJsonObjectField(
                outputValue, ScaleTag, &scale, defaultTransformInstance ? &defaultScale : nullptr, azrtti_typeid<decltype(scale)>(),
                context);

            result.Combine(storeResult);
        }

        return context.Report(
            result,
            result.GetProcessing() != JSR::Processing::Halted ? "Successfully stored Transform information."
                                                              : "Failed to store Transform information.");
    }

} // namespace AZ
