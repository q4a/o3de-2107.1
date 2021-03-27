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

import azlmbr.surface_data as surface_data

sys.path.append(os.path.join(azlmbr.paths.devroot, 'AutomatedTesting', 'Gem', 'PythonTests'))
from automatedtesting_shared.editor_test_helper import EditorTestHelper


class TestSurfaceMaskFilter_BasicSurfaceTagCreation(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="TestSurfaceMaskFilter_BasicSurfaceTagCreation", args=["level"])
        
    def run_test(self):
        self.log("SurfaceTag test started")
        
        # Create a level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=False,
        )
        
        tag1 = surface_data.SurfaceTag()
        tag2 = surface_data.SurfaceTag()
        
        # Test 1:  Verify that two tags with the same value are equal
        tag1.SetTag('equal_test')
        tag2.SetTag('equal_test')
        self.log("SurfaceTag equal tag comparison is {} expected True".format(tag1.Equal(tag2)))
        self.test_success = self.test_success and tag1.Equal(tag2)
        
        # Test 2:  Verify that two tags with different values are not equal
        tag2.SetTag('not_equal_test')
        self.log("SurfaceTag not equal tag comparison is {} expected False".format(tag1.Equal(tag2)))
        self.test_success = self.test_success and not tag1.Equal(tag2)
        
        self.log("SurfaceTag test finished")


test = TestSurfaceMaskFilter_BasicSurfaceTagCreation()
test.run()
