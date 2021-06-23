/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SceneAPI/SDKWrapper/NodeWrapper.h>

namespace AZ
{
    namespace SDKNode
    {
        NodeWrapper::NodeWrapper(fbxsdk::FbxNode* fbxNode)
            :m_fbxNode(fbxNode)
            , m_assImpNode(nullptr)
        {
        }

        NodeWrapper::NodeWrapper(aiNode* aiNode)
            :m_fbxNode(nullptr)
            , m_assImpNode(aiNode)
        {
        }

        NodeWrapper::~NodeWrapper()
        {
            m_fbxNode = nullptr;
            m_assImpNode = nullptr;
        }

        fbxsdk::FbxNode* NodeWrapper::GetFbxNode()
        {
            return m_fbxNode;
        }

        aiNode* NodeWrapper::GetAssImpNode()
        {
            return m_assImpNode;
        }

        const char* NodeWrapper::GetName() const
        {
            return "";
        }
        AZ::u64 NodeWrapper::GetUniqueId() const
        {
            return 0;
        }

        int NodeWrapper::GetMaterialCount() const
        {
            return -1;
        }

        int NodeWrapper::GetChildCount()const
        {
            return -1;
        }
        const std::shared_ptr<NodeWrapper> NodeWrapper::GetChild([[maybe_unused]] int childIndex) const
        {
            return {};
        }

    }// namespace Node
}//namespace AZ
