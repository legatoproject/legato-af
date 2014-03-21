-------------------------------------------------------------------------------
-- Copyright (c) 2012 Sierra Wireless and others.
-- All rights reserved. This program and the accompanying materials
-- are made available under the terms of the Eclipse Public License v1.0
-- which accompanies this distribution, and is available at
-- http://www.eclipse.org/legal/epl-v10.html
--
-- Contributors:
--     Romain Perier for Sierra Wireless - initial API and implementation
-------------------------------------------------------------------------------

local table_hdl = require 'agent.treemgr.handlers.table'()

table_hdl.table = {
   ["current"] = "M2M-apn-sprint",
   ["cdma_ecio"] = "-7",
   ["cdma_operator"] = "SPRINT",
   ["pn_offset"] = "224",
   ["sid"] = "22",
   ["nid"] = "6553",
   ["cell_id"] = "1234",
   ["gsm_ecio"] = "-7",
   ["gsm_operator"] = "SPRINT",
   ["rsrp"] = -70,
   ["rsrq"] = -102,
   ["bytes_rcvd"] = 789995,
   ["bytes_sent"] = 97667,
   ["roam_status"] = "No roaming",
   ["ip"] = "10.162.23.67",
   ["pkts_rcvd"] = 1453,
   ["pkts_sent"] = 1253,
   ["rssi"] = -80,
   ["service"] = "LTE",
   ["hw_info_imei"] = "012345678988877",
   ["sim_iccid"] = "8976543210555555487",
   ["sim_imsi"] = "204080899999963",
   ["subscriber_phone_num"] = "+33798553607",
   ["product_str"] = "AC771S",
   ["fw_ver"] = "04.00.00.32",
   ["fw_name"] = "AC771S Firmware",
   ["powerin"] = 4.2,
   ["board_temp"] = 52,
   ["radio_temp"] = 64,
   ["reset_nb"] = 6,
   ["latitude"] = 48.635,
   ["longitude"] = 2.012,
}

return table_hdl
