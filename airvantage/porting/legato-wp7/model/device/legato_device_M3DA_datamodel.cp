<?xml version="1.0" encoding="UTF-8"?>
<app:capabilities xmlns:app="http://www.sierrawireless.com/airvantage/application/1.0">
  <data>
    <encoding type="M3DA">
      <asset default-label="Device" id="@sys">
        <node default-label="Configuration" path="config">
          <node default-label="Agent" path="agent">
            <setting default-label="Asset Port" path="assetport" type="int">
              <description>Defines the local port on which the agent is listening in order to communicate with the assets</description>
              <constraints>
                <bounds max="65535" min="1"/>
              </constraints>
            </setting>
            <setting default-label="Asset Address" path="assetaddress">
              <description>Address on which the agent is accepting connection in order to communicate with the assets. Pattern is accepted: can be set to &quot;*&quot; to accept connection from any address, by default shell accepts only localhost connection.</description>
            </setting>
            <setting default-label="Device ID" path="deviceId">
              <description>Devices ID used to communicate with the platform server</description>
            </setting>
            <setting default-label="Signal Port" path="signalport" type="int">
              <description>Defines the local port on which the agent is listening in order to receive LUASIGNAL from external applications (Linux only)</description>
              <constraints>
                <bounds max="65535" min="1"/>
              </constraints>
            </setting>
          </node>
          <node default-label="Application Container" path="appcon">
            <setting default-label="Activate Application Container" path="activate" type="boolean">
              <description>Activate the Application Container</description>
            </setting>
          </node>
          <node default-label="Data Management" path="data">
            <setting default-label="Activate Data Management" path="activate" type="boolean">
              <description>Activate Data queues</description>
            </setting>
            <node default-label="Data Queues" path="queues">
              <description>List of available Data queues. Each data queue correspond to a data sending policy, there are three main types of policies: manual: data sending is triggered by the user; cron: data sending is triggered on cron events; latency: data sending is triggered some times after a data push.</description>
              <node default-label="Default Queue" path="default">
                <description>Default data queue to use when no policy name is used when sending data</description>
                <setting default-label="Value" path="1" type="int"/>
              </node>
              <node default-label="Queue - Hourly" path="hourly">
                <setting default-label="Latency" path="latency" type="int"/>
              </node>
              <node default-label="Queue - Daily" path="daily">
                <setting default-label="Latency" path="latency" type="int"/>
              </node>
              <node default-label="Queue - Never" path="never">
                <setting default-label="Value" path="1" type="int"/>
              </node>
              <node default-label="Queue - Now" path="now">
                <setting default-label="Latency" path="latency" type="int"/>
              </node>
            </node>
          </node>
          <node default-label="Device Management" path="device">
            <setting default-label="Device Management Activation" path="activate" type="boolean">
              <description>Device Management activation</description>
            </setting>
          </node>
          <node default-label="Logs" path="log">
            <setting default-label="Log Default Level" path="defaultlevel">
              <constraints>
                <enumeration>
                  <value default-label="None">NONE</value>
                  <value default-label="Error">ERROR</value>
                  <value default-label="Info">INFO</value>
                  <value default-label="Detail">DETAIL</value>
                  <value default-label="Debug">DEBUG</value>
                  <value default-label="All">ALL</value>
                </enumeration>
              </constraints>
            </setting>
            <node default-label="Log - Modules Levels" path="moduleslevel">
              <description>Module log level</description>
              <setting default-label="General Logs" path="GENERAL">
                <constraints>
                  <enumeration>
                    <value default-label="None">NONE</value>
                    <value default-label="Error">ERROR</value>
                    <value default-label="Info">INFO</value>
                    <value default-label="Detail">DETAIL</value>
                    <value default-label="Debug">DEBUG</value>
                    <value default-label="All">ALL</value>
                  </enumeration>
                </constraints>
              </setting>
              <setting default-label="Server Logs" path="SERVER">
                <constraints>
                  <enumeration>
                    <value default-label="None">NONE</value>
                    <value default-label="Error">ERROR</value>
                    <value default-label="Info">INFO</value>
                    <value default-label="Detail">DETAIL</value>
                    <value default-label="Debug">DEBUG</value>
                    <value default-label="All">ALL</value>
                  </enumeration>
                </constraints>
              </setting>
            </node>
            <setting default-label="Log Colored Mode" path="enablecolors" type="boolean"/>
            <setting default-label="Log Format" path="format" type="string"/>
            <setting default-label="Log Timestamp Format" path="timestampformat" type="string">
              <description>Timestampformat specifies strftime format to print date/time. Timestampformat is useful only if %t% is in formatter string</description>
            </setting>
          </node>         
