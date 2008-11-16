<?xml version="1.0" encoding="UTF-8" ?>

<!DOCTYPE interface [
	<!ENTITY cr "&#xa;">
	<!ENTITY tab "&#x9;">
]>

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

<!-- Stylesheet setup -->
<xsl:output method="text"/>
<xsl:strip-space elements="RiAPI"/>
<xsl:include href="api_utils.xsl"/>


<!-- Main entry point for matches. -->
<xsl:template match="RiAPI">
<xsl:apply-templates select="Procedures/Procedure[Rib and not(Rib/CustomImpl)]" mode="method_definition"/>
// Custom implementations are provided for the following functions:
<xsl:apply-templates select="Procedures/Procedure[Rib/CustomImpl]" mode="method_definition"/>
</xsl:template>

<xsl:template match="Procedure" mode="method_definition">
	<xsl:text>void handle</xsl:text>
	<xsl:value-of select="substring-after(Name,'Ri')"/>
	<xsl:text>(CqRibParser&amp; parser);&cr;</xsl:text>
</xsl:template>

</xsl:stylesheet>
