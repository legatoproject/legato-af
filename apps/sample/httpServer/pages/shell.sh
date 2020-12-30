#!/bin/sh
echo "Content-Type: text/html"
echo ""

echo "<html>"
echo "<head>"
echo "<title>Legato Run Shell Command</title>"
echo "<link type=\"text/css\" href=\"/stylesheet.css\" rel=\"stylesheet\"/>"
echo "</head>"

echo "<body>"
echo "<a href=\"/\"><img src=\"legato_logo.png\"/></a>"
echo "<h1>Run Shell Command</h1>"

echo "<form action=\"?\" method=\"get\">"
echo    "Command: <input autofocus type=\"text\" name=\"command\" size=\"64\"><br>"
echo    "<input type=\"submit\" value=\"Submit\">"
echo "</form>"
echo "<br>"
#echo "Query String: $QUERY_STRING"
#echo "Request Method: $REQUEST_METHOD"
IFS='=&'
set -- $QUERY_STRING
decodeURL() { printf "%b\n" "$(sed 's/+/ /g; s/%\([0-9a-fA-F][0-9a-fA-F]\)/\\x\1/g;')"; }
new_decoded=$(echo ${2} | decodeURL)
echo "<p> Decoded Command: ${new_decoded} </p>"
echo "<pre> <b>Command Output:</b> <br><br> $(${new_decoded}) </pre>"
echo "</body>"
echo "</html>"