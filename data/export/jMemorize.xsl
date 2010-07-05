<?xml version='1.0' encoding='UTF-8'?>
<!--

    I, the Copyright holder of this work, hereby release it into the Public
    Domain. This applies worldwide. In case this is not legally possible, I
    grant anyone the right to use this work for any purpose, without any
    conditions, unless such conditions are required by law.

    2012 Timothy Richard Musson <trmusson@gmail.com>

-->
<!--

  Convert an ignuit flashcard file to a jMemorize flashcard file.
  Command line usage:

  xsltproc -novalid -o outfile.jml jMemorize.xsl infile.xml

  trm 2008-01-22

-->
<xsl:stylesheet version='1.0'
  xmlns:xsl='http://www.w3.org/1999/XSL/Transform'>

  <xsl:output
    method='xml'
    indent='yes'
    encoding='UTF-8'
    standalone='no' />

  <xsl:strip-space elements='*' />

  <xsl:template match='card'>
    <Card>
      <xsl:attribute name='Backside'>
        <xsl:value-of select='back' />
      </xsl:attribute>
      <xsl:attribute name='Frontside'>
        <xsl:value-of select='front' />
      </xsl:attribute>
    </Card>
  </xsl:template>

  <xsl:template match='category'>
    <Category>
      <xsl:attribute name='name'>
        <xsl:value-of select='@title' />
      </xsl:attribute>
      <Deck>
        <xsl:apply-templates />
      </Deck>
    </Category>
  </xsl:template>

  <xsl:template match='/deck'>
    <Lesson>
      <Category>
        <xsl:attribute name='name'>
          <xsl:text>All</xsl:text>
        </xsl:attribute>
        <Deck />
        <xsl:apply-templates />
      </Category>
    </Lesson>
  </xsl:template>

</xsl:stylesheet>
