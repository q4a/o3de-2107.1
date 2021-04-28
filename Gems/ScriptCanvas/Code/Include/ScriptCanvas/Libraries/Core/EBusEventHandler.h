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

#include <Include/ScriptCanvas/Libraries/Core/EBusEventHandler.generated.h>

#include <AzCore/std/parallel/mutex.h>
#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Utils/BehaviorContextUtils.h>
#include <AzCore/std/containers/map.h>

#include <ScriptCanvas/Core/EBusNodeBus.h>

namespace AZ
{
    class BehaviorEBus;
    class BehaviorEBusHandler;
}

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            struct EBusEventEntry
            {
                static bool EBusEventEntryVersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement);
                static void Reflect(AZ::ReflectContext* context);

                AZ_TYPE_INFO(EBusEventEntry, "{92A20C1B-A54A-4583-97DB-A894377ACE21}");
                
                AZStd::string m_eventName;
                EBusEventId m_eventId;
                SlotId m_eventSlotId;
                SlotId m_resultSlotId;
                AZStd::vector<SlotId> m_parameterSlotIds;
                int m_numExpectedArguments = {};
                bool m_resultEvaluated = {};
                
                bool m_shouldHandleEvent = false;
                bool m_isHandlingEvent = false;

                bool IsExpectingResult() const;

                bool ContainsSlot(SlotId slotId) const;
            };

            //! Provides a node that represents an EBus handler
            class EBusEventHandler 
                : public Node
                , public EBusHandlerNodeRequestBus::Handler
            {
            public:

                SCRIPTCANVAS_NODE(EBusEventHandler);

                static const char* c_busIdName;
                static const char* c_busIdTooltip;

                using Events = AZStd::vector<EBusEventEntry>;
                using EventMap = AZStd::map<AZ::Crc32, EBusEventEntry>;

                EBusEventHandler() 
                    : m_autoConnectToGraphOwner(true)
                {}

                ~EBusEventHandler() override;

                AZ::BehaviorEBus* GetBus() const;

                void OnInit() override;
                void OnActivate() override;
                void OnPostActivate() override;
                void OnDeactivate() override;

                void OnGraphSet() override;
                
                void CollectVariableReferences(AZStd::unordered_set< ScriptCanvas::VariableId >& variableIds) const override;
                bool ContainsReferencesToVariables(const AZStd::unordered_set< ScriptCanvas::VariableId >& variableIds) const override;

                size_t GenerateFingerprint() const override;

                // EBusHandlerNodeRequestBus...
                void SetAddressId(const Datum& datumValue) override;
                ////

                void InitializeBus(const AZStd::string& ebusName);

                void InitializeEvent(int eventIndex);

                bool IsOutOfDate(const VersionData& graphVersion) const override;

                
                
                bool CreateHandler(AZStd::string_view ebusName);

                void Connect();
                
                void Disconnect();
                
                const EBusEventEntry* FindEventWithSlot(const Slot& slot) const;

                AZ::Outcome<AZStd::string, void> GetFunctionCallName(const Slot* /*slot*/) const override;
                bool IsEBusAddressed() const override;
                AZStd::optional<size_t> GetEventIndex(AZStd::string eventName) const;
                const EBusEventEntry* FindEvent(const AZStd::string& name) const;
                AZStd::string GetEBusName() const override;
                bool IsAutoConnected() const override;
                const Datum* GetHandlerStartAddress() const override;
                const Slot* GetEBusConnectAddressSlot() const override;
                AZStd::vector<const Slot*> GetOnVariableHandlingDataSlots() const override;
                AZStd::vector<const Slot*> GetOnVariableHandlingExecutionSlots() const override;

                inline ScriptCanvas::EBusBusId GetEBusId() const { return m_busId; }
                inline const EventMap& GetEvents() const { return m_eventMap; }
                AZ::Outcome<AZStd::string> GetInternalOutKey(const Slot& slot) const override;
                const Slot* GetEBusConnectSlot() const override;
                const Slot* GetEBusDisconnectSlot() const override;
                AZStd::vector<SlotId> GetEventSlotIds() const override;
                AZStd::vector<SlotId> GetNonEventSlotIds() const override;

                AZ::Outcome<DependencyReport, void> GetDependencies() const override;
                               
                bool IsEventSlotId(const SlotId& slotId) const;

                bool IsEventHandler() const override;

                bool IsEventConnected(const EBusEventEntry& entry) const;

                bool IsVariableWriteHandler() const override;

                bool IsValid() const;
                
                inline bool IsIDRequired() const
                {
                    return m_ebus && BehaviorContextUtils::GetEBusAddressPolicy(*m_ebus) == AZ::EBusAddressPolicy::ById;
                }

                void SetAutoConnectToGraphOwner(bool enabled);

                void OnWriteEnd();

                AZStd::string GetNodeName() const override
                {
                    return GetDebugName();
                }

                AZStd::string GetDebugName() const override
                {
                    return AZStd::string::format("%s Handler", GetEBusName().c_str());
                }

                NodeTypeIdentifier GetOutputNodeType(const SlotId& slotId) const override;

            protected:

                ConstSlotsOutcome GetSlotsInExecutionThreadByTypeImpl(const Slot& executionSlot, CombinedSlotType targetSlotType, const Slot* executionChildSlot) const override;

                inline bool IsConfigured() const { return !m_eventMap.empty(); }
                
                void OnEvent(const char* eventName, const int eventIndex, AZ::BehaviorValueParameter* result, const int numParameters, AZ::BehaviorValueParameter* parameters);

                void OnInputSignal(const SlotId&) override;
                void OnInputChanged(const Datum& input, const SlotId& slotID) override;

            private:

                EBusEventHandler(const EBusEventHandler&) = delete;
                static void OnEventGenericHook(void* userData, const char* eventName, int eventIndex, AZ::BehaviorValueParameter* result, int numParameters, AZ::BehaviorValueParameter* parameters);

                EventMap m_eventMap;
                AZStd::string m_ebusName;
                ScriptCanvas::EBusBusId m_busId;

                bool m_autoConnectToGraphOwner;

                AZ::BehaviorEBusHandler* m_handler = nullptr;
                AZ::BehaviorEBus* m_ebus = nullptr;

                AZStd::recursive_mutex m_mutex;
            };
        }
    }
}
