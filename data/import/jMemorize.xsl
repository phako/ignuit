<?xml version='1.0' encoding='UTF-8'?>
<!--

    I, the Copyright holder of this work, hereby release it into the Public
    Domain. This applies worldwide. In case this is not legally possible, I
    grant anyone the right to use this work for any purpose, without any
    conditions, unless such conditions are required by law.

    2012 Timothy Richard Musson <trmusson@gmail.com>

-->
<!--

  Convert a jMemorize flashcard file to an ignuit flashcard file.
  Command line usage:

  xsltproc -novalid -o outfile.xml jMemorize.xsl infile.jml

  trm 2008-01-18
  trm 2009-03-24 - Preserve category names.

-->
<xsl:stylesheet version='1.0'
  xmlns:xsl='http://www.w3.org/1999/XSL/Transform'>

  <xsl:output method='xml' indent='yes' encoding='UTF-8' />

  <xsl:template match='Card'>
    <card>
      <front>
        <xsl:value-of select='@Frontside' />
      </front>
      <back>
        <xsl:value-of select='@Backside' />
      </back>
    </card>
  </xsl:template>

  <xsl:template match='Deck'>
    <xsl:if test="descendant::*">
      <category>
        <xsl:attribute name='title'>
          <xsl:value-of select='../@name' />
        </xsl:attribute>
        <xsl:apply-templates />
      </category>
    </xsl:if>
  </xsl:template>

  <xsl:template match='/Lesson'>
    <deck>
      <xsl:attribute name='version'>
        <xsl:text>1</xsl:text>
      </xsl:attribute>
      <xsl:attribute name='title'>
        <xsl:text>Imported jMemorize File</xsl:text>
      </xsl:attribute>
      <xsl:attribute name='style'>
        <xsl:text>1</xsl:text>
      </xsl:attribute>

      <xsl:apply-templates />

    </deck>
  </xsl:template>

</xsl:stylesheet>
