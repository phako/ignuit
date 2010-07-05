<?xml version='1.0' encoding='UTF-8'?>
<!--

    I, the Copyright holder of this work, hereby release it into the Public
    Domain. This applies worldwide. In case this is not legally possible, I
    grant anyone the right to use this work for any purpose, without any
    conditions, unless such conditions are required by law.

    2012 Timothy Richard Musson <trmusson@gmail.com>

-->
<!--

  Convert a Mnemosyne flashcard file to an ignuit flashcard file.
  Command line usage:

  xsltproc -novalid -o outfile.xml Mnemosyne.xsl infile.xml

  trm 2009-03-13

-->
<xsl:stylesheet version='1.0'
  xmlns:xsl='http://www.w3.org/1999/XSL/Transform'>

  <xsl:output method='xml' indent='yes' encoding='UTF-8' />
  <xsl:strip-space elements='Q A' />

  <xsl:template match='Q'>
    <front>
      <xsl:value-of select='.' />
    </front>
  </xsl:template>

  <xsl:template match='A'>
    <back>
      <xsl:value-of select='.' />
    </back>
  </xsl:template>

  <xsl:template match='item'>
    <card>
      <xsl:apply-templates select='Q' />
      <xsl:apply-templates select='A' />
    </card>
  </xsl:template>

  <xsl:template match='/mnemosyne'>
    <deck>
      <xsl:attribute name='version'>
        <xsl:text>1</xsl:text>
      </xsl:attribute>
      <xsl:attribute name='title'>
        <xsl:text>Imported Mnemosyne File</xsl:text>
      </xsl:attribute>
      <xsl:attribute name='style'>
        <xsl:text>0</xsl:text>
      </xsl:attribute>
      <category>
        <xsl:attribute name='title'>
          <xsl:text>Imported Mnemosyne Cards</xsl:text>
        </xsl:attribute>
        <xsl:apply-templates select='item' />
      </category>
    </deck>
  </xsl:template>

</xsl:stylesheet>
