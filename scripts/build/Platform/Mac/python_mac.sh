#!/usr/bin/env bash
#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set -o errexit # exit on the first failure encountered

echo [ci_build] python/python.sh -u ${SCRIPT_PATH} $(eval echo ${SCRIPT_PARAMETERS})
python/python.sh -u ${SCRIPT_PATH} $(eval echo ${SCRIPT_PARAMETERS})