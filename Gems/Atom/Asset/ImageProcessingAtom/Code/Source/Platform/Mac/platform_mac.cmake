#
# Copyright (c) Contributors to the Open 3D Engine Project
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(LY_COMPILE_OPTIONS
    PRIVATE
        -fexceptions #ImageLoader/ExrLoader.cpp and PVRTC.cpp uses exceptions
)
