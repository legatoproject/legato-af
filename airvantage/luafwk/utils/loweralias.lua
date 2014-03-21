-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Fabien Fleutot for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

return function(M)
    local aliases = { }
    for camelCaseName, value in pairs (M) do
        local lowercasename = camelCaseName :lower()
        if lowercasename ~= camelCaseName then aliases[lowercasename] = value end
    end
    for k, v in pairs(aliases) do M[k] = v end
end