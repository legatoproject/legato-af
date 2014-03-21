<?xml version="1.0" encoding="ISO-8859-1" ?>
<app:application xmlns:app="http://www.sierrawireless.com/airvantage/application/1.0" 
    type="Legato_Beta"
    name="Legato_Beta" 
    revision="1.0"> <!-- application version -->

    <capabilities>
        <communication>
            <protocol comm-id="IMEI" type="M3DA" >
                <parameter name="authentication" value="None"/>
                <parameter name="cipher" value="None"/>
            </protocol>
        </communication>

        <!-- external variables definition -->
        <include>
            <file name="legato_device_M3DA_datamodel.cp" />
        </include>

        <!-- Device management commands provided by AirVantage Agent-->
        <dm>
            <action impl="MIHINI_CONFIGURE_HEARTBEAT" />
            <action impl="MIHINI_DM_SYNCHRONIZE" />
          
            <!-- Device Reboot: --> 
            <action impl="MIHINI_DM_REBOOT"/>

            <!-- Reset To Factory Default: not integrated yet
            <action impl="MIHINI_DM_RESET" />
            -->

            <!-- Application Container capabilities -->
            <action impl="MIHINI_APPCON_INSTALL" />
            <action impl="MIHINI_APPCON_UNINSTALL" />
            <action impl="MIHINI_APPCON_START" />
            <action impl="MIHINI_APPCON_STOP" />
            <!--
            <action impl="MIHINI_APPCON_STATUS" />
            -->
            

        </dm>

    </capabilities>

     <application-manager use="FIRM_IDS"/>

</app:application>
