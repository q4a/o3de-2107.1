"""
 All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 its licensors.

 For complete copyright and license terms please see the LICENSE at the root of this
 distribution (the "License"). All use of this software is governed by the License,
 or, if provided, by the license below or the license accompanying this file. Do not
 remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

# !/usr/bin/env python3

from aws_cdk import core
from aws_client_auth.client_auth_stack import AWSClientAuthStack
import os

"""Configuration"""
REGION = os.environ.get('O3D_AWS_DEPLOY_REGION', os.environ['CDK_DEFAULT_REGION'])
ACCOUNT = os.environ.get('O3D_AWS_DEPLOY_ACCOUNT', os.environ['CDK_DEFAULT_ACCOUNT'])

# Set the common prefix to group stacks in a project together.
PROJECT_NAME = os.environ.get('O3D_AWS_PROJECT_NAME', f'O3D-AWS-PROJECT').upper()

env = core.Environment(account=ACCOUNT, region=REGION)
app = core.App()

AWSClientAuthStack(app, PROJECT_NAME, env=env)

app.synth()
