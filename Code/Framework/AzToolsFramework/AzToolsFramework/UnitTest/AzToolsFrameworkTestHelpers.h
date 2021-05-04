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

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Slice/SliceAsset.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/parallel/binary_semaphore.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzTest/AzTest.h>
#include <AZTestShared/Math/MathTestHelpers.h>
#include <AZTestShared/Utils/Utils.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/Entity/EditorEntityTransformBus.h>
#include <AzToolsFramework/ComponentMode/EditorComponentModeBus.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzToolsFramework/Viewport/ActionBus.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/ViewportSelection/EditorDefaultSelection.h>
#include <AzToolsFramework/ViewportSelection/EditorInteractionSystemViewportSelectionRequestBus.h>
#include <AzToolsFramework/SourceControl/PerforceConnection.h>
#include <AzToolsFramework/UnitTest/ToolsTestApplication.h>

#include <ostream>

AZ_PUSH_DISABLE_WARNING(4127, "-Wunknown-warning-option") // warning suppressed: constant used in conditional expression
#include <QtTest/QtTest>
AZ_POP_DISABLE_WARNING

#define AUTO_RESULT_IF_SETTING_TRUE(_settingName, _result)  \
    {                                                       \
        bool settingValue = true;                          \
        if (auto* registry = AZ::SettingsRegistry::Get())   \
        {                                                   \
            registry->Get(settingValue, _settingName);      \
        }                                                   \
                                                            \
        if (settingValue)                                   \
        {                                                   \
            EXPECT_TRUE(_result);                           \
            return;                                         \
        }                                                   \
    }  

namespace AZ
{
    class Entity;
    class EntityId;

} // namespace AZ

namespace UnitTest
{
    constexpr AZStd::string_view prefabSystemSetting = "/Amazon/Preferences/EnablePrefabSystem";

    /// Test widget to store QActions generated by EditorTransformComponentSelection.
    class TestWidget
        : public QWidget
    {
    public:
        TestWidget()
            : QWidget()
        {
            // ensure TestWidget can intercept and filter any incoming events itself
            installEventFilter(this);
        }

        bool eventFilter(QObject* watched, QEvent* event) override;
    };

    /// Stores actions registered for either normal mode (regular viewport) editing and
    /// component mode editing.
    class TestEditorActions
        : private AzToolsFramework::EditorActionRequestBus::Handler
        , private AzToolsFramework::ComponentModeFramework::EditorComponentModeNotificationBus::Handler
    {
        // EditorActionRequestBus ...
        void AddActionViaBus(int id, QAction* action) override;
        void RemoveActionViaBus(QAction* action) override;
        void EnableDefaultActions() override;
        void DisableDefaultActions() override;
        void AttachOverride(QWidget* /*object*/) override {}
        void DetachOverride() override {}

        // EditorComponentModeNotificationBus ...
        void EnteredComponentMode(const AZStd::vector<AZ::Uuid>& componentTypes) override;
        void LeftComponentMode(const AZStd::vector<AZ::Uuid>& componentTypes) override;

    public:
        void Connect();
        void Disconnect();

        TestWidget m_componentModeWidget; /// Widget to hold component mode actions.
        TestWidget m_defaultWidget; /// Widget to hold normal viewport/editor actions.
    };

    //! Used to intercept various messages which get printed to the console during the startup of a tools application
    //! but are not relevant for testing.
    class ToolsApplicationMessageHandler
    {
    public:
        ToolsApplicationMessageHandler();
    private:
        AZStd::unique_ptr<ErrorHandler> m_gridMateMessageHandler;
        AZStd::unique_ptr<ErrorHandler> m_enginePathMessageHandler;
        AZStd::unique_ptr<ErrorHandler> m_skippingDriveMessageHandler;
        AZStd::unique_ptr<ErrorHandler> m_storageDriveMessageHandler;
        AZStd::unique_ptr<ErrorHandler> m_jsonComponentErrorHandler;
    };

    /// Base fixture for ToolsApplication editor tests.
    class ToolsApplicationFixture
        : public AllocatorsTestFixture
    {
    public:
        void SetUp() override final
        {
            using AzToolsFramework::GetEntityContextId;
            using AzToolsFramework::EditorInteractionSystemViewportSelectionRequestBus;

            AllocatorsTestFixture::SetUp();

            if (!GetApplication())
            {
                // Create & Start a new ToolsApplication if there's no existing one
                m_app = AZStd::make_unique<ToolsTestApplication>("ToolsApplication");
                m_app->Start(AzFramework::Application::Descriptor());
            }

            // without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash 
            // in the unit tests.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

            m_editorActions.Connect();

            const auto viewportHandlerBuilder =
                [this](const AzToolsFramework::EditorVisibleEntityDataCache* entityDataCache)
            {
                // create the default viewport (handles ComponentMode)
                AZStd::unique_ptr<AzToolsFramework::EditorDefaultSelection> defaultSelection =
                    AZStd::make_unique<AzToolsFramework::EditorDefaultSelection>(entityDataCache);

                // override the phantom widget so we can use out custom test widget
                defaultSelection->SetOverridePhantomWidget(&m_editorActions.m_componentModeWidget);

                return defaultSelection;
            };

            // setup default editor interaction model with the phantom widget overridden
            EditorInteractionSystemViewportSelectionRequestBus::Event(
                GetEntityContextId(), &EditorInteractionSystemViewportSelectionRequestBus::Events::SetHandler,
                viewportHandlerBuilder);

            SetUpEditorFixtureImpl();
        }

        void TearDown() override final
        {
            using AzToolsFramework::GetEntityContextId;
            using AzToolsFramework::EditorInteractionSystemViewportSelectionRequestBus;

            // Reset back to Default Handler to prevent having a handler with dangling "this" pointer
            EditorInteractionSystemViewportSelectionRequestBus::Event(
                GetEntityContextId(), &EditorInteractionSystemViewportSelectionRequestBus::Events::SetDefaultHandler);

            TearDownEditorFixtureImpl();
            m_editorActions.Disconnect();

            // Stop & delete the Application created by this fixture, hence not using GetApplication() here
            if (m_app)
            {
                m_app->Stop();
                m_app.reset();
            }

            AllocatorsTestFixture::TearDown();
        }

        virtual void SetUpEditorFixtureImpl() {}
        virtual void TearDownEditorFixtureImpl() {}

        AzToolsFramework::ToolsApplication* GetApplication()
        {
            if (m_app)
            {
                return m_app.get();
            }

            // Fallback to ToolsApplication created externally
            AZ::ComponentApplication* app = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(app, &AZ::ComponentApplicationBus::Events::GetApplication);

            // It's a valid case if GetApplication returns null here, it means there's no externally created Application exists.
            auto* toolsApp = azdynamic_cast<AzToolsFramework::ToolsApplication*>(app);
            AZ_Assert(!app || toolsApp, "ComponentApplication must be the ToolsApplication here. Unsuccessful dynamic_cast.");
            return toolsApp;
        }

    protected:
        TestEditorActions m_editorActions;
        ToolsApplicationMessageHandler m_messageHandler; // used to suppress trace messages in test output

    private:
        AZStd::unique_ptr<ToolsTestApplication> m_app;
    };

