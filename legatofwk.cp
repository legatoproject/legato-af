<?xml version="1.0" encoding="UTF-8"?>
<app:capabilities xmlns:app="http://www.sierrawireless.com/airvantage/application/1.0">
  <data>
    <encoding type="LWM2M">
      <asset id="legato">
        <node default-label="Legato Framework" path="0">
          <variable default-label="Version" path="0" type="string"/>
          <command default-label="Restart" path="1"/>
          <variable default-label="System Index" path="2" type="int"/>
          <variable default-label="Previous Index" path="3" type="int"/>
        </node>
        <node default-label="Legato System Update" path="1">
          <setting default-label="Package URI" path="1" type="string"/>
          <command default-label="Update" path="2"/>
          <variable default-label="State" path="3" type="int"/>
          <setting default-label="Update Supported Objects" path="4" type="boolean"/>
          <variable default-label="Update Result" path="5" type="int"/>
        </node>
      </asset>
    </encoding>
  </data>
</app:capabilities>