<!-- Monitoring not integrated yet
          <node default-label="Monitoring" path="monitoring">
            <setting default-label="Activate Monitoring" path="activate" type="boolean">
              <description>Monitoring activation</description>
            </setting>
            <setting default-label="Debug" path="debug" type="boolean">
              <description>debug mode activation</description>
            </setting>
          </node>
-->          
          <node default-label="Network Manager" path="network">
            <setting default-label="Network Manager Activation" path="activate" type="boolean">
              <description>Activate the Network Manager</description>
            </setting>
          </node>    
          <node default-label="Update" path="update">
            <setting default-label="Update Activation" path="activate" type="boolean">
              <description>Activate the Update Agent</description>
            </setting>
            <setting default-label="Update Retries number" path="retries" type="int">
              <description>Retries number per component, default value: 2</description>
            </setting>
            <setting default-label="Update Timeout" path="timeout" type="int">
              <description>timeout in seconds for component update response, default value:40</description>
            </setting>
            <setting default-label="Update Local Update File Name" path="localpkgname" type="string"> 
                <description>File name relative to drop folder</description>
            </setting>
            <setting default-label="Update Notification Period" path="dwlnotifperiod" type="int">
                <description>File name relative to drop folder</description>
            </setting>
          </node>
          <node default-label="Server" path="server">
            <setting default-label="Server URL" path="url">
              <description>URL on which the agent will try the server connection. This parameter is only relevant for HTTP transport protocol</description>
            </setting>
            <setting default-label="Server Proxy" path="proxy">
              <description>When the device is behind a proxy this settings defines a HTTP proxy. This parameter is only relevant for HTTP transport protocol. It must be a URL starting by &quot;http://&quot;.</description>
            </setting>
            <node default-label="Server Auto Connect" path="autoconnect">
              <description>Agent auto connection policy</description>
              <setting default-label="Server Auto Connect - On Demand" path="ondemand" type="int">
                <description>latency before connection (in seconds) after some data has been given to the ReadyAgent before it connects to the server (connect will occur at maximum 10 seconds after some data has been written)</description>
              </setting>
              <setting default-label="Server Auto Connect - On Boot" path="onboot" type="boolean">
                <description>Connect a few seconds after the ReadyAgent started</description>
              </setting>
              <setting default-label="Server Auto Connect - Periodic" path="period" type="int">
                <description>Period in minute</description>
              </setting>
              <setting default-label="Server Auto Connect - Cron" path="cron">
                <description>cron entry (&quot;0 0 * * *&quot; connect once a day at midnight)</description>
              </setting>
            </node>
          </node>
          <node default-label="Shell" path="shell">
            <description>Shell related settings</description>
            <setting default-label="Shell Activate" path="activate" type="boolean">
              <description>Activate the Lua Shell</description>
            </setting>
            <setting default-label="Shell Port" path="port" type="int">
              <description>Local port on which the Lua Shell server is listening</description>
              <constraints>
                <bounds max="65535" min="1"/>
              </constraints>
            </setting>
            <setting default-label="Shell Address" path="address" type="string">
              <description>Address on which the Lua Shell server is accepting connection. Pattern is accepted: can be set to &quot;*&quot; to accept connection from any address, by default shell accepts only localhost connection.</description>
            </setting>
            <setting default-label="Shell Edit Mode" path="editmode" type="string">
              <description>&quot;edit&quot; by default, can be &quot;line&quot; if the trivial line by line mode is wanted</description>
              <constraints>
                <enumeration>
                  <value default-label="Edit">edit</value>
                  <value default-label="Line">line</value>
                </enumeration>
              </constraints>
            </setting>
            <setting default-label="Shell History Size" path="historysize" type="int">
              <description>Only valid for edit mode</description>
            </setting>
          </node>
        </node>

