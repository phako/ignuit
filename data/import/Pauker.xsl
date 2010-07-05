<?xml version='1.0' encoding='UTF-8'?>
<!--

    I, the Copyright holder of this work, hereby release it into the Public
    Domain. This applies worldwide. In case this is not legally possible, I
    grant anyone the right to use this work for any purpose, without any
    conditions, unless such conditions are required by law.

    2012 Timothy Richard Musson <trmusson@gmail.com>

-->
<!--

  Convert a Pauker flashcard file to an ignuit flashcard file.
  Command line usage:

  xsltproc -novalid -o outfile.xml Pauker.xsl infile.pau

  trm 2008-01-18

-->
<xsl:stylesheet version='1.0'
  xmlns:xsl='http://www.w3.org/1999/XSL/Transform'>

  <xsl:output method='xml' indent='yes' encoding='UTF-8' />
  <xsl:strip-space elements='Description FrontSide ReverseSide' />

  <xsl:template match='FrontSide'>
    <front>
      <xsl:value-of select='.' />
    </front>
  </xsl:template>

  <xsl:template match='ReverseSide'>
    <back>
      <xsl:value-of select='.' />
    </back>
  </xsl:template>

  <xsl:template match='Card'>
    <card>
      <xsl:apply-templates select='FrontSide' />
      <xsl:apply-templates select='ReverseSide' />
    </card>
  </xsl:template>

  <xsl:template match='Batch'>
    <xsl:apply-templates select='Card' />
  </xsl:template>

  <xsl:template match='/Lesson'>
    <deck>
      <xsl:attribute name='version'>
        <xsl:text>1</xsl:text>
      </xsl:attribute>
      <xsl:attribute name='title'>
        <xsl:text>Imported Pauker File</xsl:text>
      </xsl:attribute>
      <xsl:attribute name='comment'>
        <xsl:value-of select='Description' />
      </xsl:attribute>
      <xsl:attribute name='style'>
        <xsl:text>0</xsl:text>
      </xsl:attribute>
      <category>
        <xsl:attribute name='title'>
          <xsl:text>Imported Pauker Cards</xsl:text>
        </xsl:attribute>
        <xsl:apply-templates select='Batch' />
      </category>
    </deck>
  </xsl:template>

</xsl:stylesheet>
