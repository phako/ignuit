<?xml version='1.0' encoding='UTF-8'?>
<!--

    I, the Copyright holder of this work, hereby release it into the Public
    Domain. This applies worldwide. In case this is not legally possible, I
    grant anyone the right to use this work for any purpose, without any
    conditions, unless such conditions are required by law.

    2012 Timothy Richard Musson <trmusson@gmail.com>

-->
<!--

  Convert a Granule flashcard file to an ignuit flashcard file.
  Command line usage:

  xsltproc -novalid -o outfile.xml Granule.xsl infile.dkf

  trm 2008-01-18

-->
<xsl:stylesheet version='1.0'
  xmlns:xsl='http://www.w3.org/1999/XSL/Transform'>

  <xsl:output method='xml' indent='yes' encoding='UTF-8' />
  <xsl:strip-space elements='*' />

  <xsl:variable name='linebreak'>
    <xsl:text>
</xsl:text>
  </xsl:variable>

  <xsl:template match='front'>
    <xsl:value-of select='.' />
  </xsl:template>

  <xsl:template match='front_example'>
    <xsl:value-of select='$linebreak' />
    <xsl:value-of select='.' />
  </xsl:template>

  <xsl:template match='back'>
    <xsl:value-of select='.' />
  </xsl:template>

  <xsl:template match='back_example'>
    <xsl:value-of select='$linebreak' />
    <xsl:value-of select='.' />
  </xsl:template>

  <xsl:template match='card'>
    <card>
      <front>
        <xsl:apply-templates select='front' />
        <xsl:apply-templates select='front_example' />
      </front>
      <back>
        <xsl:apply-templates select='back' />
        <xsl:apply-templates select='back_example' />
      </back>
    </card>
  </xsl:template>

  <xsl:template match='/deck'>
    <deck>
      <xsl:attribute name='version'>
        <xsl:text>1</xsl:text>
      </xsl:attribute>
      <xsl:attribute name='title'>
        <xsl:text>Imported Granule File</xsl:text>
      </xsl:attribute>
      <xsl:attribute name='author'>
        <xsl:value-of select='author' />
      </xsl:attribute>
      <xsl:attribute name='comment'>
        <xsl:value-of select='description' />
      </xsl:attribute>
      <xsl:attribute name='style'>
        <xsl:text>0</xsl:text>
      </xsl:attribute>
      <category>
        <xsl:attribute name='title'>
          <xsl:text>Imported Granule Cards</xsl:text>
        </xsl:attribute>
        <xsl:apply-templates select='card' />
      </category>
    </deck>
  </xsl:template>

</xsl:stylesheet>
