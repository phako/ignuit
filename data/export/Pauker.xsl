<?xml version='1.0' encoding='UTF-8'?>
<!--

    I, the Copyright holder of this work, hereby release it into the Public
    Domain. This applies worldwide. In case this is not legally possible, I
    grant anyone the right to use this work for any purpose, without any
    conditions, unless such conditions are required by law.

    2012 Timothy Richard Musson <trmusson@gmail.com>

-->
<!--

  Convert an ignuit flashcard file to a Pauker flashcard file.
  Command line usage:

  xsltproc -novalid -o outfile.pau Pauker.xsl infile.xml

  trm 2008-01-22

-->
<xsl:stylesheet version='1.0'
  xmlns:xsl='http://www.w3.org/1999/XSL/Transform'>

  <xsl:output
    method='xml'
    indent='yes'
    encoding='UTF-8' />

  <xsl:strip-space elements='*' />

  <xsl:template match='front'>
    <FrontSide>
      <xsl:value-of select='.' />
    </FrontSide>
  </xsl:template>

  <xsl:template match='back'>
    <ReverseSide>
      <xsl:value-of select='.' />
    </ReverseSide>
  </xsl:template>

  <xsl:template match='card'>
    <Card>
      <xsl:apply-templates select='front' />
      <xsl:apply-templates select='back' />
    </Card>
  </xsl:template>

  <xsl:template match='/deck'>
    <Lesson LessonFormat='1.7'>
      <Description>
        <xsl:value-of select='@comment' />
      </Description>
      <Batch>
        <xsl:apply-templates />
      </Batch>
      <Batch/>
      <Batch/>
    </Lesson>
  </xsl:template>

</xsl:stylesheet>
