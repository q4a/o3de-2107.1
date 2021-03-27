"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import os
import sys

import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.editor.graph as graph
import azlmbr.landscapecanvas as landscapecanvas
import azlmbr.legacy.general as general
import azlmbr.math as math
import azlmbr.paths

sys.path.append(os.path.join(azlmbr.paths.devroot, 'AutomatedTesting', 'Gem', 'PythonTests'))
import automatedtesting_shared.hydra_editor_utils as hydra
from automatedtesting_shared.editor_test_helper import EditorTestHelper

editorId = azlmbr.globals.property.LANDSCAPE_CANVAS_EDITOR_ID
newEntityId = None


class TestDisabledNodeDuplication(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="DisabledNodeDuplication", args=["level"])

    def run_test(self):

        def onEntityCreated(parameters):
            global newEntityId
            newEntityId = parameters[0]

        # Create a new empty level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=128,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=128,
            use_terrain=False,
        )

        # Open Landscape Canvas tool and verify
        general.open_pane('Landscape Canvas')
        self.test_success = self.test_success and general.is_pane_visible('Landscape Canvas')
        if general.is_pane_visible('Landscape Canvas'):
            self.log('Landscape Canvas pane is open')

        # Create a new graph in Landscape Canvas
        newGraphId = graph.AssetEditorRequestBus(bus.Event, 'CreateNewGraph', editorId)
        self.test_success = self.test_success and newGraphId
        if newGraphId:
            self.log("New graph created")

        # Listen for entity creation notifications so when we add a new node
        # we can access the corresponding Entity that was created so that we
        # can disable/remove components on that Entity
        handler = editor.EditorEntityContextNotificationBusHandler()
        handler.connect()
        handler.add_callback('OnEditorEntityCreated', onEntityCreated)

        # Mapping of our Landscape Canvas nodes with corresponding dependent components
        # that we can disable/remove to reproduce the crash
        nodes = {
            'SpawnerAreaNode': 'Vegetation Asset List',
            'MeshBlockerAreaNode': 'Mesh',
            'BlockerAreaNode': 'Vegetation Reference Shape',
            'FastNoiseGradientNode': 'Gradient Transform Modifier',
            'ImageGradientNode': 'Gradient Transform Modifier',
            'PerlinNoiseGradientNode': 'Gradient Transform Modifier',
            'RandomNoiseGradientNode': 'Gradient Transform Modifier'
        }

        # Retrieve a mapping of the TypeIds for all the components
        # we will be checking for
        componentNames = list(set(nodes.values())) # Convert to set then back to list to remove any duplicates
        componentTypeIds = hydra.get_component_type_id_map(componentNames)

        # Iterate through creating our nodes and then disabling/deleting required components
        # and then duplicating the node to reproduce the crash
        newGraph = graph.GraphManagerRequestBus(bus.Broadcast, 'GetGraph', newGraphId)
        x = 10.0
        y = 10.0
        for nodeName in nodes:
            nodePosition = math.Vector2(x, y)
            node = landscapecanvas.LandscapeCanvasNodeFactoryRequestBus(bus.Broadcast, 'CreateNodeForTypeName', newGraph, nodeName)
            graph.GraphControllerRequestBus(bus.Event, 'AddNode', newGraphId, node, nodePosition)

            dependentComponentName = nodes[nodeName]
            componentTypeId = componentTypeIds[dependentComponentName]
            componentOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentOfType', newEntityId, componentTypeId)
            component = componentOutcome.GetValue()

            # First make sure we can duplicate a node with a dependent component that is disabled
            editor.EditorComponentAPIBus(bus.Broadcast, 'DisableComponents', [component])
            general.idle_wait(1.0)
            graph.SceneRequestBus(bus.Event, 'DuplicateSelection', newGraphId) # This duplication would cause a crash without the fix
            self.log("{node} duplicated with disabled component".format(node=nodeName))

            # Then, make sure we can duplicate the node with a dependent component that is deleted
            editor.EditorComponentAPIBus(bus.Broadcast, 'RemoveComponents', [component])
            general.idle_wait(1.0)
            graph.SceneRequestBus(bus.Event, 'DuplicateSelection', newGraphId) # This duplication would cause a crash without the fix
            self.log("{node} duplicated with deleted component".format(node=nodeName))

            x += 40.0
            y += 40.0

        # Stop listening for entity creation notifications
        handler.disconnect()


test = TestDisabledNodeDuplication()
test.run()
