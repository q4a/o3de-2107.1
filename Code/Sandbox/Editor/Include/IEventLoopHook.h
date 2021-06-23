/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_INCLUDE_IEVENTLOOPHOOK_H
#define CRYINCLUDE_EDITOR_INCLUDE_IEVENTLOOPHOOK_H
#pragma once

struct IEventLoopHook
{
    IEventLoopHook* pNextHook;

    IEventLoopHook()
        : pNextHook(0) {}

    virtual bool PrePumpMessage() { return false; }
};

#endif // CRYINCLUDE_EDITOR_INCLUDE_IEVENTLOOPHOOK_H
