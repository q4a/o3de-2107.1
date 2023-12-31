----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------

-- require with:
-- local inputMultiHandler = require('scripts.utils.components.inpututils')

-- self.inputHandlers = inputMultiHandler.ConnectMultiHandlers{
--     [InputEventNotificationId("jump")] = {
--         OnPressed = function(floatValue) self:JumpPressed(floatValue) end,
--         OnHeld = function(floatValue) self:JumpHeld(floatValue) end,
--         OnReleased = function(floatValue) self:JumpReleased(floatValue) end,
--     },
-- }

-- disconnect from like this:
-- self.inputHandlers:Disconnect()
local inputMultiHandlers = require('scripts.utils.components.MultiHandlers')
return {
    ConnectMultiHandlers = inputMultiHandlers(InputEventNotificationBus, InputEventNotificationId),
}