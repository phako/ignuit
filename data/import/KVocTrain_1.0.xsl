<?xml version='1.0' encoding='UTF-8'?>
<!--

    I, the Copyright holder of this work, hereby release it into the Public
    Domain. This applies worldwide. In case this is not legally possible, I
    grant anyone the right to use this work for any purpose, without any
    conditions, unless such conditions are required by law.

    2012 Timothy Richard Musson <trmusson@gmail.com>

-->
<!--

  Convert a KDE KVocTrain file to an ignuit flashcard file.
  Command line usage:

  xsltproc -novalid -o outfile.xml KVocTrain.xsl infile.kvtml

  trm 2008-04-05

-->
<xsl:stylesheet version='1.0'
  xmlns:xsl='http://www.w3.org/1999/XSL/Transform'>

  <xsl:output method='xml' indent='yes' encoding='UTF-8' />

  <xsl:template match='o'>
    <front>
      <xsl:value-of select='.' />
    </front>
  </xsl:template>

  <xsl:template match='t'>
    <back>
      <xsl:value-of select='.' />
    </back>
  </xsl:template>

  <xsl:template match='e'>
    <card>
      <xsl:apply-templates select='o' />
      <xsl:apply-templates select='t' />
    </card>
  </xsl:template>

  <xsl:template match='/kvtml'>
    <deck>
      <xsl:attribute name='version'>
        <xsl:text>1</xsl:text>
      </xsl:attribute>
      <xsl:attribute name='title'>
        <xsl:text>Imported KVocTrain File</xsl:text>
      </xsl:attribute>
      <xsl:attribute name='style'>
        <xsl:text>1</xsl:text>
      </xsl:attribute>
      <category>
        <xsl:attribute name='title'>
          <xsl:text>Imported KVocTrain Cards</xsl:text>
        </xsl:attribute>
        <xsl:apply-templates />
      </category>
    </deck>
  </xsl:template>

</xsl:stylesheet>
