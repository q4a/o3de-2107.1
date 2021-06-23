/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_IOBSERVABLE_H
#define CRYINCLUDE_EDITOR_IOBSERVABLE_H
#pragma once

//! Observable macro to be used in pure interfaces
#define DEFINE_OBSERVABLE_PURE_METHODS(observerClassName)               \
    virtual bool RegisterObserver(observerClassName * pObserver) = 0;   \
    virtual bool UnregisterObserver(observerClassName * pObserver) = 0; \
    virtual void UnregisterAllObservers() = 0;

#endif // CRYINCLUDE_EDITOR_IOBSERVABLE_H
