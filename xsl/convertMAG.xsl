<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  <xsl:variable name="BASEURL">
      <xsl:value-of select="$baseUrl"/>
  </xsl:variable>
  <xsl:variable name="PATH">
      <xsl:value-of select="$path"/>
  </xsl:variable>
<xsl:template match="/">
<div>
  <table cellspacing="3" cellpadding="3">
    <tbody>
      <tr><th colspan="4"><h3>MAG Data</h3></th></tr>  
      <xsl:for-each select="/metadigit/*">
        <xsl:variable name="FULLFIELD" select="name()"/>
        <xsl:variable name="FIELD" select="name()"/>
        <xsl:variable name="DATA" select="text()"/>
      <tr class="mag_fields"><td>Sezione: <xsl:value-of select="name()"/></td>
        <td colspan="3">
          <xsl:for-each select="*">
            <tr class="field">
              <td class="field_name">
              <xsl:choose>
                <xsl:when test="contains(name(),':')">
                  <xsl:value-of select="substring-after(name(),':')"/>
                </xsl:when>
                <xsl:otherwise>
                  <xsl:value-of select="name()"/>
                </xsl:otherwise>
              </xsl:choose>
              
              <xsl:for-each select="*">
                <tr class="sub_field_1">
                  <td class="sub_field_1_name">
                  <xsl:choose>
                    <xsl:when test="contains(name(),':')">
                      <xsl:value-of select="substring-after(name(),':')"/>
                    </xsl:when>
                    <xsl:otherwise>
                      <xsl:value-of select="name()"/>
                    </xsl:otherwise>
                  </xsl:choose>
                  
                  <xsl:for-each select="*">
		                <tr class="sub_field_2">
		                  <td class="sub_field_2_name">
		                  <xsl:choose>
		                    <xsl:when test="contains(name(),':')">
		                      <xsl:value-of select="substring-after(name(),':')"/>
		                    </xsl:when>
		                    <xsl:otherwise>
		                      <xsl:value-of select="name()"/>
		                    </xsl:otherwise>
		                  </xsl:choose>
		                  </td>
		                  <td class="sub_field_2_value"><xsl:value-of select="text()"/></td>
		                </tr>
		              </xsl:for-each>
                  
                  </td>
                  <td class="sub_field_1_value"><xsl:value-of select="text()"/></td>
                </tr>
              </xsl:for-each>
              
              </td>
              <td class="field_value"><xsl:value-of select="text()"/></td>
            </tr>
          </xsl:for-each>
        </td>
      </tr>
      </xsl:for-each>
    </tbody>
  </table>
</div>
</xsl:template>
</xsl:stylesheet>