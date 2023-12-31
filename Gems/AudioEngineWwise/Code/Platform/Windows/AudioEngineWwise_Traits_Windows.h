/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if defined(WWISE_RELEASE)
#define AZ_TRAIT_AUDIOENGINEWWISE_AUDIOSYSTEMIMPL_USE_SUSPEND 1
#else
#define AZ_TRAIT_AUDIOENGINEWWISE_AUDIOSYSTEMIMPL_USE_SUSPEND 0
#endif
#define AZ_TRAIT_AUDIOENGINEWWISE_COMMAND_QUEUE_MEMORY_POOL_SIZE 256 /* 256 KiB */
#define AZ_TRAIT_AUDIOENGINEWWISE_COMMAND_QUEUE_MEMORY_POOL_SIZE_DEFAULT_TEXT "256 (256 KiB)"
#define AZ_TRAIT_AUDIOENGINEWWISE_DEFAULT_SPEAKER_CONFIGURATION 0
#define AZ_TRAIT_AUDIOENGINEWWISE_FILEIO_AKDEVICE_THREAD_AFFINITY_MASK 0
#if !defined(WWISE_RELEASE)
#define AZ_TRAIT_AUDIOENGINEWWISE_MONITOR_QUEUE_MEMORY_POOL_SIZE 64 /* 64 KiB */
#define AZ_TRAIT_AUDIOENGINEWWISE_MONITOR_QUEUE_MEMORY_POOL_SIZE_DEFAULT_TEXT "64 (64 KiB)"
#endif
#define AZ_TRAIT_AUDIOENGINEWWISE_PRIMARY_POOL_SIZE 128 << 10 /* 128 MiB */
#define AZ_TRAIT_AUDIOENGINEWWISE_PRIMARY_POOL_SIZE_DEFAULT_TEXT "131072 (128 MiB)"
#define AZ_TRAIT_AUDIOENGINEWWISE_SECONDARY_POOL_SIZE 0
#define AZ_TRAIT_AUDIOENGINEWWISE_SECONDARY_POOL_SIZE_DEFAULT_TEXT "0 (0 MiB)"
#define AZ_TRAIT_AUDIOENGINEWWISE_STREAMER_DEVICE_MEMORY_POOL_SIZE 2 << 10 /* 2 MiB */
#define AZ_TRAIT_AUDIOENGINEWWISE_STREAMER_DEVICE_MEMORY_POOL_SIZE_DEFAULT_TEXT "2048 (2 MiB)"

