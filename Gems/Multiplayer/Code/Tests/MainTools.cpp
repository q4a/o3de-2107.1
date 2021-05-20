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

#include <AzCore/UnitTest/UnitTest.h>
#include <AzQtComponents/Utilities/QtPluginPaths.h>
#include <AzTest/AzTest.h>
#include <AzTest/GemTestEnvironment.h>
#include <QApplication>
#include <Multiplayer/Components/NetBindComponent.h>
#include <Source/Pipeline/NetBindMarkerComponent.h>
#include <Source/Pipeline/NetworkSpawnableHolderComponent.h>
#include <UnitTest/ToolsTestApplication.h>

namespace Multiplayer
{
    class MultiplayerToolsTestEnvironment : public AZ::Test::GemTestEnvironment
    {
        AZ::ComponentApplication* CreateApplicationInstance() override
        {
            return aznew UnitTest::ToolsTestApplication("MultiplayerToolsTest");
        }
        
        void AddGemsAndComponents() override
        {
            AZStd::vector<AZ::ComponentDescriptor*> descriptors({
                NetBindComponent::CreateDescriptor(),
                NetBindMarkerComponent::CreateDescriptor(),
                NetworkSpawnableHolderComponent::CreateDescriptor()
            });

            AddComponentDescriptors(descriptors);
        }
    };
} // namespace UnitTest

// Required to support running integration tests with Qt
AZTEST_EXPORT int AZ_UNIT_TEST_HOOK_NAME(int argc, char** argv)
{
    ::testing::InitGoogleMock(&argc, argv);
    AzQtComponents::PrepareQtPaths();
    QApplication app(argc, argv);
    AZ::Test::printUnusedParametersWarning(argc, argv);
    AZ::Test::addTestEnvironments({new Multiplayer::MultiplayerToolsTestEnvironment});
    int result = RUN_ALL_TESTS();
    return result;
}
