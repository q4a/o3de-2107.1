/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

#include <Editor/Assets/ScriptCanvasAssetHolder.h>
#include <ScriptCanvas/Assets/ScriptCanvasAssetHandler.h>
#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <ScriptCanvas/Variable/VariableBus.h>
#include <ScriptCanvas/Variable/VariableData.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

namespace ScriptCanvasEditor
{
    /*! EditorScriptCanvasComponent
    The user facing Editor Component for interfacing with ScriptCanvas
    It connects to the AssetCatalogEventBus in order to remove the ScriptCanvasAssetHolder asset reference
    when the asset is removed from the file system. The reason the ScriptCanvasAssetHolder holder does not
    remove the asset reference itself is because the ScriptCanvasEditor MainWindow has a ScriptCanvasAssetHolder
    which it uses to maintain the asset data in memory. Therefore removing an open ScriptCanvasAsset from the file system
    will remove the reference from the EditorScriptCanvasComponent, but not the reference from the MainWindow allowing the
    ScriptCanvas graph to still be modified while open

    Finally per graph instance variables values are stored on the EditorScriptCanvasComponent and injected into the runtime ScriptCanvas component in BuildGameEntity
    */
    class EditorScriptCanvasComponent
        : public AzToolsFramework::Components::EditorComponentBase        
        , private EditorContextMenuRequestBus::Handler
        , private AzFramework::AssetCatalogEventBus::Handler
        , private EditorScriptCanvasComponentLoggingBus::Handler
        , private EditorScriptCanvasComponentRequestBus::Handler
        , private AssetTrackerNotificationBus::Handler
        , private AzToolsFramework::EditorEntityContextNotificationBus::Handler

    {
    public:
        AZ_COMPONENT(EditorScriptCanvasComponent, "{C28E2D29-0746-451D-A639-7F113ECF5D72}", AzToolsFramework::Components::EditorComponentBase);

        EditorScriptCanvasComponent();
        EditorScriptCanvasComponent(AZ::Data::Asset<ScriptCanvasAsset> asset);
        ~EditorScriptCanvasComponent() override;

        //=====================================================================
        // AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        //=====================================================================


        //=====================================================================
        // EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;
        void SetPrimaryAsset(const AZ::Data::AssetId&) override;
        //=====================================================================

        //=====================================================================
        // EditorScriptCanvasComponentLoggingBus
        AZ::NamedEntityId FindNamedEntityId() const override { return GetNamedEntityId(); }
        ScriptCanvas::GraphIdentifier GetGraphIdentifier() const override;
        //=====================================================================

        void OpenEditor();
        void CloseGraph();

        void SetName(const AZStd::string& name) { m_name = name; }
        const AZStd::string& GetName() const { return m_name; };
        AZ::EntityId GetEditorEntityId() const { return GetEntity() ? GetEntityId() : AZ::EntityId(); }
        AZ::NamedEntityId GetNamedEditorEntityId() const { return GetEntity() ? GetNamedEntityId() : AZ::NamedEntityId(); }
        
        //=====================================================================
        // EditorScriptCanvasComponentRequestBus
        void SetAssetId(const AZ::Data::AssetId& assetId) override;
        bool HasAssetId() const override;
        //=====================================================================

        //=====================================================================
        // EditorContextMenuRequestBus
        AZ::Data::AssetId GetAssetId() const override;
        //=====================================================================
        AZ::EntityId GetGraphEntityId() const;

        //=====================================================================
        // AssetTrackerNotificationBus
        void OnAssetReady(const ScriptCanvasMemoryAsset::pointer asset) override;
        void OnAssetSaved(const ScriptCanvasMemoryAsset::pointer asset, bool isSuccessful) override;
        void OnAssetReloaded(const ScriptCanvasMemoryAsset::pointer asset) override;
        //=====================================================================


        //=====================================================================
        // EditorEntityContextNotificationBus
        void OnStartPlayInEditor() override;

        void OnStopPlayInEditor() override;

    protected:
        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("ScriptCanvasService", 0x41fd58f3));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            (void)required;
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            (void)incompatible;
        }

        //=====================================================================
        // AssetCatalogEventBus
        void OnCatalogAssetAdded(const AZ::Data::AssetId& assetId) override;
        void OnCatalogAssetRemoved(const AZ::Data::AssetId& assetId, const AZ::Data::AssetInfo& assetInfo) override;
        //=====================================================================

        void OnScriptCanvasAssetChanged(AZ::Data::AssetId assetId);

        void UpdateName();

        //=====================================================================
        void OnScriptCanvasAssetReady(const ScriptCanvasMemoryAsset::pointer asset);
        //=====================================================================

        void AddVariable(AZStd::string_view varName, const ScriptCanvas::GraphVariable& varDatum);
        void AddNewVariables(const ScriptCanvas::VariableData& graphVarData);
        void RemoveVariable(const ScriptCanvas::VariableId& varId);
        void RemoveOldVariables(const ScriptCanvas::VariableData& graphVarData);
        bool UpdateVariable(const ScriptCanvas::GraphVariable& graphDatum, ScriptCanvas::GraphVariable& updateDatum, ScriptCanvas::GraphVariable& originalDatum);
        void LoadVariables(const ScriptCanvasMemoryAsset::pointer memoryAsset);
        void ClearVariables();

    private:
        AZ::Data::AssetId m_removedCatalogId;
        AZ::Data::AssetId m_previousAssetId;

        AZStd::string m_name;
        ScriptCanvasAssetHolder m_scriptCanvasAssetHolder;
        
        ScriptCanvas::EditableVariableData m_editableData;
    };
}