    class EditorEntityComponentChangeDetector
        : private AzToolsFramework::PropertyEditorEntityChangeNotificationBus::Handler
        , private AzToolsFramework::EditorTransformChangeNotificationBus::Handler
        , private AzToolsFramework::ToolsApplicationNotificationBus::Handler
    {
    public:
        explicit EditorEntityComponentChangeDetector(const AZ::EntityId entityId);
        ~EditorEntityComponentChangeDetector();

        bool ChangeDetected() const { return !m_componentIds.empty(); }
        bool PropertyDisplayInvalidated() const { return m_propertyDisplayInvalidated; }

        AZStd::vector<AZ::ComponentId> m_componentIds;

    private:
        // PropertyEditorEntityChangeNotificationBus ...
        void OnEntityComponentPropertyChanged(AZ::ComponentId componentId) override;

        // EditorTransformChangeNotificationBus ...
        void OnEntityTransformChanged(const AzToolsFramework::EntityIdList& entityIds) override;

        // ToolsApplicationNotificationBus ...
        void InvalidatePropertyDisplay(AzToolsFramework::PropertyModificationRefreshLevel level) override;

        bool m_propertyDisplayInvalidated = false;
    };

    // Moves the 'Editor' (ToolsApplication) into Component Mode for the given Component type.
    // @note Entities must be selected before using EnterComponentMode to ensure
    // Component Mode is entered correctly.
    template<typename ComponentT>
    void EnterComponentMode()
    {
        using AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus;
        ComponentModeSystemRequestBus::Broadcast(
            &ComponentModeSystemRequestBus::Events::AddSelectedComponentModesOfType,
            AZ::AzTypeInfo<ComponentT>::Uuid());
    }

    struct MockPerforceCommand
        : AzToolsFramework::PerforceCommand
    {
        void ExecuteCommand() override;

        void ExecuteRawCommand() override;

        bool m_persistFstatResponse = false;
        AZStd::string m_fstatResponse;
        AZStd::string m_fstatErrorResponse;
        AZStd::function<void(AZStd::string)> m_addCallback;
        AZStd::function<void(AZStd::string)> m_editCallback;
        AZStd::function<void(AZStd::string)> m_moveCallback;
        AZStd::function<void(AZStd::string)> m_deleteCallback;
    };

    struct MockPerforceConnection
        : AzToolsFramework::PerforceConnection
    {
        explicit MockPerforceConnection(MockPerforceCommand& command) : PerforceConnection(command)
        {
        }
    };

    void WaitForSourceControl(AZStd::binary_semaphore& waitSignal);

    struct SourceControlTest
        : AzToolsFramework::SourceControlNotificationBus::Handler
    {
        SourceControlTest();
        ~SourceControlTest();

        void ConnectivityStateChanged(const AzToolsFramework::SourceControlState state) override;
        void EnableSourceControl();

        bool m_connected = false;
        AZStd::binary_semaphore m_connectSignal;
        MockPerforceCommand m_command;
    };

    /// Create an Entity as it would appear in the Editor.
    /// Optional second parameter of Entity pointer if required.
    AZ::EntityId CreateDefaultEditorEntity(const char* name, AZ::Entity** outEntity = nullptr);

    /// Create a Layer Entity as it would appear in the Editor.
    /// Optional second parameter of Entity pointer if required.
    AZ::EntityId CreateEditorLayerEntity(const char* name, AZ::Entity** outEntity = nullptr);

    using SliceAssets = AZStd::unordered_map<AZ::Data::AssetId, AZ::Data::Asset<AZ::SliceAsset>>;

    /// This function transfers the ownership of all the entity pointers - do not delete or use them afterwards.
    AZ::Data::AssetId SaveAsSlice(
        AZStd::vector<AZ::Entity*> entities, AzToolsFramework::ToolsApplication* toolsApplication,
        SliceAssets& inoutSliceAssets);

    /// Instantiate the entities from the saved slice asset.
    AZ::SliceComponent::EntityList InstantiateSlice(
        AZ::Data::AssetId sliceAssetId, const SliceAssets& sliceAssets);

    /// Destroy all the created slice assets.
    void DestroySlices(SliceAssets& sliceAssets);

} // namespace UnitTest
