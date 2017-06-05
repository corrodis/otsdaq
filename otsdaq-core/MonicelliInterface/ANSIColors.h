/****************************************************************************
** Authors: Dario Menasce, Stefano Terzo
**
** I.N.F.N. Milan-Bicocca
** Piazza  della Scienza 3, Edificio U2
** Milano, 20126
**
****************************************************************************/

#ifndef _ANSICOLORS_H
#define _ANSICOLORS_H

/*! \file ANSIColors.h
 *  \brief Header file containing macros to highlight output lines with color.
 *
 *  Check also <A HREF="http://www.bluesock.org/~willg/dev/ansi.html">http://www.bluesock.org/~willg/dev/ansi.html</A>
 *
 *  Below is a usage example:
 *  <p><b>
 *  \code
 *   #include <ANSIColors.h>"
 *   ...
 *   std::cout << __COUT_HDR_FL__ << ACRed << ACBold << "This text will be highlighted in red" << ACPlain << std::endl  << std::endl;
 *  \x1Bndcode
 *   </b>
 */

#define ACBlack       "\x1B[0;30m"
#define ACBlue        "\x1B[0;34m"
#define ACGreen       "\x1B[0;32m"
#define ACCyan        "\x1B[0;36m"
#define ACRed         "\x1B[0;31m"
#define ACPurple      "\x1B[0;35m"
#define ACBrown       "\x1B[0;33m"
#define ACGray        "\x1B[0;37m"
#define ACDarkGray    "\x1B[1;30m"
#define ACLightBlue   "\x1B[1;34m"
#define ACLightGreen  "\x1B[1;32m"
#define ACLightCyan   "\x1B[1;36m"
#define ACLightRed    "\x1B[1;31m"
#define ACLightPurple "\x1B[1;35m"
#define ACYellow      "\x1B[1;33m"
#define ACWhite       "\x1B[1;37m"

#define ACPlain       "\x1B[0m"
#define ACBold        "\x1B[1m"
#define ACUnderline   "\x1B[4m"
#define ACBlink       "\x1B[5m"
#define ACReverse     "\x1B[7m"

#define ACClear       "\x1B[2J"
#define ACClearL      "\x1B[2K"

#define ACCR          " \r"

#define ACSave        "\x1B[s"
#define ACRecall      "\x1B[u"

#endif // _ANSICOLORS_H
