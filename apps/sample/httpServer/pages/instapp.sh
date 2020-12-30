#!/bin/sh
echo "Content-Type: text/html"
echo ""

echo "<html>"
echo "<head>"
echo "<title>Install Application</title>"
echo "<link type=\"text/css\" href=\"/stylesheet.css\" rel=\"stylesheet\"/>"
echo "</head>"

echo "<body>"
echo "<a href=\"/\"><img src=\"legato_logo.png\"/></a>"
echo "<h1>Install Application</h1>"

updateFile=updateFile # Need this file to hold the POST data.
temp_updateFile=temp_updateFile # Need this file to be the intermediary file when modifying the actual updateFile.

if [ "$REQUEST_METHOD" = "POST" ]; then
    echo "<p>Post Method</p>"
    if [ "$CONTENT_LENGTH" -gt 0 ]; then
        cat >|"$updateFile"
        sed '1,4d;$d' $updateFile > $temp_updateFile # Delete lines 1-4 and last line.
        wordcount=$(stat -c "%s" $temp_updateFile) # Equivalent to wc -c, getting number of bytes.
        wordcount=$((wordcount-2)) # Decrement by two when we remove the carriage return from our data.
        dd if="$temp_updateFile" of="$updateFile" bs=1 count=$wordcount # Truncate the last two bytes.
        $(/legato/systems/current/bin/update $updateFile) # Pass the updateFile to updateDaemon.
    fi
fi
echo "<form action=\"\" method=\"post\" enctype=\"multipart/form-data\">"
echo    "File: <input type=\"file\" name=\"filename\"><p>"
echo    "<button type=\"submit\">Install</button>"
echo "</form>"

echo "</body>"
echo "</html"