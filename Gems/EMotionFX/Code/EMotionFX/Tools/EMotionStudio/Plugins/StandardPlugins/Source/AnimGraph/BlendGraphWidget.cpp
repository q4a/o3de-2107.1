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

#include <AzQtComponents/Utilities/Conversions.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphConnectionCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphNodeCommands.h>
#include <EMotionFX/CommandSystem/Source/MotionSetCommands.h>
#include <EMotionFX/Source/AnimGraphExitNode.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphNodeGroup.h>
#include <EMotionFX/Source/AnimGraphObjectFactory.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/AnimGraphExitNode.h>
#include <Editor/AnimGraphEditorBus.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphActionManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphModel.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AttributesWindow.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphViewWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendTreeVisualNode.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/GraphNode.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/NavigationHistory.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/NodeConnection.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/NodeGraph.h>
#include <MCore/Source/ReflectionSerializer.h>
#include <MCore/Source/StandardHeaders.h>
#include <MysticQt/Source/KeyboardShortcutManager.h>
#include <MCore/Source/LogManager.h>

// qt includes
#include <QDropEvent>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QToolTip>
#include <QWidget>




namespace EMStudio
{
    // constructor
    BlendGraphWidget::BlendGraphWidget(AnimGraphPlugin* plugin, QWidget* parent)
        : NodeGraphWidget(plugin, nullptr, parent)
        , mContextMenuEventMousePos(0, 0)
        , mDoubleClickHappened(false)
    {
        mMoveGroup.SetGroupName("Move anim graph nodes");

        setAutoFillBackground(false);
        setAttribute(Qt::WA_OpaquePaintEvent);

        connect(&plugin->GetAnimGraphModel(), &AnimGraphModel::rowsInserted, this, &BlendGraphWidget::OnRowsInserted);
        connect(&plugin->GetAnimGraphModel(), &AnimGraphModel::dataChanged, this, &BlendGraphWidget::OnDataChanged);
        connect(&plugin->GetAnimGraphModel(), &AnimGraphModel::rowsAboutToBeRemoved, this, &BlendGraphWidget::OnRowsAboutToBeRemoved);
        connect(&plugin->GetAnimGraphModel(), &AnimGraphModel::FocusChanged, this, &BlendGraphWidget::OnFocusChanged);

        connect(&plugin->GetAnimGraphModel().GetSelectionModel(), &QItemSelectionModel::selectionChanged, this, &BlendGraphWidget::OnSelectionModelChanged);
    }


