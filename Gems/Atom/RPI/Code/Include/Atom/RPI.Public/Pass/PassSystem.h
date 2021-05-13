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

#include <Atom/RHI.Reflect/Base.h>

#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Public/Pass/Pass.h>
#include <Atom/RPI.Public/Pass/PassLibrary.h>
#include <Atom/RPI.Public/Pass/PassFactory.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>

#include <Atom/RPI.Reflect/Asset/AssetHandler.h>

#include <AzFramework/Windowing/WindowBus.h>

namespace AZ
{
    namespace RHI
    {
        class FrameGraphBuilder;
    }

    namespace RPI
    {
        //! The central class of the pass system.
        //! Responsible for preparing the frame and keeping 
        //! track of which passes need rebuilding or deleting.
        //! Holds the root of the pass hierarchy.
        class PassSystem final
            : public PassSystemInterface
        {
            friend class PassTests;
        public:
            AZ_RTTI(PassSystem, "{6AA45529-53CF-4AEF-86DF-A696C760105B}", PassSystemInterface);
            AZ_CLASS_ALLOCATOR(PassSystem, AZ::SystemAllocator, 0);

            static void Reflect(ReflectContext* context);

            static void GetAssetHandlers(AssetHandlerPtrList& assetHandlers);

            PassSystem();

            AZ_DISABLE_COPY_MOVE(PassSystem);
            
            //! Initializes the PassSystem and the Root Pass and creates the Pass InstanceDatabase
            void Init();

            //! Deletes the Root Pass and shuts down the PassSystem
            void Shutdown();
            
            //! Called every frame, essentially the 'OnTick' of the pass system
            void FrameUpdate(RHI::FrameGraphBuilder& frameGraphBuilder);

            //! Called after the frame has been rendered. Allows passes to execute post frame logic.
            void FrameEnd();

            // PassSystemInterface functions...
            void ProcessQueuedChanges() override;
            bool LoadPassTemplateMappings(const AZStd::string& templateMappingPath) override;
            void WriteTemplateToFile(const PassTemplate& passTemplate, AZStd::string_view assetFilePath) override;
            void DebugPrintPassHierarchy() override;
            bool IsBuilding() const override;
            bool IsHotReloading() const override;
            void SetHotReloading(bool hotReloading) override;
            void SetTargetedPassDebuggingName(const AZ::Name& targetPassName) override;
            const AZ::Name& GetTargetedPassDebuggingName() const override;

            // PassSystemInterface factory related functions...
            void AddPassCreator(Name className, PassCreator createFunction) override;
            Ptr<Pass> CreatePassFromClass(Name passClassName, Name passName) override;
            Ptr<Pass> CreatePassFromTemplate(const AZStd::shared_ptr<PassTemplate>& passTemplate, Name passName) override;
            Ptr<Pass> CreatePassFromTemplate(Name templateName, Name passName) override;
            Ptr<Pass> CreatePassFromRequest(const PassRequest* passRequest) override;
            bool HasCreatorForClass(Name passClassName) override;

            // PassSystemInterface library related functions...
            bool HasPassesForTemplateName(const Name& templateName) const override;
            const AZStd::vector<Pass*>& GetPassesForTemplateName(const Name& templateName) const override;
            bool AddPassTemplate(const Name& name, const AZStd::shared_ptr<PassTemplate>& passTemplate) override;
            const AZStd::shared_ptr<PassTemplate> GetPassTemplate(const Name& name) const override;
            void RemovePassFromLibrary(Pass* pass) override;
            void RegisterPass(Pass* pass) override;
            void UnregisterPass(Pass* pass) override;
            AZStd::vector<Pass*> FindPasses(const PassFilter& passFilter) const override;
            SwapChainPass* FindSwapChainPass(AzFramework::NativeWindowHandle windowHandle) const override;

        private:
            // Returns the root of the pass tree hierarchy
            const Ptr<ParentPass>& GetRootPass() override;

            // Calls BuildAttachments() on passes queued in m_buildAttachmentsList
            void BuildPassAttachments();

            // Validates Pass Hierarchy after building
            void Validate();

            // Removes queued passes in m_deletePassList from the hierarchy
            void RemovePasses();

            // Functions for queuing passes in the lists below
            void QueueForBuildAttachments(Pass* pass) override;
            void QueueForRemoval(Pass* pass) override;

            // Lists for queuing passes for various function calls
            // Name of the list reflects the pass function it will call
            AZStd::vector< Ptr<Pass> > m_buildAttachmentsList;
            AZStd::vector< Ptr<Pass> > m_removePassList;

            // Library of pass descriptors that can be instantiated through data driven pass requests
            PassLibrary m_passLibrary;

            // Class responsible for creating passes
            PassFactory m_passFactory;

            // The root of the pass tree hierarchy
            Ptr<ParentPass> m_rootPass = nullptr;

            // Whether the Pass Hierarchy changed
            bool m_passHierarchyChanged = true;

            // Whether the Pass System is currently in it's building phase 
            bool m_isBuilding = false;

            // Whether the Pass System is currently hot reloading passes 
            bool m_isHotReloading = false;

            // Name of the pass targeted for debugging
            AZ::Name m_targetedPassDebugName;

            // Counts the number of passes
            int32_t m_passCounter = 0;
        };
    }   // namespace RPI
}   // namespace AZ
