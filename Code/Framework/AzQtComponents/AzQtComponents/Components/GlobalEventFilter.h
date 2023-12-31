/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzQtComponents/AzQtComponentsAPI.h>

#include <QObject>

namespace AzQtComponents
{
    class AZ_QT_COMPONENTS_API GlobalEventFilter
        : public QObject
    {
    public:
        explicit GlobalEventFilter(QObject* watch);

        bool eventFilter(QObject* obj, QEvent* e) override;
    };
} // namespace AzQtComponents
