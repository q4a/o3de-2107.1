/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/weak_ptr.h>
#include <AzCore/std/string/string.h>
#include <SceneAPI/SceneCore/Events/SceneSerializationBus.h>

namespace AZ
{
    class SceneSerializationHandler
        : public SceneAPI::Events::SceneSerializationBus::Handler
    {
    public:
        ~SceneSerializationHandler() override;

        void Activate();
        void Deactivate();

        AZStd::shared_ptr<SceneAPI::Containers::Scene> LoadScene(
            const AZStd::string& sceneFilePath, Uuid sceneSourceGuid) override;

    private:
        bool IsValidExtension(const AZStd::string& filePath) const;
        void CleanSceneMap();

        AZStd::unordered_map<AZStd::string, AZStd::weak_ptr<SceneAPI::Containers::Scene>> m_scenes;
    };
} // namespace AZ