    // when dropping stuff in our window
    void BlendGraphWidget::dropEvent(QDropEvent* event)
    {
        // dont accept dragging/drop from and to yourself
        if (event->source() == this)
        {
            event->ignore();
            return;
        }

        if (!mActiveGraph ||
            !mPlugin->GetActionFilter().m_createNodes ||
            mActiveGraph->IsInReferencedGraph())
        {
            event->ignore();
            return;
        }

        // only accept copy actions
        if (event->dropAction() != Qt::CopyAction || event->mimeData()->hasText() == false)
        {
            event->ignore();
            return;
        }

        // if we have text, get it
        AZStd::string dropText = FromQtString(event->mimeData()->text());
        MCore::CommandLine commandLine(dropText.c_str());

        // calculate the drop position
        QPoint offset = LocalToGlobal(event->pos());
        QModelIndex targetModelIndex;
        if (GetActiveGraph())
        {
            targetModelIndex = GetActiveGraph()->GetModelIndex();
        }

        MCore::CommandGroup commandGroup("Add motion nodes");

        // check if the drag & drop is coming from an external window
        if (commandLine.CheckIfHasParameter("window"))
        {
            AZStd::vector<AZStd::string> droppedLines;
            AzFramework::StringFunc::Tokenize(dropText.c_str(), droppedLines, "\n", false, true);

            for (const AZStd::string& droppedLine : droppedLines)
            {
                MCore::CommandLine currentCommandLine(droppedLine.c_str());

                // get the name of the window where the drag came from
                AZStd::string dragWindow;
                currentCommandLine.GetValue("window", "", dragWindow);

                // drag&drop coming from the motion set window from the standard plugins
                if (dragWindow == "MotionSetWindow")
                {
                    AZStd::string motionId;
                    currentCommandLine.GetValue("motionNameID", "", motionId);

                    EMotionFX::AnimGraphMotionNode tempMotionNode;
                    AZStd::vector<AZStd::string> motionIds;
                    motionIds.emplace_back(motionId);
                    tempMotionNode.SetMotionIds(motionIds);

                    AZ::Outcome<AZStd::string> serializedMotionNode = MCore::ReflectionSerializer::Serialize(&tempMotionNode);
                    if (serializedMotionNode.IsSuccess())
                    {
                        if (targetModelIndex.isValid())
                        {
                            EMotionFX::AnimGraphNode* currentNode = targetModelIndex.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
                            CommandSystem::CreateAnimGraphNode(&commandGroup, currentNode->GetAnimGraph(), "BlendTreeMotionNode", "Motion", currentNode, offset.x(), offset.y(), serializedMotionNode.GetValue());

                            // setup the offset for the next motion
                            offset.setY(offset.y() + 60);
                        }
                    }
                }

                // drag&drop coming from the motion window from the standard plugins
                if (dragWindow == "MotionWindow")
                {
                    // get the motion id and the corresponding motion object
                    const uint32 motionID = currentCommandLine.GetValueAsInt("motionID", MCORE_INVALIDINDEX32);
                    EMotionFX::Motion* motion = EMotionFX::GetMotionManager().FindMotionByID(motionID);

                    if (!motion)
                    {
                        QMessageBox::warning(this, "Cannot Complete Drop Operation", QString("Motion id '%1' not found.").arg(motionID));
                        event->ignore();
                        return;
                    }

                    // get the anim graph instance from the current actor instance and check if it is valid
                    if (!targetModelIndex.isValid())
                    {
                        QMessageBox::warning(this, "Cannot Complete Drop Operation", "Please create an anim graph before dropping the motion.");
                        event->ignore();
                        return;
                    }

                    // Get the motion set from the anim graph instance.
                    EMotionFX::AnimGraphInstance* animGraphInstance = targetModelIndex.data(AnimGraphModel::ROLE_ANIM_GRAPH_INSTANCE).value<EMotionFX::AnimGraphInstance*>();
                    EMotionFX::MotionSet* motionSet = nullptr;
                    if (animGraphInstance)
                    {
                        motionSet = animGraphInstance->GetMotionSet();
                    }
                    else
                    {
                        // In case no anim graph is currently playing, use the selection from the node inspector.
                        EMotionFX::AnimGraphEditorRequestBus::BroadcastResult(motionSet, &EMotionFX::AnimGraphEditorRequests::GetSelectedMotionSet);
                        if (!motionSet)
                        {
                            // In case no motion set is selected and there is only one loaded, use that.
                            if (EMotionFX::GetMotionManager().GetNumMotionSets() == 1)
                            {
                                motionSet = EMotionFX::GetMotionManager().GetMotionSet(0);
                            }
                        }
                    }

                    if (!motionSet)
                    {
                        QMessageBox::warning(this, "No Motion Set Selected", "Cannot drop the motion to the anim graph. Please assign a motion set to the anim graph first.");
                        event->ignore();
                        return;
                    }

                    // try to find the motion entry for the given motion
                    EMotionFX::MotionSet::MotionEntry* motionEntry = motionSet->FindMotionEntry(motion);
                    if (motionEntry)
                    {
                        EMotionFX::AnimGraphMotionNode tempMotionNode;
                        AZStd::vector<AZStd::string> motionIds;
                        motionIds.emplace_back(motionEntry->GetId());
                        tempMotionNode.SetMotionIds(motionIds);

                        AZ::Outcome<AZStd::string> serializedContent = MCore::ReflectionSerializer::SerializeMembersExcept(&tempMotionNode, {"childNodes", "connections", "transitions"});
                        if (serializedContent.IsSuccess())
                        {
                            EMotionFX::AnimGraphNode* currentNode = targetModelIndex.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
                            CommandSystem::CreateAnimGraphNode(&commandGroup, currentNode->GetAnimGraph(), azrtti_typeid<EMotionFX::AnimGraphMotionNode>(), "Motion", currentNode, offset.x(), offset.y(), serializedContent.GetValue());

                            // setup the offset for the next motion
                            offset.setY(offset.y() + 60);
                        }
                    }
                    else
                    {
                        if (QMessageBox::warning(this, "Motion Not Part Of Motion Set", "Do you want the motion to be automatically added to the active motion set? When pressing no the drop action will be canceled.", QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
                        {
                            event->ignore();
                            return;
                        }

                        // Build a list of unique string id values from all motion set entries.
                        AZStd::vector<AZStd::string> idStrings;
                        motionSet->BuildIdStringList(idStrings);

                        // remove the media root folder from the absolute motion filename so that we get the relative one to the media root folder
                        AZStd::string motionEntryFileName = motion->GetFileName();
                        EMotionFX::GetEMotionFX().GetFilenameRelativeToMediaRoot(&motionEntryFileName);

                        if (EMotionFX::MotionSet::MotionEntry::CheckIfIsAbsoluteFilename(motionEntryFileName.c_str()))
                        {
                            AZStd::string text = AZStd::string::format("Some of the motions are located outside of the asset folder of your project:\n\n%s\n\nThis means that the motion set cannot store relative filenames and will hold absolute filenames.", EMotionFX::GetEMotionFX().GetMediaRootFolder());
                            QMessageBox::warning(this, "Warning", text.c_str());
                        }

                        const AZStd::string idString = CommandSystem::AddMotionSetEntry(motionSet->GetID(), "", idStrings, motionEntryFileName.c_str());

                        EMotionFX::AnimGraphMotionNode tempMotionNode;
                        AZStd::vector<AZStd::string> motionIds;
                        motionIds.emplace_back(idString);
                        tempMotionNode.SetMotionIds(motionIds);

                        AZ::Outcome<AZStd::string> serializedMotionNode = MCore::ReflectionSerializer::Serialize(&tempMotionNode);
                        if (serializedMotionNode.IsSuccess())
                        {
                            EMotionFX::AnimGraphNode* currentNode = targetModelIndex.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
                            CommandSystem::CreateAnimGraphNode(&commandGroup, currentNode->GetAnimGraph(), azrtti_typeid<EMotionFX::AnimGraphMotionNode>(), "Motion", currentNode, offset.x(), offset.y(), serializedMotionNode.GetValue());

                            // setup the offset for the next motion
                            offset.setY(offset.y() + 60);
                        }
                    }
                }
            }
        }
        // default handling, we're drag & dropping from the palette window
        else
        {
            AZStd::vector<AZStd::string> parts;
            AzFramework::StringFunc::Tokenize(dropText.c_str(), parts, ";", false, true);
            if (parts.size() != 3)
            {
                MCore::LogError("BlendGraphWidget::dropEvent() - Incorrect syntax using drop data '%s'", FromQtString(event->mimeData()->text()).c_str());
                event->ignore();
                return;
            }

            AZStd::string commandString;
            AZStd::string resultString;
            EMotionFX::AnimGraphNode* currentNode = targetModelIndex.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
            if (!currentNode)
            {
                event->ignore();
                return;
            }

            // build the name prefix
            AZStd::string& namePrefix = parts[2];
            AzFramework::StringFunc::Strip(namePrefix, MCore::CharacterConstants::space, true /* case sensitive */);

            const AZ::TypeId typeId = AZ::TypeId::CreateString(parts[1].c_str(), parts[1].size());
            CommandSystem::CreateAnimGraphNode(&commandGroup, currentNode->GetAnimGraph(), typeId, namePrefix, currentNode, offset.x(), offset.y());
        }

        if (!commandGroup.IsEmpty())
        {
            AZStd::string result;
            if (!GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
            {
                AZ_Error("EMotionFX", false, result.c_str());
            }
        }

        event->accept();
    }


    bool BlendGraphWidget::OnEnterDropEvent(QDragEnterEvent* event, EMotionFX::AnimGraphNode* currentNode, NodeGraph* activeGraph)
    {
        if (event->mimeData()->hasText() == false)
        {
            return false;
        }

        // if we have text, get it
        AZStd::string dropText = FromQtString(event->mimeData()->text());
        MCore::CommandLine commandLine(dropText.c_str());

        // check if the drag & drop is coming from an external window
        if (commandLine.CheckIfHasParameter("window"))
        {
            // in case the current node is nullptr and the active graph is a valid graph it means we are showing the root graph
            if (currentNode == nullptr)
            {
                QMessageBox::warning(GetMainWindow(), "Cannot Drop Motion", "Either there is no node shown or you are trying to add a motion to the root level which is not possible.");
                return false;
            }

            // check if we need to prevent dropping of non-state machine nodes
            if (azrtti_typeid(currentNode) == azrtti_typeid<EMotionFX::AnimGraphStateMachine>() ||
                azrtti_typeid(currentNode) == azrtti_typeid<EMotionFX::BlendTree>())
            {
                return true;
            }

            return false;
        }

        AZStd::vector<AZStd::string> parts;
        AzFramework::StringFunc::Tokenize(dropText.c_str(), parts, MCore::CharacterConstants::semiColon, true /* keep empty strings */, true /* keep space strings */);

        if (parts.size() != 3)
        {
            //MCore::LogError("BlendGraphWidget::dropEvent() - Incorrect syntax using drop data '%s'", FromQtString(event->mimeData()->text()).AsChar());
            return false;
        }

        if (parts[0].find("EMotionFX::AnimGraphNode") != 0)
        {
            return false;
        }

        // check if the dropped node is a state machine node
        bool canActAsState = false;
        const AZ::TypeId typeId = AZ::TypeId::CreateString(parts[1].c_str(), parts[1].size());
        if (!typeId.IsNull())
        {
            EMotionFX::AnimGraphObject* registeredObject = EMotionFX::AnimGraphObjectFactory::Create(typeId);
            MCORE_ASSERT(azrtti_istypeof<EMotionFX::AnimGraphNode>(registeredObject));

            EMotionFX::AnimGraphNode* registeredNode = static_cast<EMotionFX::AnimGraphNode*>(registeredObject);
            if (registeredNode->GetCanActAsState())
            {
                canActAsState = true;
            }

            delete registeredObject;
        }

        // in case the current node is nullptr and the active graph is a valid graph it means we are showing the root graph
        if (currentNode == nullptr)
        {
            if (activeGraph)
            {
                // only state machine nodes are allowed in the root graph
                if (canActAsState)
                {
                    return true;
                }
            }
        }
        else
        {
            // check if we need to prevent dropping of non-state machine nodes
            if (azrtti_typeid(currentNode) == azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
            {
                if (canActAsState)
                {
                    return true;
                }
            }
            else
            {
                return true;
            }
        }

        return false;
    }


    // drag enter
    void BlendGraphWidget::dragEnterEvent(QDragEnterEvent* event)
    {
        //MCore::LogDebug("BlendGraphWidget::dragEnter");
        if (GetActiveGraph())
        {
            EMotionFX::AnimGraphNode* currentNode = GetActiveGraph()->GetModelIndex().data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
            bool acceptEnterEvent = OnEnterDropEvent(event, currentNode, GetActiveGraph());

            if (acceptEnterEvent)
            {
                event->accept();
                return;
            }
        }
        event->ignore();
    }


    // drag leave
    void BlendGraphWidget::dragLeaveEvent(QDragLeaveEvent* event)
    {
        //MCore::LogDebug("BlendGraphWidget::dragLeave");
        event->accept();
    }


    // moving around while dragging
    void BlendGraphWidget::dragMoveEvent(QDragMoveEvent* event)
    {
        MCORE_UNUSED(event);
        // MCore::LogDebug("BlendGraphWidget::dragMove");
    }


    void BlendGraphWidget::OnContextMenuCreateNode()
    {
        NodeGraph* nodeGraph = GetActiveGraph();
        if (!nodeGraph)
        {
            return;
        }

        AZ_Assert(sender()->inherits("QAction"), "Expected being called from a QAction");
        QAction* action = qobject_cast<QAction*>(sender());

        // calculate the position
        const QPoint offset = SnapLocalToGrid(LocalToGlobal(mContextMenuEventMousePos));

        // build the name prefix and create the node
        const AZStd::string typeString = FromQtString(action->whatsThis());
        AZStd::string namePrefix = FromQtString(action->data().toString());
        AzFramework::StringFunc::Strip(namePrefix, MCore::CharacterConstants::space, true /* case sensitive */);
        const AZ::TypeId typeId = AZ::TypeId::CreateString(typeString.c_str(), typeString.size());

        const QModelIndex modelIndex = nodeGraph->GetModelIndex();
        EMotionFX::AnimGraphNode* currentNode = modelIndex.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
        CommandSystem::CreateAnimGraphNode(/*commandGroup=*/nullptr, currentNode->GetAnimGraph(), typeId, namePrefix, currentNode, offset.x(), offset.y());
    }


    bool BlendGraphWidget::CheckIfIsStateMachine()
    {
        NodeGraph* nodeGraph = GetActiveGraph();
        const QModelIndex modelIndex = nodeGraph->GetModelIndex();
        const EMotionFX::AnimGraphNode* animGraphNode = modelIndex.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
        return (azrtti_typeid(animGraphNode) == azrtti_typeid<EMotionFX::AnimGraphStateMachine>());
    }


    // enable or disable all selected transitions
    void BlendGraphWidget::SetSelectedTransitionsEnabled(bool isEnabled)
    {
        // only allowed when a state machine is currently being showed
        if (!CheckIfIsStateMachine())
        {
            return;
        }

        // gather the selected transitions
        AZStd::vector<NodeConnection*> selectedTransitions = GetActiveGraph()->GetSelectedNodeConnections();

        if (!selectedTransitions.empty())
        {
            MCore::CommandGroup commandGroup("Enable/disable transitions", static_cast<uint32>(selectedTransitions.size()));

            // iterate through the selected transitions and and enable or disable them
            for (NodeConnection* selectedTransition : selectedTransitions)
            {
                // get the transition and its visual representation
                StateConnection*                     visualTransition = static_cast<StateConnection*>(selectedTransition);
                EMotionFX::AnimGraphStateTransition* transition = FindTransitionForConnection(visualTransition);

                // get the target node
                EMotionFX::AnimGraphNode* targetNode = transition->GetTargetNode();
                if (targetNode == nullptr)
                {
                    MCore::LogError("Cannot enable/disable transition with id %s. Target node is invalid.", transition->GetId().ToString().c_str());
                    continue;
                }

                // get the parent node of the target node
                EMotionFX::AnimGraphNode* parentNode = targetNode->GetParentNode();
                if (parentNode == nullptr || (parentNode && azrtti_typeid(parentNode) != azrtti_typeid<EMotionFX::AnimGraphStateMachine>()))
                {
                    MCore::LogError("Cannot enable/disable transition with id %s. Parent node is invalid.", transition->GetId().ToString().c_str());
                    continue;
                }

                CommandSystem::AdjustTransition(transition, !isEnabled,
                    /*sourceNode=*/AZStd::nullopt, /*targetNode=*/AZStd::nullopt,
                    /*startOffsetXY=*/AZStd::nullopt, AZStd::nullopt, /*endOffsetXY=*/AZStd::nullopt, AZStd::nullopt,
                    /*attributesString=*/AZStd::nullopt, /*serializedMembers=*/AZStd::nullopt,
                    &commandGroup);
            }

            AZStd::string resultString;
            if (!GetCommandManager()->ExecuteCommandGroup(commandGroup, resultString))
            {
                if (resultString.size() > 0)
                {
                    MCore::LogError(resultString.c_str());
                }
            }
        }
    }


    void BlendGraphWidget::OnContextMenuEvent(QPoint mousePos, QPoint globalMousePos, const AnimGraphActionFilter& actionFilter)
    {
        if (!mAllowContextMenu)
        {
            return;
        }

        NodeGraph* nodeGraph = GetActiveGraph();
        if (!nodeGraph)
        {
            return;
        }

        // Early out in case we're adjusting or creating a new connection. Elsewise the user can open the context menu and
        // delete selected nodes while creating a new connection.
        if (nodeGraph->GetIsCreatingConnection() || nodeGraph->GetIsRelinkingConnection() ||
            nodeGraph->GetRepositionedTransitionHead() || nodeGraph->GetRepositionedTransitionTail())
        {
            return;
        }

        mContextMenuEventMousePos = mousePos;
        const AZStd::vector<EMotionFX::AnimGraphNode*> selectedAnimGraphNodes = nodeGraph->GetSelectedAnimGraphNodes();

        const AZStd::vector<NodeConnection*> selectedConnections = nodeGraph->GetSelectedNodeConnections();
        const QPoint globalPos = LocalToGlobal(mousePos);
        const bool mouseOverAnySelectedConnection = AZStd::any_of(selectedConnections.begin(), selectedConnections.end(), [globalPos](NodeConnection* connection)
            {
                return connection->CheckIfIsCloseTo(globalPos);
            });

        if (selectedAnimGraphNodes.empty() && !selectedConnections.empty() && mouseOverAnySelectedConnection)
        {
            QMenu menu(this);

            QString removeConnectionActionName;
            const QString pluralPostfix = selectedConnections.size() == 1 ? "" : "s";

            // Handle transitions in case the node graph is representing a state machine.
            if (CheckIfIsStateMachine())
            {
                removeConnectionActionName = QString("Remove transition%1").arg(pluralPostfix);

                bool hasDisabledConnection = false;
                bool hasEnabledConnection = false;
                for (NodeConnection* connection : selectedConnections)
                {
                    if (connection->GetIsDisabled())
                    {
                        hasDisabledConnection = true;
                    }
                    else
                    {
                        hasEnabledConnection = true;
                    }
                }

                // Show enable transitions menu entry in case there is at least one disabled transition in the selected ones.
                if (actionFilter.m_editNodes && hasDisabledConnection)
                {
                    QAction* enableConnectionAction = menu.addAction(QString("Enable transition%1").arg(pluralPostfix));
                    connect(enableConnectionAction, &QAction::triggered, this, &BlendGraphWidget::EnableSelectedTransitions);
                }

                if (actionFilter.m_editNodes && hasEnabledConnection)
                {
                    QAction* disableConnectionAction = menu.addAction(QString("Disable transition%1").arg(pluralPostfix));
                    connect(disableConnectionAction, &QAction::triggered, this, &BlendGraphWidget::DisableSelectedTransitions);
                }

                if (actionFilter.m_copyAndPaste &&
                    selectedConnections.size() == 1)
                {
                    EMotionFX::AnimGraphStateTransition* transition = FindTransitionForConnection(selectedConnections[0]);
                    if (transition)
                    {
                        mPlugin->GetAttributesWindow()->AddTransitionCopyPasteMenuEntries(&menu);
                    }
                }
            }
            // Handle blend tree connections in case the node graph is representing a blend tree.
            else
            {
                removeConnectionActionName = QString("Remove connection%1").arg(pluralPostfix);
            }

            if (actionFilter.m_delete &&
                !mActiveGraph->IsInReferencedGraph())
            {
                QAction* removeConnectionAction = menu.addAction(removeConnectionActionName);
                connect(removeConnectionAction, &QAction::triggered, this, static_cast<void (BlendGraphWidget::*)()>(&BlendGraphWidget::DeleteSelectedItems));
            }

            menu.exec(globalMousePos);
        }
        else
        {
            OnContextMenuEvent(this, mousePos, globalMousePos, mPlugin, selectedAnimGraphNodes, true, false, actionFilter);
        }
    }


    void BlendGraphWidget::mouseDoubleClickEvent(QMouseEvent* event)
    {
        if (mActiveGraph == nullptr)
        {
            return;
        }

        mDoubleClickHappened = true;
        NodeGraphWidget::mouseDoubleClickEvent(event);

        GraphNode* node = mActiveGraph->FindNode(event->pos());
        if (node)
        {
            const QModelIndex nodeModelIndex = node->GetModelIndex();
            EMotionFX::AnimGraphNode* animGraphNode = nodeModelIndex.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
            if (animGraphNode)
            {
                if (animGraphNode->GetHasVisualGraph())
                {
                    if (!node->GetIsInsideArrowRect(mMousePos))
                    {
                        mPlugin->GetAnimGraphModel().Focus(nodeModelIndex);
                    }
                }
            }
        }

        event->accept();
    }


    void BlendGraphWidget::mousePressEvent(QMouseEvent* event)
    {
        mDoubleClickHappened = false;

        NodeGraphWidget::mousePressEvent(event);
    }


    void BlendGraphWidget::mouseReleaseEvent(QMouseEvent* event)
    {
        //MCore::LogError("mouse release");

        if (mDoubleClickHappened == false)
        {
            if (event->button() == Qt::RightButton)
            {
                OnContextMenuEvent(event->pos(), event->globalPos(), mPlugin->GetActionFilter());
                //setCursor( Qt::ArrowCursor );
            }
        }

        NodeGraphWidget::mouseReleaseEvent(event);
        //setCursor( Qt::ArrowCursor );
        mDoubleClickHappened = false;
    }


    // start moving
    void BlendGraphWidget::OnMoveStart()
    {
        mMoveGroup.RemoveAllCommands();
    }


    // moved a node
    void BlendGraphWidget::OnMoveNode(GraphNode* node, int32 x, int32 y)
    {
        // build the command string
        const EMotionFX::AnimGraphNode* animGraphNode = node->GetModelIndex().data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();

        const AZStd::string moveString = AZStd::string::format("AnimGraphAdjustNode -animGraphID %i -name \"%s\" -xPos %d -yPos %d -updateAttributes false",
            animGraphNode->GetAnimGraph()->GetID(),
            animGraphNode->GetName(),
            x,
            y);

        // add it to the group
        mMoveGroup.AddCommandString(moveString);
    }


    // end moving
    void BlendGraphWidget::OnMoveEnd()
    {
        AZStd::string resultString;

        // execute the command
        if (GetCommandManager()->ExecuteCommandGroup(mMoveGroup, resultString) == false)
        {
            if (resultString.size() > 0)
            {
                MCore::LogError(resultString.c_str());
            }
        }
    }


    void BlendGraphWidget::OnSelectionModelChanged(const QItemSelection& selected, const QItemSelection& deselected)
    {
        // To avoid getting the view out of sync, we are going to collect the selected/deselected items
        // by GraphNode. We collect all the nodes by parent and then skip the GraphNodes we don't have.

        // First element of the pair is the selected list, second element is the deselected list
        using IndexListByIndex = AZStd::unordered_map<QModelIndex, AZStd::pair<QModelIndexList, QModelIndexList>, QModelIndexHash>;
        IndexListByIndex itemsByParent;

        for (const QItemSelectionRange& selectedRange : selected)
        {
            const QModelIndexList indexes = selectedRange.indexes();
            for (const QModelIndex& selectedIndex : indexes)
            {
                QModelIndex parent = selectedIndex.model()->parent(selectedIndex);
                if (selectedIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>() == AnimGraphModel::ModelItemType::CONNECTION)
                {
                    // if the item is a connection, we need send it to the graph of the parent of the node that
                    // contains the connection (the parent of the parent)
                    parent = parent.model()->parent(parent);
                }
                itemsByParent[parent].first.push_back(selectedIndex);
            }
        }
        for (const QItemSelectionRange& deselectedRange : deselected)
        {
            const QModelIndexList indexes = deselectedRange.indexes();
            for (const QModelIndex& deselectedIndex : indexes)
            {
                QModelIndex parent = deselectedIndex.model()->parent(deselectedIndex);
                if (deselectedIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>() == AnimGraphModel::ModelItemType::CONNECTION)
                {
                    // if the item is a connection, we need send it to the graph of the parent of the node that
                    // contains the connection (the parent of the parent)
                    parent = parent.model()->parent(parent);
                }
                itemsByParent[parent].second.push_back(deselectedIndex);
            }
        }
    }


    void BlendGraphWidget::ProcessFrame(bool redraw)
    {
        // get the active graph
        NodeGraph* activeGraph = GetActiveGraph();
        if (activeGraph)
        {
            activeGraph->UpdateVisualGraphFlags();
        }

        if (redraw)
        {
            update();
        }
    }


    // change the final node (node may not be nullptr)
    void BlendGraphWidget::SetVirtualFinalNode(const QModelIndex& nodeModelIndex)
    {
        if (nodeModelIndex.isValid())
        {
            const QModelIndex parent = nodeModelIndex.parent();
            NodeGraphByModelIndex::const_iterator it = m_nodeGraphByModelIndex.find(parent);
            if (it != m_nodeGraphByModelIndex.end())
            {
                EMotionFX::AnimGraphNode* parentNode = it->first.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
                if (azrtti_typeid(parentNode) == azrtti_typeid<EMotionFX::BlendTree>())
                {
                    EMotionFX::AnimGraphNode* node = nodeModelIndex.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
                    EMotionFX::BlendTree* blendTree = static_cast<EMotionFX::BlendTree*>(parentNode);

                    // update all graph node opacity values
                    it->second->RecursiveSetOpacity(blendTree->GetFinalNode(), 0.065f);
                    it->second->RecursiveSetOpacity(node, 1.0f);

                    if (node != blendTree->GetFinalNode())
                    {
                        GraphNode* graphNode = it->second->FindGraphNode(nodeModelIndex);
                        graphNode->SetBorderColor(QColor(0, 255, 0));
                    }
                }
            }
        }
    }


    // check if a connection is valid or not
    bool BlendGraphWidget::CheckIfIsCreateConnectionValid(uint32 portNr, GraphNode* portNode, NodePort* port, bool isInputPort)
    {
        MCORE_UNUSED(port);
        MCORE_ASSERT(mActiveGraph);

        GraphNode* sourceNode = mActiveGraph->GetCreateConnectionNode();
        GraphNode* targetNode = portNode;

        // don't allow connection to itself
        if (sourceNode == targetNode)
        {
            return false;
        }

        // if we're not dealing with state nodes
        if (sourceNode->GetType() != StateGraphNode::TYPE_ID || targetNode->GetType() != StateGraphNode::TYPE_ID)
        {
            // dont allow to connect an input port to another input port or output port to another output port
            if (isInputPort == mActiveGraph->GetCreateConnectionIsInputPort())
            {
                return false;
            }
        }

        // if this were states, it's all fine
        if (sourceNode->GetType() == StateGraphNode::TYPE_ID || targetNode->GetType() == StateGraphNode::TYPE_ID)
        {
            return CheckIfIsValidTransition(sourceNode, targetNode);
        }

        // check if there is already a connection in the port
        MCORE_ASSERT(portNode->GetType() == BlendTreeVisualNode::TYPE_ID);
        MCORE_ASSERT(sourceNode->GetType() == BlendTreeVisualNode::TYPE_ID);
        BlendTreeVisualNode* targetBlendNode;
        BlendTreeVisualNode* sourceBlendNode;
        uint32 sourcePortNr;
        uint32 targetPortNr;

        // make sure the input always comes from the source node
        if (isInputPort)
        {
            sourceBlendNode = static_cast<BlendTreeVisualNode*>(sourceNode);
            targetBlendNode = static_cast<BlendTreeVisualNode*>(targetNode);
            sourcePortNr    = mActiveGraph->GetCreateConnectionPortNr();
            targetPortNr    = portNr;
        }
        else
        {
            sourceBlendNode = static_cast<BlendTreeVisualNode*>(targetNode);
            targetBlendNode = static_cast<BlendTreeVisualNode*>(sourceNode);
            sourcePortNr    = portNr;
            targetPortNr    = mActiveGraph->GetCreateConnectionPortNr();
        }

        EMotionFX::AnimGraphNode::Port& sourcePort = sourceBlendNode->GetEMFXNode()->GetOutputPort(sourcePortNr);
        EMotionFX::AnimGraphNode::Port& targetPort = targetBlendNode->GetEMFXNode()->GetInputPort(targetPortNr);

        // if the port data types are not compatible, don't allow the connection
        if (sourcePort.CheckIfIsCompatibleWith(targetPort) == false)
        {
            return false;
        }

        EMotionFX::AnimGraphNode* parentNode = targetBlendNode->GetEMFXNode()->GetParentNode();
        EMotionFX::BlendTree* blendTree = static_cast<EMotionFX::BlendTree*>(parentNode);

        if (blendTree->ConnectionWillProduceCycle(sourceBlendNode->GetEMFXNode(), targetBlendNode->GetEMFXNode()))
        {
            return false;
        }

        return true;
    }

    bool BlendGraphWidget::CheckIfIsValidTransition(GraphNode* sourceState, [[maybe_unused]] GraphNode* targetState)
    {
        if (azrtti_typeid(static_cast<AnimGraphVisualNode*>(sourceState)->GetEMFXNode()) == azrtti_typeid<EMotionFX::AnimGraphExitNode>())
        {
            return false;
        }

        return true;
    }

    bool BlendGraphWidget::CheckIfIsValidTransitionSource(GraphNode* sourceState)

    {
        if (azrtti_typeid(static_cast<AnimGraphVisualNode*>(sourceState)->GetEMFXNode()) == azrtti_typeid<EMotionFX::AnimGraphExitNode>())
        {
            return false;
        }

        return true;
    }


    EMotionFX::AnimGraphStateTransition* BlendGraphWidget::FindTransitionForConnection(NodeConnection* connection) const
    {
        if (connection)
        {
            if (connection->GetModelIndex().data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>() == AnimGraphModel::ModelItemType::TRANSITION)
            {
                return connection->GetModelIndex().data(AnimGraphModel::ROLE_TRANSITION_POINTER).value<EMotionFX::AnimGraphStateTransition*>();
            }
        }
        return nullptr;
    }


    EMotionFX::BlendTreeConnection* BlendGraphWidget::FindBlendTreeConnection(NodeConnection* connection) const
    {
        if (connection)
        {
            if (connection->GetModelIndex().data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>() == AnimGraphModel::ModelItemType::CONNECTION)
            {
                return connection->GetModelIndex().data(AnimGraphModel::ROLE_CONNECTION_POINTER).value<EMotionFX::BlendTreeConnection*>();
            }
        }
        return nullptr;
    }


    // create the connection
    void BlendGraphWidget::OnCreateConnection(uint32 sourcePortNr, GraphNode* sourceNode, bool sourceIsInputPort, uint32 targetPortNr, GraphNode* targetNode, bool targetIsInputPort, const QPoint& startOffset, const QPoint& endOffset)
    {
        MCORE_UNUSED(targetIsInputPort);
        MCORE_ASSERT(mActiveGraph);

        GraphNode*  realSourceNode;
        GraphNode*  realTargetNode;
        uint32      realInputPortNr;
        uint32      realOutputPortNr;

        if (sourceIsInputPort)
        {
            realSourceNode      = targetNode;
            realTargetNode      = sourceNode;
            realOutputPortNr    = targetPortNr;
            realInputPortNr     = sourcePortNr;
        }
        else
        {
            realSourceNode      = sourceNode;
            realTargetNode      = targetNode;
            realOutputPortNr    = sourcePortNr;
            realInputPortNr     = targetPortNr;
        }

        const EMotionFX::AnimGraphNode* targetAnimGraphNode = targetNode->GetModelIndex().data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();

        MCore::CommandGroup commandGroup;
        AZStd::string command;

        // Check if there already is a connection plugged into the port where we want to put our new connection in.
        NodeConnection* existingConnection = mActiveGraph->FindInputConnection(realTargetNode, realInputPortNr);

        // Special case for state nodes.
        AZ::TypeId transitionType = AZ::TypeId::CreateNull();
        if (sourceNode->GetType() == StateGraphNode::TYPE_ID && targetNode->GetType() == StateGraphNode::TYPE_ID)
        {
            transitionType      = azrtti_typeid<EMotionFX::AnimGraphStateTransition>();
            realInputPortNr     = 0;
            realOutputPortNr    = 0;
            commandGroup.SetGroupName("Create state machine transition");
        }
        else
        {
            // Check if there already is a connection and remove it in this case.
            if (existingConnection)
            {
                commandGroup.SetGroupName("Replace blend tree connection");

                command = AZStd::string::format("AnimGraphRemoveConnection -animGraphID %i -sourceNode \"%s\" -sourcePort %d -targetNode \"%s\" -targetPort %d",
                    targetAnimGraphNode->GetAnimGraph()->GetID(),
                    existingConnection->GetSourceNode()->GetName(),
                    existingConnection->GetOutputPortNr(),
                    existingConnection->GetTargetNode()->GetName(),
                    existingConnection->GetInputPortNr());

                commandGroup.AddCommandString(command);
            }
            else
            {
                commandGroup.SetGroupName("Create blend tree connection");
            }
        }


        if (transitionType.IsNull())
        {
            command = AZStd::string::format("AnimGraphCreateConnection -animGraphID %i -sourceNode \"%s\" -targetNode \"%s\" -sourcePort %d -targetPort %d -startOffsetX %d -startOffsetY %d -endOffsetX %d -endOffsetY %d",
                targetAnimGraphNode->GetAnimGraph()->GetID(),
                realSourceNode->GetName(),
                realTargetNode->GetName(),
                realOutputPortNr,
                realInputPortNr,
                startOffset.x(),
                startOffset.y(),
                endOffset.x(),
                endOffset.y());
        }
        else
        {
            command = AZStd::string::format("AnimGraphCreateConnection -animGraphID %i -sourceNode \"%s\" -targetNode \"%s\" -sourcePort %d -targetPort %d -startOffsetX %d -startOffsetY %d -endOffsetX %d -endOffsetY %d -transitionType \"%s\"",
                targetAnimGraphNode->GetAnimGraph()->GetID(),
                realSourceNode->GetName(),
                realTargetNode->GetName(),
                realOutputPortNr,
                realInputPortNr,
                startOffset.x(),
                startOffset.y(),
                endOffset.x(),
                endOffset.y(),
                transitionType.ToString<AZStd::string>().c_str());
        }

        commandGroup.AddCommandString(command);

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }


    // curved connection when creating a new one?
    bool BlendGraphWidget::CreateConnectionMustBeCurved()
    {
        if (mActiveGraph == nullptr)
        {
            return true;
        }

        if (mActiveGraph->GetCreateConnectionNode()->GetType() == StateGraphNode::TYPE_ID)
        {
            return false;
        }

        return true;
    }


    // show helper connection suggestion lines when creating a new connection?
    bool BlendGraphWidget::CreateConnectionShowsHelpers()
    {
        if (mActiveGraph == nullptr)
        {
            return true;
        }

        if (mActiveGraph->GetCreateConnectionNode()->GetType() == StateGraphNode::TYPE_ID)
        {
            return false;
        }

        return true;
    }


    // delete all selected items
    void BlendGraphWidget::DeleteSelectedItems()
    {
        DeleteSelectedItems(GetActiveGraph());
    }


    void BlendGraphWidget::DeleteSelectedItems(NodeGraph* nodeGraph)
    {
        if (!nodeGraph)
        {
            return;
        }

        // Do not allow to delete nodes or connections when creating or relinking connections or transitions.
        // In this case the delete operation will cancel the create or relink operation.
        if (nodeGraph->GetIsCreatingConnection() || nodeGraph->GetIsRelinkingConnection() ||
            nodeGraph->GetRepositionedTransitionHead() || nodeGraph->GetRepositionedTransitionTail())
        {
            nodeGraph->StopCreateConnection();
            nodeGraph->StopRelinkConnection();
            nodeGraph->StopReplaceTransitionHead();
            nodeGraph->StopReplaceTransitionTail();
            return;
        }

        MCore::CommandGroup commandGroup("Delete selected anim graph items");

        AZStd::vector<EMotionFX::BlendTreeConnection*> connectionList;
        AZStd::vector<EMotionFX::AnimGraphStateTransition*> transitionList;
        AZStd::vector<EMotionFX::AnimGraphNode*> nodeList;
        connectionList.reserve(256);
        transitionList.reserve(256);
        nodeList.reserve(256);

        // Delete all selected connections in the graph view first.
        AZStd::string commandString, sourceNodeName;
        AZ::u32 numDeletedConnections = 0;
        const AZStd::vector<NodeConnection*> selectedConnections = GetActiveGraph()->GetSelectedNodeConnections();
        for (NodeConnection* selectedConnection : selectedConnections)
        {
            EMotionFX::AnimGraphStateTransition* emfxTransition = FindTransitionForConnection(selectedConnection);
            if (emfxTransition)
            {
                CommandSystem::DeleteStateTransition(&commandGroup, emfxTransition, transitionList);
                numDeletedConnections++;
            }
            else
            {
                EMotionFX::BlendTreeConnection* emfxConnection = FindBlendTreeConnection(selectedConnection);
                EMotionFX::AnimGraphNode* emfxTargetNode = selectedConnection->GetTargetNode()->GetModelIndex().data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();

                if (emfxConnection && emfxTargetNode)
                {
                    CommandSystem::DeleteConnection(&commandGroup, emfxTargetNode, emfxConnection, connectionList);
                    numDeletedConnections++;
                }
            }
        }

        // Prepare the list of nodes to remove.
        AZStd::vector<AZStd::string> selectedNodeNames;
        const AZStd::vector<GraphNode*> selectedNodes = GetActiveGraph()->GetSelectedGraphNodes();
        for (GraphNode* graphNode : selectedNodes)
        {
            selectedNodeNames.emplace_back(graphNode->GetName());
        }

        EMotionFX::AnimGraphNode* parentNode = GetActiveGraph()->GetModelIndex().data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
        CommandSystem::DeleteNodes(&commandGroup, parentNode->GetAnimGraph(), selectedNodeNames, nodeList, connectionList, transitionList);

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }


    // some node collapsed
    void BlendGraphWidget::OnNodeCollapsed(GraphNode* node, bool isCollapsed)
    {
        if (node->GetType() == BlendTreeVisualNode::TYPE_ID)
        {
            BlendTreeVisualNode* blendNode = static_cast<BlendTreeVisualNode*>(node);
            blendNode->GetEMFXNode()->SetIsCollapsed(isCollapsed);
        }
    }


    // left-clicked on a node while shift pressed
    void BlendGraphWidget::OnShiftClickedNode(GraphNode* node)
    {
        // when we are dealing with a state node
        if (node->GetType() == StateGraphNode::TYPE_ID)
        {
            EMotionFX::AnimGraphInstance* animGraphInstance = GetActiveGraph()->GetModelIndex().data(AnimGraphModel::ROLE_ANIM_GRAPH_INSTANCE).value<EMotionFX::AnimGraphInstance*>();
            if (animGraphInstance)
            {
                animGraphInstance->TransitionToState(node->GetName());
            }
        }
    }


    void BlendGraphWidget::OnNodeGroupSelected()
    {
        assert(sender()->inherits("QAction"));
        QAction* action = qobject_cast<QAction*>(sender());

        // find the selected node
        const QItemSelection selection = mPlugin->GetAnimGraphModel().GetSelectionModel().selection();
        const QModelIndexList selectionList = selection.indexes();
        if (selectionList.empty())
        {
            return;
        }

        const EMotionFX::AnimGraphNode* parentNode = GetActiveGraph()->GetModelIndex().data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
        EMotionFX::AnimGraph* animGraph = parentNode->GetAnimGraph();
        if (!animGraph)
        {
            return;
        }

        // get the node group name from the action and search the node group
        EMotionFX::AnimGraphNodeGroup* newNodeGroup;
        if (action->data().toInt() == 0)
        {
            newNodeGroup = nullptr;
        }
        else
        {
            const AZStd::string nodeGroupName = FromQtString(action->text());
            newNodeGroup = animGraph->FindNodeGroupByName(nodeGroupName.c_str());
        }

        MCore::CommandGroup commandGroup("Adjust anim graph node group");

        AZStd::string nodeNames;
        for (const QModelIndex& selectedIndex : selectionList)
        {
            // Skip transitions and blend tree connections.
            if (selectedIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>() != AnimGraphModel::ModelItemType::NODE)
            {
                continue;
            }

            EMotionFX::AnimGraphNode* selectedNode = selectedIndex.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
            EMotionFX::AnimGraphNodeGroup* nodeGroup = animGraph->FindNodeGroupForNode(selectedNode);
            if (nodeGroup)
            {
                const AZStd::string command = AZStd::string::format("AnimGraphAdjustNodeGroup -animGraphID %i -name \"%s\" -nodeNames \"%s\" -nodeAction \"remove\"", animGraph->GetID(), nodeGroup->GetName(), selectedNode->GetName());
                commandGroup.AddCommandString(command);
            }

            nodeNames += selectedNode->GetName();
            nodeNames += ";";
        }
        if (!nodeNames.empty())
        {
            nodeNames.pop_back();
        }

        if (newNodeGroup)
        {
            const AZStd::string command = AZStd::string::format("AnimGraphAdjustNodeGroup -animGraphID %i -name \"%s\" -nodeNames \"%s\" -nodeAction \"add\"", animGraph->GetID(), newNodeGroup->GetName(), nodeNames.c_str());
            commandGroup.AddCommandString(command);
        }

        AZStd::string outResult;
        if (!GetCommandManager()->ExecuteCommandGroup(commandGroup, outResult))
        {
            AZ_Error("EMotionFX", false, outResult.c_str());
        }
    }


    bool BlendGraphWidget::PreparePainting()
    {
        // skip rendering in case rendering is disabled
        if (mPlugin->GetDisableRendering())
        {
            return false;
        }

        if (mActiveGraph)
        {
            // enable or disable graph animation
            mActiveGraph->SetUseAnimation(mPlugin->GetAnimGraphOptions().GetGraphAnimation());
        }

        // pass down the show fps options flag
        NodeGraphWidget::SetShowFPS(mPlugin->GetAnimGraphOptions().GetShowFPS());

        return true;
    }


    // enable or disable node visualization
    void BlendGraphWidget::OnVisualizeToggle(GraphNode* node, bool visualizeEnabled)
    {
        //if (node->GetType() == BlendTreeVisualNode::TYPE_ID)
        {
            BlendTreeVisualNode* blendNode = static_cast<BlendTreeVisualNode*>(node);
            blendNode->GetEMFXNode()->SetVisualization(visualizeEnabled);
        }
    }


    // enable or disable the node
    void BlendGraphWidget::OnEnabledToggle(GraphNode* node, bool enabled)
    {
        if (node->GetType() == BlendTreeVisualNode::TYPE_ID)
        {
            BlendTreeVisualNode* blendNode = static_cast<BlendTreeVisualNode*>(node);
            blendNode->GetEMFXNode()->SetIsEnabled(enabled);
        }
    }


    // change visualize options
    void BlendGraphWidget::OnSetupVisualizeOptions(GraphNode* node)
    {
        BlendTreeVisualNode* blendNode = static_cast<BlendTreeVisualNode*>(node);
        mPlugin->GetActionManager().ShowNodeColorPicker(blendNode->GetEMFXNode());
    }


    bool BlendGraphWidget::event(QEvent* event)
    {
        if (event->type() == QEvent::ToolTip)
        {
            QHelpEvent* helpEvent = static_cast<QHelpEvent*>(event);

            QFontMetrics fontMetrics(QToolTip::font());

            QFont boldFont = QToolTip::font();
            boldFont.setBold(true);
            QFontMetrics boldFontMetrics(boldFont);

            if (mActiveGraph)
            {
                AZStd::string toolTipString;

                QPoint localPos     = helpEvent->pos();
                QPoint globalPos    = LocalToGlobal(localPos);
                QPoint tooltipPos   = helpEvent->globalPos();

                // find the connection at the mouse position
                NodeConnection* connection = mActiveGraph->FindConnection(globalPos);
                if (connection)
                {
                    bool conditionFound = false;
                    if (connection->GetType() == StateConnection::TYPE_ID)
                    {
                        StateConnection* stateConnection = static_cast<StateConnection*>(connection);
                        EMotionFX::AnimGraphTransitionCondition* condition = stateConnection->FindCondition(globalPos);
                        if (condition)
                        {
                            AZStd::string tempConditionString;
                            condition->GetTooltip(&tempConditionString);

                            toolTipString = "<qt>";
                            toolTipString += tempConditionString;
                            toolTipString += "</qt>";

                            conditionFound = true;
                        }
                    }

                    // get the output and the input port numbers
                    const uint32 outputPortNr   = connection->GetOutputPortNr();
                    const uint32 inputPortNr    = connection->GetInputPortNr();

                    // show connection or state transition tooltip
                    if (conditionFound == false)
                    {
                        GraphNode* sourceNode = connection->GetSourceNode();
                        GraphNode* targetNode = connection->GetTargetNode();

                        // prepare the colors
                        QColor sourceColor, targetColor;
                        if (sourceNode)
                        {
                            sourceColor = sourceNode->GetBaseColor();
                        }
                        if (targetNode)
                        {
                            targetColor = targetNode->GetBaseColor();
                        }

                        // prepare the node names
                        AZStd::string sourceNodeName, targetNodeName;
                        if (sourceNode)
                        {
                            sourceNodeName = sourceNode->GetName();
                        }
                        if (targetNode)
                        {
                            targetNodeName = targetNode->GetName();
                        }
                        //sourceNodeName.ConvertToNonBreakingHTMLSpaces();
                        //targetNodeName.ConvertToNonBreakingHTMLSpaces();

                        // check if we are dealing with a node inside a blend tree
                        if (sourceNode && sourceNode->GetType() == BlendTreeVisualNode::TYPE_ID)
                        {
                            // type cast it to a blend graph node and get the corresponding emfx node
                            BlendTreeVisualNode* blendSourceNode = static_cast<BlendTreeVisualNode*>(sourceNode);
                            EMotionFX::AnimGraphNode* sourceEMFXNode = blendSourceNode->GetEMFXNode();

                            // prepare the port names
                            AZStd::string outputPortName, inputPortName;
                            if (sourceNode)
                            {
                                outputPortName = sourceNode->GetOutputPort(outputPortNr)->GetName();
                            }
                            if (targetNode)
                            {
                                inputPortName = targetNode->GetInputPort(inputPortNr)->GetName();
                            }
                            //outputPortName.ConvertToNonBreakingHTMLSpaces();
                            //inputPortName.ConvertToNonBreakingHTMLSpaces();

                            int columnSourceWidth = boldFontMetrics.horizontalAdvance(sourceNodeName.c_str()) + boldFontMetrics.horizontalAdvance(" ") + fontMetrics.horizontalAdvance("(Port: ") +  fontMetrics.horizontalAdvance(outputPortName.c_str()) + fontMetrics.horizontalAdvance(")");
                            int columnTargetWidth = boldFontMetrics.horizontalAdvance(targetNodeName.c_str()) + boldFontMetrics.horizontalAdvance(" ") + fontMetrics.horizontalAdvance("(Port: ") +  fontMetrics.horizontalAdvance(inputPortName.c_str()) + fontMetrics.horizontalAdvance(")");

                            // construct the html tooltip string
                            toolTipString += AZStd::string::format("<qt><table border=\"0\"><tr><td width=\"%i\"><p style=\"color:rgb(%i,%i,%i)\"><b>%s </b>(Port: %s)</p></td> <td>to</td> <td width=\"%i\"><p style=\"color:rgb(%i,%i,%i)\"><b>%s </b>(Port: %s)</p></td></tr>", columnSourceWidth, sourceColor.red(), sourceColor.green(), sourceColor.blue(), sourceNodeName.c_str(), outputPortName.c_str(), columnTargetWidth, targetColor.red(), targetColor.green(), targetColor.blue(), targetNodeName.c_str(), inputPortName.c_str());

                            // now check if the connection is coming from a parameter node
                            if (azrtti_typeid(sourceEMFXNode) == azrtti_typeid<EMotionFX::BlendTreeParameterNode>())
                            {
                                EMotionFX::BlendTreeParameterNode* parameterNode = static_cast<EMotionFX::BlendTreeParameterNode*>(sourceEMFXNode);

                                // get the parameter index from the port where the connection starts
                                uint32 parameterIndex = parameterNode->GetParameterIndex(outputPortNr);
                                if (parameterIndex != MCORE_INVALIDINDEX32)
                                {
                                    // get access to the parameter name and add it to the tool tip
                                    EMotionFX::AnimGraph* animGraph = parameterNode->GetAnimGraph();
                                    const EMotionFX::Parameter* parameter = animGraph->FindValueParameter(parameterIndex);

                                    toolTipString += "\n<qt><table border=\"0\"><tr>";
                                    toolTipString += AZStd::string::format("<td><p style=\"color:rgb(80, 80, 80)\"><b>Parameter:</b></p></td><td><p style=\"color:rgb(115, 115, 115)\">%s</p></td>", parameter->GetName().c_str());
                                    toolTipString += "</tr></table></qt>";
                                }
                            }
                        }
                        // state machine node
                        else
                        {
                            toolTipString = "<qt><table><tr>";

                            // construct the html tooltip string
                            if (sourceNode && targetNode)
                            {
                                toolTipString += AZStd::string::format("<td width=\"%i\"><b><p style=\"color:rgb(%i,%i,%i)\">%s</p></b></td> <td>to</td> <td width=\"%i\"><b><nobr><p style=\"color:rgb(%i,%i,%i)\">%s</p></nobr></b></td>", boldFontMetrics.horizontalAdvance(sourceNodeName.c_str()), sourceColor.red(), sourceColor.green(), sourceColor.blue(), sourceNodeName.c_str(), boldFontMetrics.horizontalAdvance(targetNodeName.c_str()), targetColor.red(), targetColor.green(), targetColor.blue(), targetNodeName.c_str());
                            }
                            else if (targetNode)
                            {
                                toolTipString += AZStd::string::format("<td>to</td> <td width=\"%i\"><b><p style=\"color:rgb(%i,%i,%i)\">%s</p></b></td>", boldFontMetrics.horizontalAdvance(targetNodeName.c_str()), targetColor.red(), targetColor.green(), targetColor.blue(), targetNodeName.c_str());
                            }

                            toolTipString += "</tr></table></qt>";
                        }
                    }
                }

                GraphNode*                  node            = mActiveGraph->FindNode(localPos);
                EMotionFX::AnimGraphNode*  animGraphNode  = nullptr;

                if (node)
                {
                    BlendTreeVisualNode* blendNode   = static_cast<BlendTreeVisualNode*>(node);
                    animGraphNode              = blendNode->GetEMFXNode();

                    AZStd::string tempString;
                    toolTipString = "<qt><table border=\"0\">";

                    // node name
                    toolTipString += AZStd::string::format("<tr><td><b>Name:</b></td><td><nobr>%s</nobr></td></tr>", animGraphNode->GetName());

                    // node palette name
                    toolTipString += AZStd::string::format("<tr><td><b>Type:</b></td><td><nobr>%s</nobr></td></tr>", animGraphNode->GetPaletteName());

                    if (animGraphNode->GetCanHaveChildren())
                    {
                        // child nodes
                        toolTipString += AZStd::string::format("<tr><td><b><nobr>Child Nodes:</nobr></b></td><td>%i</td></tr>", animGraphNode->GetNumChildNodes());

                        // recursive child nodes
                        toolTipString += AZStd::string::format("<tr><td width=\"140\"><b><nobr>Recursive Child Nodes:</nobr></b></td><td>%i</td></tr>", animGraphNode->RecursiveCalcNumNodes());
                    }

                    // states
                    if (node->GetType() == StateGraphNode::TYPE_ID)
                    {
                        // get access to the state machine
                        EMotionFX::AnimGraphStateMachine* stateMachine = nullptr;
                        EMotionFX::AnimGraphNode* parentNode = animGraphNode->GetParentNode();
                        if (parentNode && azrtti_typeid(parentNode) == azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
                        {
                            stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(parentNode);
                        }

                        // incoming transitions
                        toolTipString += AZStd::string::format("<tr><td><b>Incoming Transitions:</b></td><td>%i</td></tr>", stateMachine->CalcNumIncomingTransitions(animGraphNode));

                        // outgoing transitions
                        toolTipString += AZStd::string::format("<tr><td width=\"130\"><b>Outgoing Transitions:</b></td><td>%i</td></tr>", stateMachine->CalcNumOutgoingTransitions(animGraphNode));
                    }

                    // complete the table
                    toolTipString += "</table></qt>";
                }

                if (!toolTipString.empty())
                {
                    QRect toolTipRect(globalPos.x() - 4, globalPos.y() - 4, 8, 8);
                    QToolTip::showText(tooltipPos, toolTipString.c_str(), this, toolTipRect);
                }

                return NodeGraphWidget::event(event);
            }
        }

        return NodeGraphWidget::event(event);
    }


    void BlendGraphWidget::ReplaceTransition(NodeConnection* connection, QPoint oldStartOffset, QPoint oldEndOffset, GraphNode* oldSourceNode, GraphNode* oldTargetNode, GraphNode* newSourceNode, GraphNode* newTargetNode)
    {
        if (connection->GetType() != StateConnection::TYPE_ID)
        {
            return;
        }

        StateConnection* stateConnection = static_cast<StateConnection*>(connection);
        EMotionFX::AnimGraphStateTransition* transition = connection->GetModelIndex().data(AnimGraphModel::ROLE_TRANSITION_POINTER).value<EMotionFX::AnimGraphStateTransition*>();

        AZStd::optional<AZStd::string> newSourceNodeName = AZStd::nullopt;
        if (newSourceNode)
        {
            newSourceNodeName = newSourceNode->GetNameString();
        }
        AZStd::optional<AZStd::string> newTargetNodeName = AZStd::nullopt;
        if (newTargetNode)
        {
            newTargetNodeName = newTargetNode->GetNameString();
        }
        const AZ::s32 newStartOffsetX = transition->GetVisualStartOffsetX();
        const AZ::s32 newStartOffsetY = transition->GetVisualStartOffsetY();
        const AZ::s32 newEndOffsetX = transition->GetVisualEndOffsetX();
        const AZ::s32 newEndOffsetY = transition->GetVisualEndOffsetY();

        mActiveGraph->StopReplaceTransitionHead();
        mActiveGraph->StopReplaceTransitionTail();

        // Reset the visual transition before calling the actual command so that undo captures the right previous values.
        stateConnection->SetSourceNode(oldSourceNode);
        stateConnection->SetTargetNode(oldTargetNode);
        transition->SetVisualOffsets(oldStartOffset.x(), oldStartOffset.y(), oldEndOffset.x(), oldEndOffset.y());

        if (mActiveGraph->GetReplaceTransitionValid())
        {
            CommandSystem::AdjustTransition(transition,
                /*isDisabled=*/AZStd::nullopt,
                newSourceNodeName, newTargetNodeName,
                newStartOffsetX, newStartOffsetY,
                newEndOffsetX, newEndOffsetY);
        }
    }


    void BlendGraphWidget::OnRowsInserted(const QModelIndex& parent, int first, int last)
    {
        // Here we could be receiving connections, transitions or nodes being inserted into
        // the model.
        // For nodes, we need to locate the parent NodeGraph and insert elements.
        // For connections, we need to locate the parent Node. With the parent node, we can
        //      locate the parent NodeGraph
        // For transitions, the parent index is the state machine therefore we can locate
        //      the parent NodeGraph directly.
        // So, only for connections we need to do something special. For transitions and nodes
        // we just locate the NodeGraph through the parent index. For connections we locate the
        // parent index of "parent" and then use that to locate the NodeGraph.
        // We only update if we have the NodeGraph cached, if the NodeGraph is not cached, next
        // time that we focus on it, it will create the whole structure.
        if (parent.isValid())
        {
            const QModelIndex grandParent = parent.model()->parent(parent);
            NodeGraphByModelIndex::iterator itGrandParent = m_nodeGraphByModelIndex.find(grandParent);
            NodeGraphByModelIndex::iterator itParent = m_nodeGraphByModelIndex.find(parent);
            if (itGrandParent == m_nodeGraphByModelIndex.end() && itParent == m_nodeGraphByModelIndex.end())
            {
                return; // early out if we dont have any of those cached
            }

            QModelIndexList modelIndexesForGrandParent;
            QModelIndexList modelIndexesForParent;
            for (int row = first; row <= last; ++row)
            {
                const QModelIndex childModelIndex = parent.model()->index(row, 0, parent);
                const AnimGraphModel::ModelItemType type = childModelIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>();
                switch (type)
                {
                case AnimGraphModel::ModelItemType::NODE:
                case AnimGraphModel::ModelItemType::TRANSITION:
                default:
                    modelIndexesForParent.push_back(childModelIndex);
                    break;
                case AnimGraphModel::ModelItemType::CONNECTION:
                    modelIndexesForGrandParent.push_back(childModelIndex);
                    break;
                }
            }

            if (!modelIndexesForGrandParent.empty() && itGrandParent != m_nodeGraphByModelIndex.end())
            {
                itGrandParent->second->OnRowsInserted(modelIndexesForGrandParent);
            }
            if (!modelIndexesForParent.empty() && itParent != m_nodeGraphByModelIndex.end())
            {
                itParent->second->OnRowsInserted(modelIndexesForParent);
            }
        }
    }

    void BlendGraphWidget::OnRowsAboutToBeRemoved(const QModelIndex& parent, int first, int last)
    {
        // Remove the graphs, if it is not in our cache, then it is not removed, if it is, its removed
        if (parent.isValid())
        {
            const QModelIndex grandParent = parent.model()->parent(parent);
            NodeGraphByModelIndex::iterator itGrandParent = m_nodeGraphByModelIndex.find(grandParent);
            NodeGraphByModelIndex::iterator itParent = m_nodeGraphByModelIndex.find(parent);
            if (itGrandParent == m_nodeGraphByModelIndex.end() && itParent == m_nodeGraphByModelIndex.end())
            {
                return; // early out if we dont have any of those cached
            }

            QModelIndexList modelIndexesForGrandParent;
            QModelIndexList modelIndexesForParent;
            for (int row = first; row <= last; ++row)
            {
                const QModelIndex childModelIndex = parent.model()->index(row, 0, parent);
                const AnimGraphModel::ModelItemType type = childModelIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>();
                switch (type)
                {
                case AnimGraphModel::ModelItemType::NODE:
                case AnimGraphModel::ModelItemType::TRANSITION:
                default:
                    modelIndexesForParent.push_back(childModelIndex);
                    break;
                case AnimGraphModel::ModelItemType::CONNECTION:
                    modelIndexesForGrandParent.push_back(childModelIndex);
                    break;
                }
            }

            if (!modelIndexesForGrandParent.empty() && itGrandParent != m_nodeGraphByModelIndex.end())
            {
                itGrandParent->second->OnRowsAboutToBeRemoved(modelIndexesForGrandParent);
            }
            if (!modelIndexesForParent.empty() && itParent != m_nodeGraphByModelIndex.end())
            {
                itParent->second->OnRowsAboutToBeRemoved(modelIndexesForParent);
            }
            // Check if we have any node graph stored for those nodes
            for (const QModelIndex& modelIndex : modelIndexesForParent)
            {
                m_nodeGraphByModelIndex.erase(modelIndex);
            }
        }
    }

    void BlendGraphWidget::OnDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
    {
        const QItemSelectionRange range(topLeft, bottomRight);
        const QModelIndexList changedIndexes = range.indexes();
        for (const QModelIndex& changed : changedIndexes)
        {
            QModelIndex parentGraph = changed.model()->parent(changed);

            const AnimGraphModel::ModelItemType itemType = changed.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>();
            if (itemType == AnimGraphModel::ModelItemType::CONNECTION)
            {
                parentGraph = parentGraph.model()->parent(parentGraph);
            }

            NodeGraphByModelIndex::iterator itParent = m_nodeGraphByModelIndex.find(parentGraph);
            if (itParent != m_nodeGraphByModelIndex.end()) // otherwise we dont have the graph cached
            {
                itParent->second->OnDataChanged(changed, roles);
            }
        }
    }

    void BlendGraphWidget::OnFocusChanged(const QModelIndex& newFocusIndex, const QModelIndex& newFocusParent, const QModelIndex& oldFocusIndex, const QModelIndex& oldFocusParent)
    {
        AZ_UNUSED(oldFocusIndex);

        if (newFocusParent.isValid())
        {
            if (newFocusParent != oldFocusParent)
            {
                // Parent changed, we need to dive into that parent
                AZStd::pair<NodeGraphByModelIndex::iterator, bool> inserted = m_nodeGraphByModelIndex.emplace(newFocusParent, AZStd::make_unique<NodeGraph>(newFocusParent, this));
                NodeGraph& nodeGraph = *inserted.first->second;
                if (inserted.second)
                {
                    nodeGraph.Reinit();
                }
                SetActiveGraph(&nodeGraph);
            }

            if (newFocusIndex != newFocusParent)
            {
                // We are focusing on a node inside a blendtree/statemachine/referencenode
                GraphNode* graphNode = mActiveGraph->FindGraphNode(newFocusIndex);
                mActiveGraph->ZoomOnRect(graphNode->GetRect(), geometry().width(), geometry().height(), true);
            }
        }
        else
        {
            SetActiveGraph(nullptr);
        }
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/moc_BlendGraphWidget.cpp>
