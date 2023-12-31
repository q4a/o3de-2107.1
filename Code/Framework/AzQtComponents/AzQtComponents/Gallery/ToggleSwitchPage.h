/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QWidget>
#include <QScopedPointer>
#endif

namespace Ui {
    class ToggleSwitchPage;
}

class ToggleSwitchPage : public QWidget
{
    Q_OBJECT

public:
    explicit ToggleSwitchPage(QWidget* parent = nullptr);
    ~ToggleSwitchPage() override;

private:
    QScopedPointer<Ui::ToggleSwitchPage> ui;
};


