<?xml version='1.0' encoding='UTF-8'?>
<!--

    I, the Copyright holder of this work, hereby release it into the Public
    Domain. This applies worldwide. In case this is not legally possible, I
    grant anyone the right to use this work for any purpose, without any
    conditions, unless such conditions are required by law.

    2012 Timothy Richard Musson <trmusson@gmail.com>

-->
<!--

  Convert an ignuit flashcard file to a Granule flashcard file.
  Command line usage:

  xsltproc -novalid -o outfile.dkf Granule.xsl infile.xml

  FIXME: back_example isn't implemented.

  trm 2008-01-22

-->
<xsl:stylesheet version='1.0'
  xmlns:xsl='http://www.w3.org/1999/XSL/Transform'>

  <xsl:output
    method='xml'
    indent='yes'
    encoding='UTF-8'
    doctype-public='deck'
    doctype-system='http://granule.sourceforge.net/granule.dtd' />

  <xsl:strip-space elements='*' />

  <xsl:template match='front'>
    <front>
      <xsl:value-of select='.' />
    </front>
  </xsl:template>

  <xsl:template match='back'>
    <back>
      <xsl:value-of select='.' />
    </back>
    <back_example />
  </xsl:template>

  <xsl:template match='card'>
    <card>
      <xsl:attribute name='id' />
      <xsl:apply-templates select='front' />
      <xsl:apply-templates select='back' />
    </card>
  </xsl:template>

  <xsl:template match='/deck'>
    <deck>
      <author>
        <xsl:value-of select='@author' />
      </author>
      <description>
        <xsl:value-of select='@comment' />
      </description>
      <sound_path />
      <xsl:apply-templates />
    </deck>
  </xsl:template>

</xsl:stylesheet>
