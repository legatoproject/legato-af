#!/bin/sh
cat << EOF
Content-Type: text/html

<html>
<head>
<title>Legato</title>
</head>
<body>
<h1>Stats for this board</h1>
<p> Date: $(date)
<p> Uptime: $(uptime)
<p> Legato Version: $(/legato/systems/current/bin/legato version)
</body>
</html>
EOF
