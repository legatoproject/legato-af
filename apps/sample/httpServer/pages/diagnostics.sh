#!/bin/sh
cat << EOF
Content-Type: text/html

<html>
<head>
<title>Legato Diagnostics</title>
<link type="text/css" href="/stylesheet.css" rel="stylesheet"/>
</head>
<body>
<a href="/"><img src="legato_logo.png"/></a>

<h2>Stats</h2>

<dl>
<dt>Date</dt><dd> $(date) </dd>
<dt>Uptime</dt><dd> $(cat /proc/uptime) </dd>
<dt>Legato Version</dt><dd>  $(/legato/systems/current/bin/legato version)  </dd>
</dl>

<a href="/server-status">lighttpd status</a>

<h2>Network [ifconfig]</h2>
<pre>
$(/sbin/ifconfig)
</pre>

<h2>Running Processes [ps aux]</h2>
<pre>
$(/bin/ps aux)
</pre>

</body>
</html>
EOF
