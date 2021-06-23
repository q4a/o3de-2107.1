/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef __EMSTUDIO_MOTIONRETARGETINGWINDOW_H
#define __EMSTUDIO_MOTIONRETARGETINGWINDOW_H

// include MCore
#if !defined(Q_MOC_RUN)
#include "../StandardPluginsConfig.h"
#include <MCore/Source/StandardHeaders.h>
#include <EMotionFX/Source/PlayBackInfo.h>
#include "../../../../EMStudioSDK/Source/NodeSelectionWindow.h"
#include <EMotionFX/CommandSystem/Source/SelectionCommands.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#endif

QT_FORWARD_DECLARE_CLASS(QPushButton)

namespace EMStudio
{
    // forward declarations
    class MotionWindowPlugin;

    class MotionRetargetingWindow
        : public QWidget
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(MotionRetargetingWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        MotionRetargetingWindow(QWidget* parent, MotionWindowPlugin* motionWindowPlugin);
        ~MotionRetargetingWindow();

        void Init();

    public slots:
        void UpdateInterface();
        void UpdateMotions();

    private:
        MotionWindowPlugin*                 mMotionWindowPlugin;
        QCheckBox*                          mMotionRetargetingButton;
        //QCheckBox*                          mRenderMotionBindPose;
        EMotionFX::ActorInstance*           mSelectedActorInstance;
        EMotionFX::Actor*                   mActor;
        CommandSystem::SelectionList        mSelectionList;
    };
} // namespace EMStudio


#endif
