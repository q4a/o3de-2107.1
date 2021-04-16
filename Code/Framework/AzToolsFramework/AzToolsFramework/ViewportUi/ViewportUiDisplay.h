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

#include <AzToolsFramework/ViewportUi/Button.h>
#include <AzToolsFramework/ViewportUi/Cluster.h>
#include <AzToolsFramework/ViewportUi/TextField.h>
#include <AzToolsFramework/ViewportUi/ViewportUiRequestBus.h>
#include <AzToolsFramework/ViewportUi/ViewportUiDisplayLayout.h>
#include <QMainWindow>
#include <QPointer>
#include <QLabel>

AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option")
#include <QGridLayout>
AZ_POP_DISABLE_WARNING

class QPoint;

namespace AzToolsFramework::ViewportUi::Internal
{
    //! Used to track info for each widget in the Viewport UI.
    struct ViewportUiElementInfo
    {
        AZStd::shared_ptr<QWidget> m_widget; //<! Reference to the widget.
        ViewportUiElementId m_viewportUiElementId; //<! Corresponding ViewportUiElementId of the widget.
        bool m_anchored = true; //<! Whether the widget is anchored to one position or moves with camera/entity.
        AZ::Vector3 m_worldPosition; //<! If not anchored, use this to project widget position to screen space.

        bool IsValid() const
        {
            return m_viewportUiElementId != InvalidViewportUiElementId;
        }
    };

    using ViewportUiElementIdInfoLookup = AZStd::unordered_map<ViewportUiElementId, ViewportUiElementInfo>;

    //! Helper function to give a widget a transparent background
    void SetTransparentBackground(QWidget* widget);

    //! Creates a transparent widget over a viewport render overlay, and adds/manages other Qt widgets
    //! to display on top of the viewport.
    class ViewportUiDisplay
    {
    public:
        ViewportUiDisplay(QWidget* parent, QWidget* renderOverlay);
        ~ViewportUiDisplay();

        void AddCluster(AZStd::shared_ptr<Cluster> cluster);
        void AddClusterButton(ViewportUiElementId clusterId, Button* button);
        void RemoveClusterButton(ViewportUiElementId clusterId, ButtonId buttonId);
        void UpdateCluster(const ViewportUiElementId clusterId);

        void AddSwitcher(AZStd::shared_ptr<Cluster> cluster, ButtonId currMode);
        void AddSwitcherButton(ViewportUiElementId clusterId, Button* button);
        void RemoveSwitcherButton(ViewportUiElementId clusterId, ButtonId buttonId);
        void UpdateSwitcher(const ViewportUiElementId clusterId);
        void SetSwitcherActiveMode(ViewportUiElementId clusterId, ButtonId buttonId);

        void AddTextField(AZStd::shared_ptr<TextField> textField);
        void UpdateTextField(ViewportUiElementId textFieldId);

        //! After removing, can no longer be accessed by its ViewportUiElementId unless it is re-added.
        void RemoveViewportUiElement(ViewportUiElementId elementId);

        //! Moves the Viewport UI over the Render Overlay, projects new positions of non-anchored elements,
        //! and sets Viewport UI geometry to include only areas populated by Viewport UI Elements.
        void Update();

        const QMainWindow* GetUiMainWindow() const;
        const QWidget* GetUiOverlay() const;
        const QGridLayout* GetUiOverlayLayout() const;

        //! Initializes UI main window and overlay by setting attributes such as transparency and visibility.
        void InitializeUiOverlay();

        void ShowViewportUiElement(ViewportUiElementId elementId);
        void HideViewportUiElement(ViewportUiElementId elementId);

        AZStd::shared_ptr<QWidget> GetViewportUiElement(ViewportUiElementId elementId);
        bool IsViewportUiElementVisible(ViewportUiElementId elementId);

        void CreateComponentModeBorder(const AZStd::string& borderTitle);
        void RemoveComponentModeBorder();
        
    private:
        void PrepareWidgetForViewportUi(QPointer<QWidget> widget);

        ViewportUiElementId AddViewportUiElement(AZStd::shared_ptr<QWidget> widget);
        ViewportUiElementId GetViewportUiElementId(QPointer<QWidget> widget);
        void AddMaximumSizeViewportUiElement(QPointer<QWidget> widget);

        void PositionViewportUiElementFromWorldSpace(ViewportUiElementId elementId, const AZ::Vector3& pos);
        void PositionViewportUiElementAnchored(ViewportUiElementId elementId, const Qt::Alignment alignment);
        void PositionUiOverlayOverRenderViewport();

        bool UiDisplayEnabled() const;
        void SetUiOverlayContents(QPointer<QWidget> widget);
        void SetUiOverlayContentsAnchored(QPointer<QWidget>, Qt::Alignment aligment);
        void UpdateUiOverlayGeometry();

        ViewportUiElementInfo GetViewportUiElementInfo(const ViewportUiElementId elementId);

        QMainWindow m_uiMainWindow; //<! The window which contains the UI Overlay.
        QWidget m_uiOverlay; //<! The UI Overlay which displays Viewport UI Elements.
        QGridLayout m_fullScreenLayout; //<! The layout which extends across the full screen.
        ViewportUiDisplayLayout m_uiOverlayLayout; //<! The layout used for optionally anchoring Viewport UI Elements.
        QLabel m_componentModeBorderText; //<! The text used for the Component Mode border.

        QWidget* m_renderOverlay;
        QPointer<QWidget> m_fullScreenWidget; //<! Reference to the widget attached to m_fullScreenLayout if any.
        int m_viewportId;
        int64_t m_numViewportElements = 0; 

        ViewportUiElementIdInfoLookup m_viewportUiElements;
    };
} // namespace AzToolsFramework::ViewportUi::Internal
