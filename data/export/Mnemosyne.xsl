<?xml version='1.0' encoding='UTF-8'?>
<!--

    I, the Copyright holder of this work, hereby release it into the Public
    Domain. This applies worldwide. In case this is not legally possible, I
    grant anyone the right to use this work for any purpose, without any
    conditions, unless such conditions are required by law.

    2012 Timothy Richard Musson <trmusson@gmail.com>

-->
<!--

  Convert an ignuit flashcard file to a Mnemosyne flashcard file.
  Command line usage:

  xsltproc -novalid -o outfile.xml Mnemosyne.xsl infile.xml

  trm 2009-03-13

-->
<xsl:stylesheet version='1.0'
  xmlns:xsl='http://www.w3.org/1999/XSL/Transform'>

  <xsl:output
    method='xml'
    indent='yes'
    encoding='UTF-8' />

  <xsl:strip-space elements='*' />

  <xsl:template match='front'>
    <Q><xsl:value-of select='.' /></Q>
  </xsl:template>

  <xsl:template match='back'>
    <A><xsl:value-of select='.' /></A>
  </xsl:template>

  <xsl:template match='card'>
    <item>
      <cat>&lt;default&gt;</cat>
      <xsl:apply-templates select='front' />
      <xsl:apply-templates select='back' />
    </item>
  </xsl:template>

  <xsl:template match='/deck'>
    <mnemosyne core_version='1'>
      <category active='1'>
        <name>
          <xsl:value-of select='@title' />
        </name>
      </category>
      <xsl:apply-templates />
    </mnemosyne>
  </xsl:template>

</xsl:stylesheet>