<!-- End of Config subtree -->



<!-- Update subtree: Not integrated yet

    <node default-label="Update" path="update">
        <node default-label="Software List" path="swlist">
            <node default-label="Components" path="components">
                <node default-label="Platform defaults" path="0">
                    <node default-label="Features" path="provides">
                        <variable default-label="ReadyAgent" path="ReadyAgent" type="string"/>
                    </node>
                </node>
            </node>
        </node>
        <node default-label="Current Update" path="currentupdate">
    
        </node>
    </node>

--> 





<!-- Status Report Variables, need specific tree handler -->

        <node path="system" default-label="System">
          <node path="cellular" default-label="Cellular">
            <node path="apn" default-label="APN">
              <variable path="current" type="string" default-label="Current APN" />
            </node>
            <node path="link" default-label="Link">
              <variable path="rssi" type="int" default-label="RSSI" />
              <variable path="service" type="string" default-label="Service" />
              <variable path="bytes_rcvd" type="double" default-label="Bytes received" />
              <variable path="bytes_sent" type="double" default-label="Bytes sent" />
              <variable path="pkts_rcvd" type="double" default-label="Packets received" />
              <variable path="pkts_sent" type="double" default-label="Packets sent" />
              <variable path="roam_status" type="string" default-label="Roaming status" />
              <variable path="ip" type="string" default-label="Cellular connection IP address" />
            </node>
            <node path="cdma" default-label="CDMA">
              <node path="link" default-label="CDMA Link">
                <variable path="operator" type="string" default-label="CDMA Operator" />
                <variable path="ecio" type="string" default-label="CDMA Ec/Io" />
                <variable path="pn_offset" type="string" default-label="CDMA current PN Offset" />
                <variable path="sid" type="string" default-label="CDMA Sector Identifier (SID)" />
                <variable path="nid" type="string" default-label="CDMA Network Identifier (NID)" />
              </node>
            </node>
            <node path="gsm" default-label="GSM">
              <node path="link" default-label="GSM Link">
                <variable path="operator" type="string" default-label="GSM Operator" />
                <variable path="ecio" type="string" default-label="GSM Ec/Io" />
                <variable path="cell_id" type="string" default-label="GSM Cell Id" />
              </node>
            </node>
            <node path="lte" default-label="LTE">
              <node path="link" default-label="LTE Link">
                <variable path="rsrp" type="int" default-label="RSRP" />
                <variable path="rsrq" type="int" default-label="RSRQ" />
              </node>
            </node>
          </node>
          <node path="gps" default-label="GPS">
            <variable path="latitude" type="double" default-label="Latitude" />
            <variable path="longitude" type="double" default-label="Longitude" />
          </node>
          <node path="io" default-label="IO">
              <variable path="powerin" type="double" default-label="Voltage status of input power pin" />
          </node>
          <node path="advanced" default-label="Advanced">
              <variable path="board_temp" type="int" default-label="Board Temp in Celsius degrees" />
              <variable path="radio_temp" type="int" default-label="Radio Module Temp in Celsius degrees" />
              <variable path="reset_nb" type="int" default-label="Nb of system resets" />
          </node>
        </node>

<!-- End of Status Report Variables -->

      </asset>
    </encoding>
  </data>
</app:capabilities>