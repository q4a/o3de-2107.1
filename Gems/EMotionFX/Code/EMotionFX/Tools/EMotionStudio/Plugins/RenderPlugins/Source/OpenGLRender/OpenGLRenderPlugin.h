/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <EMotionFX/Rendering/OpenGL2/Source/glactor.h>
#include <EMotionFX/Rendering/OpenGL2/Source/GraphicsManager.h>
#include <EMotionFX/Rendering/OpenGL2/Source/GLRenderUtil.h>
#include "../../../../EMStudioSDK/Source/RenderPlugin/RenderPlugin.h"
#include "../../../../EMStudioSDK/Source/RenderPlugin/RenderOptions.h"
#include "../../../../EMStudioSDK/Source/RenderPlugin/RenderWidget.h"
#include "../../../../EMStudioSDK/Source/RenderPlugin/RenderLayouts.h"
#include "../../../../EMStudioSDK/Source/RenderPlugin/RenderUpdateCallback.h"
#include "GLWidget.h"
#endif

namespace EMStudio
{
    class OpenGLRenderPlugin
        : public EMStudio::RenderPlugin
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(OpenGLRenderPlugin, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_RENDERPLUGIN);

    public:
        OpenGLRenderPlugin();
        ~OpenGLRenderPlugin();

        // plugin information
        const char* GetCompileDate() const override         { return MCORE_DATE; }
        const char* GetName() const override                { return "OpenGL Render Window"; }
        uint32 GetClassID() const override                  { return static_cast<uint32>(RenderPlugin::CLASS_ID); }
        const char* GetCreatorName() const override         { return "O3DE"; }
        float GetVersion() const override                   { return 1.0f;  }
        bool GetIsClosable() const override                 { return true;  }
        bool GetIsFloatable() const override                { return true;  }
        bool GetIsVertical() const override                 { return false; }

        // overloaded main init function
        bool Init();
        EMStudioPlugin* Clone()                             { return new OpenGLRenderPlugin(); }

        // overloaded functions
        void CreateRenderWidget(RenderViewWidget* renderViewWidget, RenderWidget** outRenderWidget, QWidget** outWidget) override;

        // OpenGL engine helper functions
        bool InitializeGraphicsManager();
        MCORE_INLINE RenderGL::GraphicsManager* GetGraphicsManager()                { return mGraphicsManager; }

    private:
        RenderGL::GraphicsManager*          mGraphicsManager;           // shared OpenGL engine object

        // overloaded emstudio actor create function which creates an OpenGL render actor internally
        bool CreateEMStudioActor(EMotionFX::Actor* actor);

        void RenderActorInstance(EMotionFX::ActorInstance* actorInstance, float timePassedInSeconds) override;
    };
} // namespace EMStudio
