-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Laurent Barthelemy for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local testname="update_test_02_installscript"

--install script test: not much to define here
--signal will be send by install script in package
--
--(Note: there three components (3 install scripts) being installed 
-- during this update, the "success" signal in sent by the last component update hook,
-- let set the timeout a little bit bigger than usual)
return { timeout=4, signalname=testname , status="success"}
