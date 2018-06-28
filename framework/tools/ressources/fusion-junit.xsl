<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
    <xsl:output indent="yes" encoding="UTF-8" method="xml"/>
    <xsl:param name="updateFile"/>

    <xsl:template match="@*|node()">
        <xsl:copy>
            <xsl:apply-templates select="@*|node()" />
        </xsl:copy>
    </xsl:template>

    <xsl:template match="testsuites">

        <xsl:copy>

            <!-- Copy the previous testsuites result -->
            <xsl:apply-templates select="@*|node()" />

            <!-- add the new campaign testsuilte result -->
            <xsl:for-each select="document($updateFile)//testsuites/testsuite">
                <xsl:copy>
                    <xsl:apply-templates select="@*|node()" />
                </xsl:copy>
            </xsl:for-each>

        </xsl:copy>

    </xsl:template>

</xsl:stylesheet>
