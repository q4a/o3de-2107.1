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

#include <AzTest/AzTest.h>

#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>

namespace UnitTest
{
    using namespace AZ;
    using namespace AzToolsFramework;

    class EditorEntityHelpersTest
        : public ToolsApplicationFixture
    {
        void SetUpEditorFixtureImpl() override
        {
            m_parent1 = CreateDefaultEditorEntity("Parent1");
            m_child1 = CreateDefaultEditorEntity("Child1");
            m_child2 = CreateDefaultEditorEntity("Child2");
            m_grandChild1 = CreateDefaultEditorEntity("GrandChild1");
            m_parent2 = CreateDefaultEditorEntity("Parent2");

            AZ::TransformBus::Event(m_child1, &AZ::TransformBus::Events::SetParent, m_parent1);
            AZ::TransformBus::Event(m_child2, &AZ::TransformBus::Events::SetParent, m_parent1);
            AZ::TransformBus::Event(m_grandChild1, &AZ::TransformBus::Events::SetParent, m_child1);
        }

    public:
        AZ::EntityId m_parent1;
        AZ::EntityId m_child1;
        AZ::EntityId m_child2;
        AZ::EntityId m_grandChild1;
        AZ::EntityId m_parent2;
    };

    TEST_F(EditorEntityHelpersTest, EditorEntityHelpersTests_GetCulledEntityHierarchy)
    {
        EntityIdList testEntityIds{ m_parent1, m_child1, m_child2, m_grandChild1, m_parent2 };

        EntityIdSet culledSet = GetCulledEntityHierarchy(testEntityIds);

        // There should only be two EntityIds returned (m_parent1, and m_parent2),
        // since all the others should be culled out since they have a common ancestor
        // in the list already
        EXPECT_EQ(culledSet.size(), 2);

        EntityIdList foundEntityIds{ m_parent1, m_parent2 };
        for (auto& entityId : foundEntityIds)
        {
            EXPECT_TRUE(AZStd::find(culledSet.begin(), culledSet.end(), entityId) != culledSet.end());
        }

        EntityIdList culledEntityIds{ m_child1, m_child2, m_grandChild1 };
        for (auto& entityId : culledEntityIds)
        {
            EXPECT_FALSE(AZStd::find(culledSet.begin(), culledSet.end(), entityId) != culledSet.end());
        }
    }
}
