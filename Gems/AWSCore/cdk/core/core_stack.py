"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from aws_cdk import (
    core,
    aws_iam as iam,
    aws_resourcegroups as resource_groups,
)

from constants import Constants
from core_stack_properties import CoreStackProperties


class CoreStack(core.Stack):
    """
    The AWS Core stack

    Defines common resource project and group(s) to use in project
    """

    # Resource groups cannot start with 'AWS' or 'aws' so add this prefix
    RESOURCE_GROUP_PREFIX = 'O3DE'

    def __init__(self, scope: core.Construct, id_: str, project_name: str, feature_name: str, **kwargs) -> None:
        super().__init__(scope, id_, **kwargs)

        self._project_name = project_name
        self._feature_name = feature_name

        # Define Admin Group - these are folks who can deploy, update, edit and delete resources
        self._admin_group = iam.Group(self, id='Admins', group_name=f'{project_name}-Admins')

        # Define User Group - these are users who can call ServiceAPIs etc as players/users
        self._user_group = iam.Group(self, id='Users', group_name=f'{project_name}-Users')

        # Generate a project resource group and automatically add stacks to them via tags
        # Will automatically add all project stacks to this resource group if they are tagged correctly
        query_property = resource_groups.CfnGroup.QueryProperty(
            tag_filters=[
                resource_groups.CfnGroup.TagFilterProperty(
                    key=Constants.O3DE_PROJECT_TAG_NAME,
                    values=[self._project_name]),
                resource_groups.CfnGroup.TagFilterProperty(
                    key=Constants.O3DE_FEATURE_TAG_NAME,
                    values=[self._feature_name]),
            ])

        # Note: Resource group names cannot start with AWS
        resource_group = resource_groups.CfnGroup(
            self,
            id=f'{self._project_name}-ResourceGroup',
            name=f'{CoreStack.RESOURCE_GROUP_PREFIX}-{self._project_name}-ResourceGroup',
            description=f'{self._project_name} application resource group',
            resource_query=resource_groups.CfnGroup.ResourceQueryProperty(
                query=query_property,
                type='TAG_FILTERS_1_0')
        )

        # Define exports
        # Export resource group
        self._resource_group_output = core.CfnOutput(
            self,
            id=f'{self._project_name}-ResourceGroupOutput',
            description='The core stack resource group',
            export_name=f"{self._project_name}:ResourceGroup",
            value=resource_group.name)

        self._user_group_output = core.CfnOutput(
            self,
            id=f'{self._project_name}-UserGroupOutput',
            description='The core stack User group',
            export_name=f"{self._project_name}:UserGroup",
            value=self._user_group.group_arn)

        self._admin_group_output = core.CfnOutput(
            self,
            id=f'{self._project_name}-AdminGroupOutput',
            description='The core stack Admins group',
            export_name=f"{self._project_name}:AdminGroup",
            value=self._admin_group.group_arn)

    @property
    def properties(self) -> CoreStackProperties:
        _props = CoreStackProperties()
        _props.user_group = self._user_group
        _props.admin_group = self._admin_group
        return _props
