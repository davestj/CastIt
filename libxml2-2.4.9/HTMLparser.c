/*
 * HTMLparser.c : an HTML 4.0 non-verifying parser
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
 */

#include "libxml.h"
#ifdef LIBXML_HTML_ENABLED

#include <string.h>
#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_ZLIB_H
#include <zlib.h>
#endif

#include <libxml/xmlmemory.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/parserInternals.h>
#include <libxml/xmlerror.h>
#include <libxml/HTMLparser.h>
#include <libxml/HTMLtree.h>
#include <libxml/entities.h>
#include <libxml/encoding.h>
#include <libxml/valid.h>
#include <libxml/xmlIO.h>
#include <libxml/globals.h>

#define HTML_MAX_NAMELEN 1000
#define HTML_PARSER_BIG_BUFFER_SIZE 1000
#define HTML_PARSER_BUFFER_SIZE 100

/* #define DEBUG */
/* #define DEBUG_PUSH */

static int htmlOmittedDefaultValue = 1;

xmlChar * htmlDecodeEntities(htmlParserCtxtPtr ctxt, int len,
			     xmlChar end, xmlChar  end2, xmlChar end3);

/************************************************************************
 *									*
 * 		Parser stacks related functions and macros		*
 *									*
 ************************************************************************/

/*
 * Generic function for accessing stacks in the Parser Context
 */

#define PUSH_AND_POP(scope, type, name)					\
scope int html##name##Push(htmlParserCtxtPtr ctxt, type value) {	\
    if (ctxt->name##Nr >= ctxt->name##Max) {				\
	ctxt->name##Max *= 2;						\
        ctxt->name##Tab = (type *) xmlRealloc(ctxt->name##Tab,		\
	             ctxt->name##Max * sizeof(ctxt->name##Tab[0]));	\
        if (ctxt->name##Tab == NULL) {					\
	    xmlGenericError(xmlGenericErrorContext,			\
		    		"realloc failed !\n");			\
	    return(0);							\
	}								\
    }									\
    ctxt->name##Tab[ctxt->name##Nr] = value;				\
    ctxt->name = value;							\
    return(ctxt->name##Nr++);						\
}									\
scope type html##name##Pop(htmlParserCtxtPtr ctxt) {			\
    type ret;								\
    if (ctxt->name##Nr < 0) return(0);					\
    ctxt->name##Nr--;							\
    if (ctxt->name##Nr < 0) return(0);					\
    if (ctxt->name##Nr > 0)						\
	ctxt->name = ctxt->name##Tab[ctxt->name##Nr - 1];		\
    else								\
        ctxt->name = NULL;						\
    ret = ctxt->name##Tab[ctxt->name##Nr];				\
    ctxt->name##Tab[ctxt->name##Nr] = 0;				\
    return(ret);							\
}									\

/* PUSH_AND_POP(static, xmlNodePtr, node) */
PUSH_AND_POP(static, xmlChar*, name)

/*
 * Macros for accessing the content. Those should be used only by the parser,
 * and not exported.
 *
 * Dirty macros, i.e. one need to make assumption on the context to use them
 *
 *   CUR_PTR return the current pointer to the xmlChar to be parsed.
 *   CUR     returns the current xmlChar value, i.e. a 8 bit value if compiled
 *           in ISO-Latin or UTF-8, and the current 16 bit value if compiled
 *           in UNICODE mode. This should be used internally by the parser
 *           only to compare to ASCII values otherwise it would break when
 *           running with UTF-8 encoding.
 *   NXT(n)  returns the n'th next xmlChar. Same as CUR is should be used only
 *           to compare on ASCII based substring.
 *   UPP(n)  returns the n'th next xmlChar converted to uppercase. Same as CUR
 *           it should be used only to compare on ASCII based substring.
 *   SKIP(n) Skip n xmlChar, and must also be used only to skip ASCII defined
 *           strings within the parser.
 *
 * Clean macros, not dependent of an ASCII context, expect UTF-8 encoding
 *
 *   CURRENT Returns the current char value, with the full decoding of
 *           UTF-8 if we are using this mode. It returns an int.
 *   NEXT    Skip to the next character, this does the proper decoding
 *           in UTF-8 mode. It also pop-up unfinished entities on the fly.
 *   COPY(to) copy one char to *to, increment CUR_PTR and to accordingly
 */

#define UPPER (toupper(*ctxt->input->cur))

#define SKIP(val) ctxt->nbChars += (val),ctxt->input->cur += (val)

#define NXT(val) ctxt->input->cur[(val)]

#define UPP(val) (toupper(ctxt->input->cur[(val)]))

#define CUR_PTR ctxt->input->cur

#define SHRINK  xmlParserInputShrink(ctxt->input)

#define GROW  xmlParserInputGrow(ctxt->input, INPUT_CHUNK)

#define CURRENT ((int) (*ctxt->input->cur))

#define SKIP_BLANKS htmlSkipBlankChars(ctxt)

/* Inported from XML */

/* #define CUR (ctxt->token ? ctxt->token : (int) (*ctxt->input->cur)) */
#define CUR ((int) (*ctxt->input->cur))
#define NEXT xmlNextChar(ctxt),ctxt->nbChars++

#define RAW (ctxt->token ? -1 : (*ctxt->input->cur))
#define NXT(val) ctxt->input->cur[(val)]
#define CUR_PTR ctxt->input->cur


#define NEXTL(l) do {							\
    if (*(ctxt->input->cur) == '\n') {					\
	ctxt->input->line++; ctxt->input->col = 1;			\
    } else ctxt->input->col++;						\
    ctxt->token = 0; ctxt->input->cur += l; ctxt->nbChars++;		\
  } while (0)
    
/************
    \
    if (*ctxt->input->cur == '%') xmlParserHandlePEReference(ctxt);	\
    if (*ctxt->input->cur == '&') xmlParserHandleReference(ctxt);
 ************/

#define CUR_CHAR(l) htmlCurrentChar(ctxt, &l)
#define CUR_SCHAR(s, l) xmlStringCurrentChar(ctxt, s, &l)

#define COPY_BUF(l,b,i,v)						\
    if (l == 1) b[i++] = (xmlChar) v;					\
    else i += xmlCopyChar(l,&b[i],v)

/**
 * htmlCurrentChar:
 * @ctxt:  the HTML parser context
 * @len:  pointer to the length of the char read
 *
 * The current char value, if using UTF-8 this may actaully span multiple
 * bytes in the input buffer. Implement the end of line normalization:
 * 2.11 End-of-Line Handling
 * If the encoding is unspecified, in the case we find an ISO-Latin-1
 * char, then the encoding converter is plugged in automatically.
 *
 * Returns the current char value and its length
 */

static int
htmlCurrentChar(xmlParserCtxtPtr ctxt, int *len) {
    if (ctxt->instate == XML_PARSER_EOF)
	return(0);

    if (ctxt->token != 0) {
	*len = 0;
	return(ctxt->token);
    }	
    if (ctxt->charset == XML_CHAR_ENCODING_UTF8) {
	/*
	 * We are supposed to handle UTF8, check it's valid
	 * From rfc2044: encoding of the Unicode values on UTF-8:
	 *
	 * UCS-4 range (hex.)           UTF-8 octet sequence (binary)
	 * 0000 0000-0000 007F   0xxxxxxx
	 * 0000 0080-0000 07FF   110xxxxx 10xxxxxx
	 * 0000 0800-0000 FFFF   1110xxxx 10xxxxxx 10xxxxxx 
	 *
	 * Check for the 0x110000 limit too
	 */
	const unsigned char *cur = ctxt->input->cur;
	unsigned char c;
	unsigned int val;

	c = *cur;
	if (c & 0x80) {
	    if (cur[1] == 0)
		xmlParserInputGrow(ctxt->input, INPUT_CHUNK);
	    if ((cur[1] & 0xc0) != 0x80)
		goto encoding_error;
	    if ((c & 0xe0) == 0xe0) {

		if (cur[2] == 0)
		    xmlParserInputGrow(ctxt->input, INPUT_CHUNK);
		if ((cur[2] & 0xc0) != 0x80)
		    goto encoding_error;
		if ((c & 0xf0) == 0xf0) {
		    if (cur[3] == 0)
			xmlParserInputGrow(ctxt->input, INPUT_CHUNK);
		    if (((c & 0xf8) != 0xf0) ||
			((cur[3] & 0xc0) != 0x80))
			goto encoding_error;
		    /* 4-byte code */
		    *len = 4;
		    val = (cur[0] & 0x7) << 18;
		    val |= (cur[1] & 0x3f) << 12;
		    val |= (cur[2] & 0x3f) << 6;
		    val |= cur[3] & 0x3f;
		} else {
		  /* 3-byte code */
		    *len = 3;
		    val = (cur[0] & 0xf) << 12;
		    val |= (cur[1] & 0x3f) << 6;
		    val |= cur[2] & 0x3f;
		}
	    } else {
	      /* 2-byte code */
		*len = 2;
		val = (cur[0] & 0x1f) << 6;
		val |= cur[1] & 0x3f;
	    }
	    if (!IS_CHAR(val)) {
		ctxt->errNo = XML_ERR_INVALID_ENCODING;
		if ((ctxt->sax != NULL) &&
		    (ctxt->sax->error != NULL))
		    ctxt->sax->error(ctxt->userData, 
				     "Char 0x%X out of allowed range\n", val);
		ctxt->wellFormed = 0;
		ctxt->disableSAX = 1;
	    }    
	    return(val);
	} else {
	    /* 1-byte code */
	    *len = 1;
	    return((int) *ctxt->input->cur);
	}
    }
    /*
     * Assume it's a fixed length encoding (1) with
     * a compatibke encoding for the ASCII set, since
     * XML constructs only use < 128 chars
     */
    *len = 1;
    if ((int) *ctxt->input->cur < 0x80)
	return((int) *ctxt->input->cur);

    /*
     * Humm this is bad, do an automatic flow conversion
     */
    xmlSwitchEncoding(ctxt, XML_CHAR_ENCODING_8859_1);
    ctxt->charset = XML_CHAR_ENCODING_UTF8;
    return(xmlCurrentChar(ctxt, len));

encoding_error:
    /*
     * If we detect an UTF8 error that probably mean that the
     * input encoding didn't get properly advertized in the
     * declaration header. Report the error and switch the encoding
     * to ISO-Latin-1 (if you don't like this policy, just declare the
     * encoding !)
     */
    ctxt->errNo = XML_ERR_INVALID_ENCODING;
    if ((ctxt->sax != NULL) && (ctxt->sax->error != NULL)) {
	ctxt->sax->error(ctxt->userData, 
			 "Input is not proper UTF-8, indicate encoding !\n");
	ctxt->sax->error(ctxt->userData, "Bytes: 0x%02X 0x%02X 0x%02X 0x%02X\n",
			ctxt->input->cur[0], ctxt->input->cur[1],
			ctxt->input->cur[2], ctxt->input->cur[3]);
    }

    ctxt->charset = XML_CHAR_ENCODING_8859_1; 
    *len = 1;
    return((int) *ctxt->input->cur);
}

/**
 * htmlSkipBlankChars:
 * @ctxt:  the HTML parser context
 *
 * skip all blanks character found at that point in the input streams.
 *
 * Returns the number of space chars skipped
 */

static int
htmlSkipBlankChars(xmlParserCtxtPtr ctxt) {
    int res = 0;

    while (IS_BLANK(*(ctxt->input->cur))) {
	if ((*ctxt->input->cur == 0) &&
	    (xmlParserInputGrow(ctxt->input, INPUT_CHUNK) <= 0)) {
		xmlPopInput(ctxt);
	} else {
	    if (*(ctxt->input->cur) == '\n') {
		ctxt->input->line++; ctxt->input->col = 1;
	    } else ctxt->input->col++;
	    ctxt->input->cur++;
	    ctxt->nbChars++;
	    if (*ctxt->input->cur == 0)
		xmlParserInputGrow(ctxt->input, INPUT_CHUNK);
	}
	res++;
    }
    return(res);
}



/************************************************************************
 *									*
 * 		The list of HTML elements and their properties		*
 *									*
 ************************************************************************/

/*
 *  Start Tag: 1 means the start tag can be ommited
 *  End Tag:   1 means the end tag can be ommited
 *             2 means it's forbidden (empty elements)
 *             3 means the tag is stylistic and should be closed easilly
 *  Depr:      this element is deprecated
 *  DTD:       1 means that this element is valid only in the Loose DTD
 *             2 means that this element is valid only in the Frameset DTD
 *
 * Name,Start Tag,End Tag,Save End,Empty,Deprecated,DTD,inline,Description
 */
static const htmlElemDesc
html40ElementTable[] = {
{ "a",		0, 0, 0, 0, 0, 0, 1, "anchor " },
{ "abbr",	0, 0, 0, 0, 0, 0, 1, "abbreviated form" },
{ "acronym",	0, 0, 0, 0, 0, 0, 1, "" },
{ "address",	0, 0, 0, 0, 0, 0, 0, "information on author " },
{ "applet",	0, 0, 0, 0, 1, 1, 2, "java applet " },
{ "area",	0, 2, 2, 1, 0, 0, 0, "client-side image map area " },
{ "b",		0, 3, 0, 0, 0, 0, 1, "bold text style" },
{ "base",	0, 2, 2, 1, 0, 0, 0, "document base uri " },
{ "basefont",	0, 2, 2, 1, 1, 1, 1, "base font size " },
{ "bdo",	0, 0, 0, 0, 0, 0, 1, "i18n bidi over-ride " },
{ "big",	0, 3, 0, 0, 0, 0, 1, "large text style" },
{ "blockquote",	0, 0, 0, 0, 0, 0, 0, "long quotation " },
{ "body",	1, 1, 0, 0, 0, 0, 0, "document body " },
{ "br",		0, 2, 2, 1, 0, 0, 1, "forced line break " },
{ "button",	0, 0, 0, 0, 0, 0, 2, "push button " },
{ "caption",	0, 0, 0, 0, 0, 0, 0, "table caption " },
{ "center",	0, 3, 0, 0, 1, 1, 0, "shorthand for div align=center " },
{ "cite",	0, 0, 0, 0, 0, 0, 1, "citation" },
{ "code",	0, 0, 0, 0, 0, 0, 1, "computer code fragment" },
{ "col",	0, 2, 2, 1, 0, 0, 0, "table column " },
{ "colgroup",	0, 1, 0, 0, 0, 0, 0, "table column group " },
{ "dd",		0, 1, 0, 0, 0, 0, 0, "definition description " },
{ "del",	0, 0, 0, 0, 0, 0, 2, "deleted text " },
{ "dfn",	0, 0, 0, 0, 0, 0, 1, "instance definition" },
{ "dir",	0, 0, 0, 0, 1, 1, 0, "directory list" },
{ "div",	0, 0, 0, 0, 0, 0, 0, "generic language/style container"},
{ "dl",		0, 0, 0, 0, 0, 0, 0, "definition list " },
{ "dt",		0, 1, 0, 0, 0, 0, 0, "definition term " },
{ "em",		0, 3, 0, 0, 0, 0, 1, "emphasis" },
{ "fieldset",	0, 0, 0, 0, 0, 0, 0, "form control group " },
{ "font",	0, 3, 0, 0, 1, 1, 1, "local change to font " },
{ "form",	0, 0, 0, 0, 0, 0, 0, "interactive form " },
{ "frame",	0, 2, 2, 1, 0, 2, 0, "subwindow " },
{ "frameset",	0, 0, 0, 0, 0, 2, 0, "window subdivision" },
{ "h1",		0, 0, 0, 0, 0, 0, 0, "heading " },
{ "h2",		0, 0, 0, 0, 0, 0, 0, "heading " },
{ "h3",		0, 0, 0, 0, 0, 0, 0, "heading " },
{ "h4",		0, 0, 0, 0, 0, 0, 0, "heading " },
{ "h5",		0, 0, 0, 0, 0, 0, 0, "heading " },
{ "h6",		0, 0, 0, 0, 0, 0, 0, "heading " },
{ "head",	1, 1, 0, 0, 0, 0, 0, "document head " },
{ "hr",		0, 2, 2, 1, 0, 0, 0, "horizontal rule " },
{ "html",	1, 1, 0, 0, 0, 0, 0, "document root element " },
{ "i",		0, 3, 0, 0, 0, 0, 1, "italic text style" },
{ "iframe",	0, 0, 0, 0, 0, 1, 2, "inline subwindow " },
{ "img",	0, 2, 2, 1, 0, 0, 1, "embedded image " },
{ "input",	0, 2, 2, 1, 0, 0, 1, "form control " },
{ "ins",	0, 0, 0, 0, 0, 0, 2, "inserted text" },
{ "isindex",	0, 2, 2, 1, 1, 1, 0, "single line prompt " },
{ "kbd",	0, 0, 0, 0, 0, 0, 1, "text to be entered by the user" },
{ "label",	0, 0, 0, 0, 0, 0, 1, "form field label text " },
{ "legend",	0, 0, 0, 0, 0, 0, 0, "fieldset legend " },
{ "li",		0, 1, 1, 0, 0, 0, 0, "list item " },
{ "link",	0, 2, 2, 1, 0, 0, 0, "a media-independent link " },
{ "map",	0, 0, 0, 0, 0, 0, 2, "client-side image map " },
{ "menu",	0, 0, 0, 0, 1, 1, 0, "menu list " },
{ "meta",	0, 2, 2, 1, 0, 0, 0, "generic metainformation " },
{ "noframes",	0, 0, 0, 0, 0, 2, 0, "alternate content container for non frame-based rendering " },
{ "noscript",	0, 0, 0, 0, 0, 0, 0, "alternate content container for non script-based rendering " },
{ "object",	0, 0, 0, 0, 0, 0, 2, "generic embedded object " },
{ "ol",		0, 0, 0, 0, 0, 0, 0, "ordered list " },
{ "optgroup",	0, 0, 0, 0, 0, 0, 0, "option group " },
{ "option",	0, 1, 0, 0, 0, 0, 0, "selectable choice " },
{ "p",		0, 1, 1, 0, 0, 0, 0, "paragraph " },
{ "param",	0, 2, 2, 1, 0, 0, 0, "named property value " },
{ "pre",	0, 0, 0, 0, 0, 0, 0, "preformatted text " },
{ "q",		0, 0, 0, 0, 0, 0, 1, "short inline quotation " },
{ "s",		0, 3, 0, 0, 1, 1, 1, "strike-through text style" },
{ "samp",	0, 0, 0, 0, 0, 0, 1, "sample program output, scripts, etc." },
{ "script",	0, 0, 0, 0, 0, 0, 2, "script statements " },
{ "select",	0, 0, 0, 0, 0, 0, 1, "option selector " },
{ "small",	0, 3, 0, 0, 0, 0, 1, "small text style" },
{ "span",	0, 0, 0, 0, 0, 0, 1, "generic language/style container " },
{ "strike",	0, 3, 0, 0, 1, 1, 1, "strike-through text" },
{ "strong",	0, 3, 0, 0, 0, 0, 1, "strong emphasis" },
{ "style",	0, 0, 0, 0, 0, 0, 0, "style info " },
{ "sub",	0, 3, 0, 0, 0, 0, 1, "subscript" },
{ "sup",	0, 3, 0, 0, 0, 0, 1, "superscript " },
{ "table",	0, 0, 0, 0, 0, 0, 0, "&#160;" },
{ "tbody",	1, 0, 0, 0, 0, 0, 0, "table body " },
{ "td",		0, 0, 0, 0, 0, 0, 0, "table data cell" },
{ "textarea",	0, 0, 0, 0, 0, 0, 1, "multi-line text field " },
{ "tfoot",	0, 1, 0, 0, 0, 0, 0, "table footer " },
{ "th",		0, 1, 0, 0, 0, 0, 0, "table header cell" },
{ "thead",	0, 1, 0, 0, 0, 0, 0, "table header " },
{ "title",	0, 0, 0, 0, 0, 0, 0, "document title " },
{ "tr",		0, 0, 0, 0, 0, 0, 0, "table row " },
{ "tt",		0, 3, 0, 0, 0, 0, 1, "teletype or monospaced text style" },
{ "u",		0, 3, 0, 0, 1, 1, 1, "underlined text style" },
{ "ul",		0, 0, 0, 0, 0, 0, 0, "unordered list " },
{ "var",	0, 0, 0, 0, 0, 0, 1, "instance of a variable or program argument" },
};

/*
 * start tags that imply the end of current element
 */
static const char *htmlStartClose[] = {
"form",		"form", "p", "hr", "h1", "h2", "h3", "h4", "h5", "h6",
		"dl", "ul", "ol", "menu", "dir", "address", "pre",
		"listing", "xmp", "head", NULL,
"head",		"p", NULL,
"title",	"p", NULL,
"body",		"head", "style", "link", "title", "p", NULL,
"li",		"p", "h1", "h2", "h3", "h4", "h5", "h6", "dl", "address",
		"pre", "listing", "xmp", "head", "li", NULL,
"hr",		"p", "head", NULL,
"h1",		"p", "head", NULL,
"h2",		"p", "head", NULL,
"h3",		"p", "head", NULL,
"h4",		"p", "head", NULL,
"h5",		"p", "head", NULL,
"h6",		"p", "head", NULL,
"dir",		"p", "head", NULL,
"address",	"p", "head", "ul", NULL,
"pre",		"p", "head", "ul", NULL,
"listing",	"p", "head", NULL,
"xmp",		"p", "head", NULL,
"blockquote",	"p", "head", NULL,
"dl",		"p", "dt", "menu", "dir", "address", "pre", "listing",
		"xmp", "head", NULL,
"dt",		"p", "menu", "dir", "address", "pre", "listing", "xmp",
                "head", "dd", NULL,
"dd",		"p", "menu", "dir", "address", "pre", "listing", "xmp",
                "head", "dt", NULL,
"ul",		"p", "head", "ol", "menu", "dir", "address", "pre",
		"listing", "xmp", NULL,
"ol",		"p", "head", "ul", NULL,
"menu",		"p", "head", "ul", NULL,
"p",		"p", "head", "h1", "h2", "h3", "h4", "h5", "h6", NULL,
"div",		"p", "head", NULL,
"noscript",	"p", "head", NULL,
"center",	"font", "b", "i", "p", "head", NULL,
"a",		"a", NULL,
"caption",	"p", NULL,
"colgroup",	"caption", "colgroup", "col", "p", NULL,
"col",		"caption", "col", "p", NULL,
"table",	"p", "head", "h1", "h2", "h3", "h4", "h5", "h6", "pre",
		"listing", "xmp", "a", NULL,
"th",		"th", "td", "p", "span", "font", "a", "b", "i", "u", NULL,
"td",		"th", "td", "p", "span", "font", "a", "b", "i", "u", NULL,      
"tr",		"th", "td", "tr", "caption", "col", "colgroup", "p", NULL,
"thead",	"caption", "col", "colgroup", NULL,
"tfoot",	"th", "td", "tr", "caption", "col", "colgroup", "thead",
		"tbody", "p", NULL,
"tbody",	"th", "td", "tr", "caption", "col", "colgroup", "thead",
		"tfoot", "tbody", "p", NULL,
"optgroup",	"option", NULL,
"option",	"option", NULL,
"fieldset",	"legend", "p", "head", "h1", "h2", "h3", "h4", "h5", "h6",
		"pre", "listing", "xmp", "a", NULL,
NULL
};

/*
 * The list of HTML elements which are supposed not to have
 * CDATA content and where a p element will be implied
 *
 * TODO: extend that list by reading the HTML SGML DtD on
 *       implied paragraph
 */
static const char *htmlNoContentElements[] = {
    "html",
    "head",
    "body",
    NULL
};

/*
 * The list of HTML attributes which are of content %Script;
 * NOTE: when adding ones, check htmlIsScriptAttribute() since
 *       it assumes the name starts with 'on'
 */
static const char *htmlScriptAttributes[] = {
    "onclick",
    "ondblclick",
    "onmousedown",
    "onmouseup",
    "onmouseover",
    "onmousemove",
    "onmouseout",
    "onkeypress",
    "onkeydown",
    "onkeyup",
    "onload",
    "onunload",
    "onfocus",
    "onblur",
    "onsubmit",
    "onrest",
    "onchange",
    "onselect"
};

/*
 * This table is used by the htmlparser to know what to do with
 * broken html pages. By assigning different priorities to different
 * elements the parser can decide how to handle extra endtags.
 * Endtags are only allowed to close elements with lower or equal
 * priority.
 */ 

typedef struct {
    const char *name;
    int priority;
} elementPriority;

static const elementPriority htmlEndPriority[] = {
    {"div",   150},
    {"td",    160},
    {"th",    160},
    {"tr",    170},
    {"thead", 180},
    {"tbody", 180},
    {"tfoot", 180},
    {"table", 190},
    {"head",  200},
    {"body",  200},
    {"html",  220},
    {NULL,    100} /* Default priority */
};

static const char** htmlStartCloseIndex[100];
static int htmlStartCloseIndexinitialized = 0;

/************************************************************************
 *									*
 * 		functions to handle HTML specific data			*
 *									*
 ************************************************************************/

/**
 * htmlInitAutoClose:
 *
 * Initialize the htmlStartCloseIndex for fast lookup of closing tags names.
 * This is not reentrant. Call xmlInitParser() once before processing in
 * case of use in multithreaded programs.
 */
void
htmlInitAutoClose(void) {
    int indx, i = 0;

    if (htmlStartCloseIndexinitialized) return;

    for (indx = 0;indx < 100;indx ++) htmlStartCloseIndex[indx] = NULL;
    indx = 0;
    while ((htmlStartClose[i] != NULL) && (indx < 100 - 1)) {
        htmlStartCloseIndex[indx++] = &htmlStartClose[i];
	while (htmlStartClose[i] != NULL) i++;
	i++;
    }
    htmlStartCloseIndexinitialized = 1;
}

/**
 * htmlTagLookup:
 * @tag:  The tag name in lowercase
 *
 * Lookup the HTML tag in the ElementTable
 *
 * Returns the related htmlElemDescPtr or NULL if not found.
 */
const htmlElemDesc *
htmlTagLookup(const xmlChar *tag) {
    unsigned int i;

    for (i = 0; i < (sizeof(html40ElementTable) /
                     sizeof(html40ElementTable[0]));i++) {
        if (!xmlStrcasecmp(tag, BAD_CAST html40ElementTable[i].name))
	    return((const htmlElemDescPtr) (const htmlElemDescPtr) (const htmlElemDescPtr) (const htmlElemDescPtr) (const htmlElemDescPtr) (const htmlElemDescPtr) (const htmlElemDescPtr) (const htmlElemDescPtr) (const htmlElemDescPtr) &html40ElementTable[i]);
    }
    return(NULL);
}

/**
 * htmlGetEndPriority:
 * @name: The name of the element to look up the priority for.
 * 
 * Return value: The "endtag" priority.
 **/
static int
htmlGetEndPriority (const xmlChar *name) {
	int i = 0;

	while ((htmlEndPriority[i].name != NULL) &&
	       (!xmlStrEqual((const xmlChar *)htmlEndPriority[i].name, name)))
	    i++;

	return(htmlEndPriority[i].priority);
}

/**
 * htmlCheckAutoClose:
 * @newtag:  The new tag name
 * @oldtag:  The old tag name
 *
 * Checks wether the new tag is one of the registered valid tags for closing old.
 * Initialize the htmlStartCloseIndex for fast lookup of closing tags names.
 *
 * Returns 0 if no, 1 if yes.
 */
static int
htmlCheckAutoClose(const xmlChar *newtag, const xmlChar *oldtag) {
    int i, indx;
    const char **closed = NULL;

    if (htmlStartCloseIndexinitialized == 0) htmlInitAutoClose();

    /* inefficient, but not a big deal */
    for (indx = 0; indx < 100;indx++) {
        closed = htmlStartCloseIndex[indx];
	if (closed == NULL) return(0);
	if (xmlStrEqual(BAD_CAST *closed, newtag)) break;
    }

    i = closed - htmlStartClose;
    i++;
    while (htmlStartClose[i] != NULL) {
        if (xmlStrEqual(BAD_CAST htmlStartClose[i], oldtag)) {
	    return(1);
	}
	i++;
    }
    return(0);
}

/**
 * htmlAutoCloseOnClose:
 * @ctxt:  an HTML parser context
 * @newtag:  The new tag name
 * @force:  force the tag closure
 *
 * The HTmL DtD allows an ending tag to implicitely close other tags.
 */
static void
htmlAutoCloseOnClose(htmlParserCtxtPtr ctxt, const xmlChar *newtag) {
    const htmlElemDesc * info;
    xmlChar *oldname;
    int i, priority;

#ifdef DEBUG
    xmlGenericError(xmlGenericErrorContext,"Close of %s stack: %d elements\n", newtag, ctxt->nameNr);
    for (i = 0;i < ctxt->nameNr;i++) 
        xmlGenericError(xmlGenericErrorContext,"%d : %s\n", i, ctxt->nameTab[i]);
#endif

    priority = htmlGetEndPriority (newtag);

    for (i = (ctxt->nameNr - 1);i >= 0;i--) {

        if (xmlStrEqual(newtag, ctxt->nameTab[i])) break;
	/*
	 * A missplaced endtagad can only close elements with lower
	 * or equal priority, so if we find an element with higher
	 * priority before we find an element with
	 * matching name, we just ignore this endtag 
	 */
	if (htmlGetEndPriority (ctxt->nameTab[i]) > priority) return;
    }
    if (i < 0) return;

    while (!xmlStrEqual(newtag, ctxt->name)) {
	info = htmlTagLookup(ctxt->name);
	if ((info == NULL) || (info->endTag == 1)) {
#ifdef DEBUG
	    xmlGenericError(xmlGenericErrorContext,"htmlAutoCloseOnClose: %s closes %s\n", newtag, ctxt->name);
#endif
        } else if (info->endTag == 3) {
#ifdef DEBUG
	    xmlGenericError(xmlGenericErrorContext,"End of tag %s: expecting %s\n", newtag, ctxt->name);

#endif
	    if ((ctxt->sax != NULL) && (ctxt->sax->error != NULL))
		ctxt->sax->error(ctxt->userData,
		 "Opening and ending tag mismatch: %s and %s\n",
				 newtag, ctxt->name);
	    ctxt->wellFormed = 0;
	}
	if ((ctxt->sax != NULL) && (ctxt->sax->endElement != NULL))
	    ctxt->sax->endElement(ctxt->userData, ctxt->name);
	oldname = htmlnamePop(ctxt);
	if (oldname != NULL) {
#ifdef DEBUG
	    xmlGenericError(xmlGenericErrorContext,"htmlAutoCloseOnClose: popped %s\n", oldname);
#endif
	    xmlFree(oldname);
	}	
    }
}

/**
 * htmlAutoCloseOnEnd:
 * @ctxt:  an HTML parser context
 *
 * Close all remaining tags at the end of the stream
 */
static void
htmlAutoCloseOnEnd(htmlParserCtxtPtr ctxt) {
    xmlChar *oldname;
    int i;

    if (ctxt->nameNr == 0)
	return;
#ifdef DEBUG
    xmlGenericError(xmlGenericErrorContext,"Close of stack: %d elements\n", ctxt->nameNr);
#endif

    for (i = (ctxt->nameNr - 1);i >= 0;i--) {
#ifdef DEBUG
        xmlGenericError(xmlGenericErrorContext,"%d : %s\n", i, ctxt->nameTab[i]);
#endif
	if ((ctxt->sax != NULL) && (ctxt->sax->endElement != NULL))
	    ctxt->sax->endElement(ctxt->userData, ctxt->name);
	oldname = htmlnamePop(ctxt);
	if (oldname != NULL) {
#ifdef DEBUG
	    xmlGenericError(xmlGenericErrorContext,"htmlAutoCloseOnEnd: popped %s\n", oldname);
#endif
	    xmlFree(oldname);
	}	
    }
}

/**
 * htmlAutoClose:
 * @ctxt:  an HTML parser context
 * @newtag:  The new tag name or NULL
 *
 * The HTmL DtD allows a tag to implicitely close other tags.
 * The list is kept in htmlStartClose array. This function is
 * called when a new tag has been detected and generates the
 * appropriates closes if possible/needed.
 * If newtag is NULL this mean we are at the end of the resource
 * and we should check 
 */
static void
htmlAutoClose(htmlParserCtxtPtr ctxt, const xmlChar *newtag) {
    xmlChar *oldname;
    while ((newtag != NULL) && (ctxt->name != NULL) && 
           (htmlCheckAutoClose(newtag, ctxt->name))) {
#ifdef DEBUG
	xmlGenericError(xmlGenericErrorContext,"htmlAutoClose: %s closes %s\n", newtag, ctxt->name);
#endif
	if ((ctxt->sax != NULL) && (ctxt->sax->endElement != NULL))
	    ctxt->sax->endElement(ctxt->userData, ctxt->name);
	oldname = htmlnamePop(ctxt);
	if (oldname != NULL) {
#ifdef DEBUG
	    xmlGenericError(xmlGenericErrorContext,"htmlAutoClose: popped %s\n", oldname);
#endif
	    xmlFree(oldname);
        }
    }
    if (newtag == NULL) {
	htmlAutoCloseOnEnd(ctxt);
	return;
    }
    while ((newtag == NULL) && (ctxt->name != NULL) &&
	   ((xmlStrEqual(ctxt->name, BAD_CAST"head")) ||
	    (xmlStrEqual(ctxt->name, BAD_CAST"body")) ||
	    (xmlStrEqual(ctxt->name, BAD_CAST"html")))) {
#ifdef DEBUG
	xmlGenericError(xmlGenericErrorContext,"htmlAutoClose: EOF closes %s\n", ctxt->name);
#endif
	if ((ctxt->sax != NULL) && (ctxt->sax->endElement != NULL))
	    ctxt->sax->endElement(ctxt->userData, ctxt->name);
	oldname = htmlnamePop(ctxt);
	if (oldname != NULL) {
#ifdef DEBUG
	    xmlGenericError(xmlGenericErrorContext,"htmlAutoClose: popped %s\n", oldname);
#endif
	    xmlFree(oldname);
        }
   }

}

/**
 * htmlAutoCloseTag:
 * @doc:  the HTML document
 * @name:  The tag name
 * @elem:  the HTML element
 *
 * The HTmL DtD allows a tag to implicitely close other tags.
 * The list is kept in htmlStartClose array. This function checks
 * if the element or one of it's children would autoclose the
 * given tag.
 *
 * Returns 1 if autoclose, 0 otherwise
 */
int
htmlAutoCloseTag(htmlDocPtr doc, const xmlChar *name, htmlNodePtr elem) {
    htmlNodePtr child;

    if (elem == NULL) return(1);
    if (xmlStrEqual(name, elem->name)) return(0);
    if (htmlCheckAutoClose(elem->name, name)) return(1);
    child = elem->children;
    while (child != NULL) {
        if (htmlAutoCloseTag(doc, name, child)) return(1);
	child = child->next;
    }
    return(0);
}

/**
 * htmlIsAutoClosed:
 * @doc:  the HTML document
 * @elem:  the HTML element
 *
 * The HTmL DtD allows a tag to implicitely close other tags.
 * The list is kept in htmlStartClose array. This function checks
 * if a tag is autoclosed by one of it's child
 *
 * Returns 1 if autoclosed, 0 otherwise
 */
int
htmlIsAutoClosed(htmlDocPtr doc, htmlNodePtr elem) {
    htmlNodePtr child;

    if (elem == NULL) return(1);
    child = elem->children;
    while (child != NULL) {
	if (htmlAutoCloseTag(doc, elem->name, child)) return(1);
	child = child->next;
    }
    return(0);
}

/**
 * htmlCheckImplied:
 * @ctxt:  an HTML parser context
 * @newtag:  The new tag name
 *
 * The HTML DtD allows a tag to exists only implicitely
 * called when a new tag has been detected and generates the
 * appropriates implicit tags if missing
 */
static void
htmlCheckImplied(htmlParserCtxtPtr ctxt, const xmlChar *newtag) {
    if (!htmlOmittedDefaultValue)
	return;
    if (xmlStrEqual(newtag, BAD_CAST"html"))
	return;
    if (ctxt->nameNr <= 0) {
#ifdef DEBUG
	xmlGenericError(xmlGenericErrorContext,"Implied element html: pushed html\n");
#endif    
	htmlnamePush(ctxt, xmlStrdup(BAD_CAST"html"));
	if ((ctxt->sax != NULL) && (ctxt->sax->startElement != NULL))
	    ctxt->sax->startElement(ctxt->userData, BAD_CAST"html", NULL);
    }
    if ((xmlStrEqual(newtag, BAD_CAST"body")) || (xmlStrEqual(newtag, BAD_CAST"head")))
        return;
    if ((ctxt->nameNr <= 1) && 
        ((xmlStrEqual(newtag, BAD_CAST"script")) ||
	 (xmlStrEqual(newtag, BAD_CAST"style")) ||
	 (xmlStrEqual(newtag, BAD_CAST"meta")) ||
	 (xmlStrEqual(newtag, BAD_CAST"link")) ||
	 (xmlStrEqual(newtag, BAD_CAST"title")) ||
	 (xmlStrEqual(newtag, BAD_CAST"base")))) {
	    /* 
	     * dropped OBJECT ... i you put it first BODY will be
	     * assumed !
	     */
#ifdef DEBUG
	    xmlGenericError(xmlGenericErrorContext,"Implied element head: pushed head\n");
#endif    
	    htmlnamePush(ctxt, xmlStrdup(BAD_CAST"head"));
	    if ((ctxt->sax != NULL) && (ctxt->sax->startElement != NULL))
		ctxt->sax->startElement(ctxt->userData, BAD_CAST"head", NULL);
    } else if ((!xmlStrEqual(newtag, BAD_CAST"noframes")) &&
	       (!xmlStrEqual(newtag, BAD_CAST"frame")) &&
	       (!xmlStrEqual(newtag, BAD_CAST"frameset"))) {
	int i;
	for (i = 0;i < ctxt->nameNr;i++) {
	    if (xmlStrEqual(ctxt->nameTab[i], BAD_CAST"body")) {
		return;
	    }
	    if (xmlStrEqual(ctxt->nameTab[i], BAD_CAST"head")) {
		return;
	    }
	}
	    
#ifdef DEBUG
	xmlGenericError(xmlGenericErrorContext,"Implied element body: pushed body\n");
#endif    
	htmlnamePush(ctxt, xmlStrdup(BAD_CAST"body"));
	if ((ctxt->sax != NULL) && (ctxt->sax->startElement != NULL))
	    ctxt->sax->startElement(ctxt->userData, BAD_CAST"body", NULL);
    }
}

/**
 * htmlCheckParagraph
 * @ctxt:  an HTML parser context
 *
 * Check whether a p element need to be implied before inserting
 * characters in the current element.
 *
 * Returns 1 if a paragraph has been inserted, 0 if not and -1
 *         in case of error.
 */

static int
htmlCheckParagraph(htmlParserCtxtPtr ctxt) {
    const xmlChar *tag;
    int i;

    if (ctxt == NULL)
	return(-1);
    tag = ctxt->name;
    if (tag == NULL) {
	htmlAutoClose(ctxt, BAD_CAST"p");
	htmlCheckImplied(ctxt, BAD_CAST"p");
	htmlnamePush(ctxt, xmlStrdup(BAD_CAST"p"));
	if ((ctxt->sax != NULL) && (ctxt->sax->startElement != NULL))
	    ctxt->sax->startElement(ctxt->userData, BAD_CAST"p", NULL);
	return(1);
    }
    if (!htmlOmittedDefaultValue)
	return(0);
    for (i = 0; htmlNoContentElements[i] != NULL; i++) {
	if (xmlStrEqual(tag, BAD_CAST htmlNoContentElements[i])) {
#ifdef DEBUG
	    xmlGenericError(xmlGenericErrorContext,"Implied element paragraph\n");
#endif    
	    htmlAutoClose(ctxt, BAD_CAST"p");
	    htmlCheckImplied(ctxt, BAD_CAST"p");
	    htmlnamePush(ctxt, xmlStrdup(BAD_CAST"p"));
	    if ((ctxt->sax != NULL) && (ctxt->sax->startElement != NULL))
		ctxt->sax->startElement(ctxt->userData, BAD_CAST"p", NULL);
	    return(1);
	}
    }
    return(0);
}

/**
 * htmlIsScriptAttribute:
 * @name:  an attribute name
 *
 * Check if an attribute is of content type Script
 *
 * Returns 1 is the attribute is a script 0 otherwise
 */
int
htmlIsScriptAttribute(const xmlChar *name) {
    unsigned int i;

    if (name == NULL)
       	return(0);
    /*
     * all script attributes start with 'on'
     */
    if ((name[0] != 'o') || (name[1] != 'n'))
       	return(0);
    for (i = 0;
	 i < sizeof(htmlScriptAttributes)/sizeof(htmlScriptAttributes[0]);
	 i++) {
	if (xmlStrEqual(name, (const xmlChar *) htmlScriptAttributes[i]))
	    return(1);
    }
    return(0);
}

/************************************************************************
 *									*
 * 		The list of HTML predefined entities			*
 *									*
 ************************************************************************/


static const htmlEntityDesc  html40EntitiesTable[] = {
/*
 * the 4 absolute ones, plus apostrophe.
 */
{ 34,	"quot",	"quotation mark = APL quote, U+0022 ISOnum" },
{ 38,	"amp",	"ampersand, U+0026 ISOnum" },
{ 39,	"apos",	"single quote" },
{ 60,	"lt",	"less-than sign, U+003C ISOnum" },
{ 62,	"gt",	"greater-than sign, U+003E ISOnum" },

/*
 * A bunch still in the 128-255 range
 * Replacing them depend really on the charset used.
 */
{ 160,	"nbsp",	"no-break space = non-breaking space, U+00A0 ISOnum" },
{ 161,	"iexcl","inverted exclamation mark, U+00A1 ISOnum" },
{ 162,	"cent",	"cent sign, U+00A2 ISOnum" },
{ 163,	"pound","pound sign, U+00A3 ISOnum" },
{ 164,	"curren","currency sign, U+00A4 ISOnum" },
{ 165,	"yen",	"yen sign = yuan sign, U+00A5 ISOnum" },
{ 166,	"brvbar","broken bar = broken vertical bar, U+00A6 ISOnum" },
{ 167,	"sect",	"section sign, U+00A7 ISOnum" },
{ 168,	"uml",	"diaeresis = spacing diaeresis, U+00A8 ISOdia" },
{ 169,	"copy",	"copyright sign, U+00A9 ISOnum" },
{ 170,	"ordf",	"feminine ordinal indicator, U+00AA ISOnum" },
{ 171,	"laquo","left-pointing double angle quotation mark = left pointing guillemet, U+00AB ISOnum" },
{ 172,	"not",	"not sign, U+00AC ISOnum" },
{ 173,	"shy",	"soft hyphen = discretionary hyphen, U+00AD ISOnum" },
{ 174,	"reg",	"registered sign = registered trade mark sign, U+00AE ISOnum" },
{ 175,	"macr",	"macron = spacing macron = overline = APL overbar, U+00AF ISOdia" },
{ 176,	"deg",	"degree sign, U+00B0 ISOnum" },
{ 177,	"plusmn","plus-minus sign = plus-or-minus sign, U+00B1 ISOnum" },
{ 178,	"sup2",	"superscript two = superscript digit two = squared, U+00B2 ISOnum" },
{ 179,	"sup3",	"superscript three = superscript digit three = cubed, U+00B3 ISOnum" },
{ 180,	"acute","acute accent = spacing acute, U+00B4 ISOdia" },
{ 181,	"micro","micro sign, U+00B5 ISOnum" },
{ 182,	"para",	"pilcrow sign = paragraph sign, U+00B6 ISOnum" },
{ 183,	"middot","middle dot = Georgian comma Greek middle dot, U+00B7 ISOnum" },
{ 184,	"cedil","cedilla = spacing cedilla, U+00B8 ISOdia" },
{ 185,	"sup1",	"superscript one = superscript digit one, U+00B9 ISOnum" },
{ 186,	"ordm",	"masculine ordinal indicator, U+00BA ISOnum" },
{ 187,	"raquo","right-pointing double angle quotation mark right pointing guillemet, U+00BB ISOnum" },
{ 188,	"frac14","vulgar fraction one quarter = fraction one quarter, U+00BC ISOnum" },
{ 189,	"frac12","vulgar fraction one half = fraction one half, U+00BD ISOnum" },
{ 190,	"frac34","vulgar fraction three quarters = fraction three quarters, U+00BE ISOnum" },
{ 191,	"iquest","inverted question mark = turned question mark, U+00BF ISOnum" },
{ 192,	"Agrave","latin capital letter A with grave = latin capital letter A grave, U+00C0 ISOlat1" },
{ 193,	"Aacute","latin capital letter A with acute, U+00C1 ISOlat1" },
{ 194,	"Acirc","latin capital letter A with circumflex, U+00C2 ISOlat1" },
{ 195,	"Atilde","latin capital letter A with tilde, U+00C3 ISOlat1" },
{ 196,	"Auml",	"latin capital letter A with diaeresis, U+00C4 ISOlat1" },
{ 197,	"Aring","latin capital letter A with ring above = latin capital letter A ring, U+00C5 ISOlat1" },
{ 198,	"AElig","latin capital letter AE = latin capital ligature AE, U+00C6 ISOlat1" },
{ 199,	"Ccedil","latin capital letter C with cedilla, U+00C7 ISOlat1" },
{ 200,	"Egrave","latin capital letter E with grave, U+00C8 ISOlat1" },
{ 201,	"Eacute","latin capital letter E with acute, U+00C9 ISOlat1" },
{ 202,	"Ecirc","latin capital letter E with circumflex, U+00CA ISOlat1" },
{ 203,	"Euml",	"latin capital letter E with diaeresis, U+00CB ISOlat1" },
{ 204,	"Igrave","latin capital letter I with grave, U+00CC ISOlat1" },
{ 205,	"Iacute","latin capital letter I with acute, U+00CD ISOlat1" },
{ 206,	"Icirc","latin capital letter I with circumflex, U+00CE ISOlat1" },
{ 207,	"Iuml",	"latin capital letter I with diaeresis, U+00CF ISOlat1" },
{ 208,	"ETH",	"latin capital letter ETH, U+00D0 ISOlat1" },
{ 209,	"Ntilde","latin capital letter N with tilde, U+00D1 ISOlat1" },
{ 210,	"Ograve","latin capital letter O with grave, U+00D2 ISOlat1" },
{ 211,	"Oacute","latin capital letter O with acute, U+00D3 ISOlat1" },
{ 212,	"Ocirc","latin capital letter O with circumflex, U+00D4 ISOlat1" },
{ 213,	"Otilde","latin capital letter O with tilde, U+00D5 ISOlat1" },
{ 214,	"Ouml",	"latin capital letter O with diaeresis, U+00D6 ISOlat1" },
{ 215,	"times","multiplication sign, U+00D7 ISOnum" },
{ 216,	"Oslash","latin capital letter O with stroke latin capital letter O slash, U+00D8 ISOlat1" },
{ 217,	"Ugrave","latin capital letter U with grave, U+00D9 ISOlat1" },
{ 218,	"Uacute","latin capital letter U with acute, U+00DA ISOlat1" },
{ 219,	"Ucirc","latin capital letter U with circumflex, U+00DB ISOlat1" },
{ 220,	"Uuml",	"latin capital letter U with diaeresis, U+00DC ISOlat1" },
{ 221,	"Yacute","latin capital letter Y with acute, U+00DD ISOlat1" },
{ 222,	"THORN","latin capital letter THORN, U+00DE ISOlat1" },
{ 223,	"szlig","latin small letter sharp s = ess-zed, U+00DF ISOlat1" },
{ 224,	"agrave","latin small letter a with grave = latin small letter a grave, U+00E0 ISOlat1" },
{ 225,	"aacute","latin small letter a with acute, U+00E1 ISOlat1" },
{ 226,	"acirc","latin small letter a with circumflex, U+00E2 ISOlat1" },
{ 227,	"atilde","latin small letter a with tilde, U+00E3 ISOlat1" },
{ 228,	"auml",	"latin small letter a with diaeresis, U+00E4 ISOlat1" },
{ 229,	"aring","latin small letter a with ring above = latin small letter a ring, U+00E5 ISOlat1" },
{ 230,	"aelig","latin small letter ae = latin small ligature ae, U+00E6 ISOlat1" },
{ 231,	"ccedil","latin small letter c with cedilla, U+00E7 ISOlat1" },
{ 232,	"egrave","latin small letter e with grave, U+00E8 ISOlat1" },
{ 233,	"eacute","latin small letter e with acute, U+00E9 ISOlat1" },
{ 234,	"ecirc","latin small letter e with circumflex, U+00EA ISOlat1" },
{ 235,	"euml",	"latin small letter e with diaeresis, U+00EB ISOlat1" },
{ 236,	"igrave","latin small letter i with grave, U+00EC ISOlat1" },
{ 237,	"iacute","latin small letter i with acute, U+00ED ISOlat1" },
{ 238,	"icirc","latin small letter i with circumflex, U+00EE ISOlat1" },
{ 239,	"iuml",	"latin small letter i with diaeresis, U+00EF ISOlat1" },
{ 240,	"eth",	"latin small letter eth, U+00F0 ISOlat1" },
{ 241,	"ntilde","latin small letter n with tilde, U+00F1 ISOlat1" },
{ 242,	"ograve","latin small letter o with grave, U+00F2 ISOlat1" },
{ 243,	"oacute","latin small letter o with acute, U+00F3 ISOlat1" },
{ 244,	"ocirc","latin small letter o with circumflex, U+00F4 ISOlat1" },
{ 245,	"otilde","latin small letter o with tilde, U+00F5 ISOlat1" },
{ 246,	"ouml",	"latin small letter o with diaeresis, U+00F6 ISOlat1" },
{ 247,	"divide","division sign, U+00F7 ISOnum" },
{ 248,	"oslash","latin small letter o with stroke, = latin small letter o slash, U+00F8 ISOlat1" },
{ 249,	"ugrave","latin small letter u with grave, U+00F9 ISOlat1" },
{ 250,	"uacute","latin small letter u with acute, U+00FA ISOlat1" },
{ 251,	"ucirc","latin small letter u with circumflex, U+00FB ISOlat1" },
{ 252,	"uuml",	"latin small letter u with diaeresis, U+00FC ISOlat1" },
{ 253,	"yacute","latin small letter y with acute, U+00FD ISOlat1" },
{ 254,	"thorn","latin small letter thorn with, U+00FE ISOlat1" },
{ 255,	"yuml",	"latin small letter y with diaeresis, U+00FF ISOlat1" },

{ 338,	"OElig","latin capital ligature OE, U+0152 ISOlat2" },
{ 339,	"oelig","latin small ligature oe, U+0153 ISOlat2" },
{ 352,	"Scaron","latin capital letter S with caron, U+0160 ISOlat2" },
{ 353,	"scaron","latin small letter s with caron, U+0161 ISOlat2" },
{ 376,	"Yuml",	"latin capital letter Y with diaeresis, U+0178 ISOlat2" },

/*
 * Anything below should really be kept as entities references
 */
{ 402,	"fnof",	"latin small f with hook = function = florin, U+0192 ISOtech" },

{ 710,	"circ",	"modifier letter circumflex accent, U+02C6 ISOpub" },
{ 732,	"tilde","small tilde, U+02DC ISOdia" },

{ 913,	"Alpha","greek capital letter alpha, U+0391" },
{ 914,	"Beta",	"greek capital letter beta, U+0392" },
{ 915,	"Gamma","greek capital letter gamma, U+0393 ISOgrk3" },
{ 916,	"Delta","greek capital letter delta, U+0394 ISOgrk3" },
{ 917,	"Epsilon","greek capital letter epsilon, U+0395" },
{ 918,	"Zeta",	"greek capital letter zeta, U+0396" },
{ 919,	"Eta",	"greek capital letter eta, U+0397" },
{ 920,	"Theta","greek capital letter theta, U+0398 ISOgrk3" },
{ 921,	"Iota",	"greek capital letter iota, U+0399" },
{ 922,	"Kappa","greek capital letter kappa, U+039A" },
{ 923,	"Lambda", "greek capital letter lambda, U+039B ISOgrk3" },
{ 924,	"Mu",	"greek capital letter mu, U+039C" },
{ 925,	"Nu",	"greek capital letter nu, U+039D" },
{ 926,	"Xi",	"greek capital letter xi, U+039E ISOgrk3" },
{ 927,	"Omicron","greek capital letter omicron, U+039F" },
{ 928,	"Pi",	"greek capital letter pi, U+03A0 ISOgrk3" },
{ 929,	"Rho",	"greek capital letter rho, U+03A1" },
{ 931,	"Sigma","greek capital letter sigma, U+03A3 ISOgrk3" },
{ 932,	"Tau",	"greek capital letter tau, U+03A4" },
{ 933,	"Upsilon","greek capital letter upsilon, U+03A5 ISOgrk3" },
{ 934,	"Phi",	"greek capital letter phi, U+03A6 ISOgrk3" },
{ 935,	"Chi",	"greek capital letter chi, U+03A7" },
{ 936,	"Psi",	"greek capital letter psi, U+03A8 ISOgrk3" },
{ 937,	"Omega","greek capital letter omega, U+03A9 ISOgrk3" },

{ 945,	"alpha","greek small letter alpha, U+03B1 ISOgrk3" },
{ 946,	"beta",	"greek small letter beta, U+03B2 ISOgrk3" },
{ 947,	"gamma","greek small letter gamma, U+03B3 ISOgrk3" },
{ 948,	"delta","greek small letter delta, U+03B4 ISOgrk3" },
{ 949,	"epsilon","greek small letter epsilon, U+03B5 ISOgrk3" },
{ 950,	"zeta",	"greek small letter zeta, U+03B6 ISOgrk3" },
{ 951,	"eta",	"greek small letter eta, U+03B7 ISOgrk3" },
{ 952,	"theta","greek small letter theta, U+03B8 ISOgrk3" },
{ 953,	"iota",	"greek small letter iota, U+03B9 ISOgrk3" },
{ 954,	"kappa","greek small letter kappa, U+03BA ISOgrk3" },
{ 955,	"lambda","greek small letter lambda, U+03BB ISOgrk3" },
{ 956,	"mu",	"greek small letter mu, U+03BC ISOgrk3" },
{ 957,	"nu",	"greek small letter nu, U+03BD ISOgrk3" },
{ 958,	"xi",	"greek small letter xi, U+03BE ISOgrk3" },
{ 959,	"omicron","greek small letter omicron, U+03BF NEW" },
{ 960,	"pi",	"greek small letter pi, U+03C0 ISOgrk3" },
{ 961,	"rho",	"greek small letter rho, U+03C1 ISOgrk3" },
{ 962,	"sigmaf","greek small letter final sigma, U+03C2 ISOgrk3" },
{ 963,	"sigma","greek small letter sigma, U+03C3 ISOgrk3" },
{ 964,	"tau",	"greek small letter tau, U+03C4 ISOgrk3" },
{ 965,	"upsilon","greek small letter upsilon, U+03C5 ISOgrk3" },
{ 966,	"phi",	"greek small letter phi, U+03C6 ISOgrk3" },
{ 967,	"chi",	"greek small letter chi, U+03C7 ISOgrk3" },
{ 968,	"psi",	"greek small letter psi, U+03C8 ISOgrk3" },
{ 969,	"omega","greek small letter omega, U+03C9 ISOgrk3" },
{ 977,	"thetasym","greek small letter theta symbol, U+03D1 NEW" },
{ 978,	"upsih","greek upsilon with hook symbol, U+03D2 NEW" },
{ 982,	"piv",	"greek pi symbol, U+03D6 ISOgrk3" },

{ 8194,	"ensp",	"en space, U+2002 ISOpub" },
{ 8195,	"emsp",	"em space, U+2003 ISOpub" },
{ 8201,	"thinsp","thin space, U+2009 ISOpub" },
{ 8204,	"zwnj",	"zero width non-joiner, U+200C NEW RFC 2070" },
{ 8205,	"zwj",	"zero width joiner, U+200D NEW RFC 2070" },
{ 8206,	"lrm",	"left-to-right mark, U+200E NEW RFC 2070" },
{ 8207,	"rlm",	"right-to-left mark, U+200F NEW RFC 2070" },
{ 8211,	"ndash","en dash, U+2013 ISOpub" },
{ 8212,	"mdash","em dash, U+2014 ISOpub" },
{ 8216,	"lsquo","left single quotation mark, U+2018 ISOnum" },
{ 8217,	"rsquo","right single quotation mark, U+2019 ISOnum" },
{ 8218,	"sbquo","single low-9 quotation mark, U+201A NEW" },
{ 8220,	"ldquo","left double quotation mark, U+201C ISOnum" },
{ 8221,	"rdquo","right double quotation mark, U+201D ISOnum" },
{ 8222,	"bdquo","double low-9 quotation mark, U+201E NEW" },
{ 8224,	"dagger","dagger, U+2020 ISOpub" },
{ 8225,	"Dagger","double dagger, U+2021 ISOpub" },

{ 8226,	"bull",	"bullet = black small circle, U+2022 ISOpub" },
{ 8230,	"hellip","horizontal ellipsis = three dot leader, U+2026 ISOpub" },

{ 8240,	"permil","per mille sign, U+2030 ISOtech" },

{ 8242,	"prime","prime = minutes = feet, U+2032 ISOtech" },
{ 8243,	"Prime","double prime = seconds = inches, U+2033 ISOtech" },

{ 8249,	"lsaquo","single left-pointing angle quotation mark, U+2039 ISO proposed" },
{ 8250,	"rsaquo","single right-pointing angle quotation mark, U+203A ISO proposed" },

{ 8254,	"oline","overline = spacing overscore, U+203E NEW" },
{ 8260,	"frasl","fraction slash, U+2044 NEW" },

{ 8364,	"euro",	"euro sign, U+20AC NEW" },

{ 8465,	"image","blackletter capital I = imaginary part, U+2111 ISOamso" },
{ 8472,	"weierp","script capital P = power set = Weierstrass p, U+2118 ISOamso" },
{ 8476,	"real",	"blackletter capital R = real part symbol, U+211C ISOamso" },
{ 8482,	"trade","trade mark sign, U+2122 ISOnum" },
{ 8501,	"alefsym","alef symbol = first transfinite cardinal, U+2135 NEW" },
{ 8592,	"larr",	"leftwards arrow, U+2190 ISOnum" },
{ 8593,	"uarr",	"upwards arrow, U+2191 ISOnum" },
{ 8594,	"rarr",	"rightwards arrow, U+2192 ISOnum" },
{ 8595,	"darr",	"downwards arrow, U+2193 ISOnum" },
{ 8596,	"harr",	"left right arrow, U+2194 ISOamsa" },
{ 8629,	"crarr","downwards arrow with corner leftwards = carriage return, U+21B5 NEW" },
{ 8656,	"lArr",	"leftwards double arrow, U+21D0 ISOtech" },
{ 8657,	"uArr",	"upwards double arrow, U+21D1 ISOamsa" },
{ 8658,	"rArr",	"rightwards double arrow, U+21D2 ISOtech" },
{ 8659,	"dArr",	"downwards double arrow, U+21D3 ISOamsa" },
{ 8660,	"hArr",	"left right double arrow, U+21D4 ISOamsa" },

{ 8704,	"forall","for all, U+2200 ISOtech" },
{ 8706,	"part",	"partial differential, U+2202 ISOtech" },
{ 8707,	"exist","there exists, U+2203 ISOtech" },
{ 8709,	"empty","empty set = null set = diameter, U+2205 ISOamso" },
{ 8711,	"nabla","nabla = backward difference, U+2207 ISOtech" },
{ 8712,	"isin",	"element of, U+2208 ISOtech" },
{ 8713,	"notin","not an element of, U+2209 ISOtech" },
{ 8715,	"ni",	"contains as member, U+220B ISOtech" },
{ 8719,	"prod",	"n-ary product = product sign, U+220F ISOamsb" },
{ 8721,	"sum",	"n-ary sumation, U+2211 ISOamsb" },
{ 8722,	"minus","minus sign, U+2212 ISOtech" },
{ 8727,	"lowast","asterisk operator, U+2217 ISOtech" },
{ 8730,	"radic","square root = radical sign, U+221A ISOtech" },
{ 8733,	"prop",	"proportional to, U+221D ISOtech" },
{ 8734,	"infin","infinity, U+221E ISOtech" },
{ 8736,	"ang",	"angle, U+2220 ISOamso" },
{ 8743,	"and",	"logical and = wedge, U+2227 ISOtech" },
{ 8744,	"or",	"logical or = vee, U+2228 ISOtech" },
{ 8745,	"cap",	"intersection = cap, U+2229 ISOtech" },
{ 8746,	"cup",	"union = cup, U+222A ISOtech" },
{ 8747,	"int",	"integral, U+222B ISOtech" },
{ 8756,	"there4","therefore, U+2234 ISOtech" },
{ 8764,	"sim",	"tilde operator = varies with = similar to, U+223C ISOtech" },
{ 8773,	"cong",	"approximately equal to, U+2245 ISOtech" },
{ 8776,	"asymp","almost equal to = asymptotic to, U+2248 ISOamsr" },
{ 8800,	"ne",	"not equal to, U+2260 ISOtech" },
{ 8801,	"equiv","identical to, U+2261 ISOtech" },
{ 8804,	"le",	"less-than or equal to, U+2264 ISOtech" },
{ 8805,	"ge",	"greater-than or equal to, U+2265 ISOtech" },
{ 8834,	"sub",	"subset of, U+2282 ISOtech" },
{ 8835,	"sup",	"superset of, U+2283 ISOtech" },
{ 8836,	"nsub",	"not a subset of, U+2284 ISOamsn" },
{ 8838,	"sube",	"subset of or equal to, U+2286 ISOtech" },
{ 8839,	"supe",	"superset of or equal to, U+2287 ISOtech" },
{ 8853,	"oplus","circled plus = direct sum, U+2295 ISOamsb" },
{ 8855,	"otimes","circled times = vector product, U+2297 ISOamsb" },
{ 8869,	"perp",	"up tack = orthogonal to = perpendicular, U+22A5 ISOtech" },
{ 8901,	"sdot",	"dot operator, U+22C5 ISOamsb" },
{ 8968,	"lceil","left ceiling = apl upstile, U+2308 ISOamsc" },
{ 8969,	"rceil","right ceiling, U+2309 ISOamsc" },
{ 8970,	"lfloor","left floor = apl downstile, U+230A ISOamsc" },
{ 8971,	"rfloor","right floor, U+230B ISOamsc" },
{ 9001,	"lang",	"left-pointing angle bracket = bra, U+2329 ISOtech" },
{ 9002,	"rang",	"right-pointing angle bracket = ket, U+232A ISOtech" },
{ 9674,	"loz",	"lozenge, U+25CA ISOpub" },

{ 9824,	"spades","black spade suit, U+2660 ISOpub" },
{ 9827,	"clubs","black club suit = shamrock, U+2663 ISOpub" },
{ 9829,	"hearts","black heart suit = valentine, U+2665 ISOpub" },
{ 9830,	"diams","black diamond suit, U+2666 ISOpub" },

};

/************************************************************************
 *									*
 *		Commodity functions to handle entities			*
 *									*
 ************************************************************************/

/*
 * Macro used to grow the current buffer.
 */
#define growBuffer(buffer) {						\
    buffer##_size *= 2;							\
    buffer = (xmlChar *) xmlRealloc(buffer, buffer##_size * sizeof(xmlChar));	\
    if (buffer == NULL) {						\
	perror("realloc failed");					\
	return(NULL);							\
    }									\
}

/**
 * htmlEntityLookup:
 * @name: the entity name
 *
 * Lookup the given entity in EntitiesTable
 *
 * TODO: the linear scan is really ugly, an hash table is really needed.
 *
 * Returns the associated htmlEntityDescPtr if found, NULL otherwise.
 */
const htmlEntityDesc *
htmlEntityLookup(const xmlChar *name) {
    unsigned int i;

    for (i = 0;i < (sizeof(html40EntitiesTable)/
                    sizeof(html40EntitiesTable[0]));i++) {
        if (xmlStrEqual(name, BAD_CAST html40EntitiesTable[i].name)) {
#ifdef DEBUG
            xmlGenericError(xmlGenericErrorContext,"Found entity %s\n", name);
#endif
            return((const htmlEntityDescPtr) &html40EntitiesTable[i]);
	}
    }
    return(NULL);
}

/**
 * htmlEntityValueLookup:
 * @value: the entity's unicode value
 *
 * Lookup the given entity in EntitiesTable
 *
 * TODO: the linear scan is really ugly, an hash table is really needed.
 *
 * Returns the associated htmlEntityDescPtr if found, NULL otherwise.
 */
const htmlEntityDesc *
htmlEntityValueLookup(unsigned int value) {
    unsigned int i;
#ifdef DEBUG
    unsigned int lv = 0;
#endif

    for (i = 0;i < (sizeof(html40EntitiesTable)/
                    sizeof(html40EntitiesTable[0]));i++) {
        if (html40EntitiesTable[i].value >= value) {
	    if (html40EntitiesTable[i].value > value)
		break;
#ifdef DEBUG
	    xmlGenericError(xmlGenericErrorContext,"Found entity %s\n", html40EntitiesTable[i].name);
#endif
            return((const htmlEntityDescPtr) &html40EntitiesTable[i]);
	}
#ifdef DEBUG
	if (lv > html40EntitiesTable[i].value) {
	    xmlGenericError(xmlGenericErrorContext,
		    "html40EntitiesTable[] is not sorted (%d > %d)!\n",
		    lv, html40EntitiesTable[i].value);
	}
	lv = html40EntitiesTable[i].value;
#endif
    }
    return(NULL);
}

/**
 * UTF8ToHtml:
 * @out:  a pointer to an array of bytes to store the result
 * @outlen:  the length of @out
 * @in:  a pointer to an array of UTF-8 chars
 * @inlen:  the length of @in
 *
 * Take a block of UTF-8 chars in and try to convert it to an ASCII
 * plus HTML entities block of chars out.
 *
 * Returns 0 if success, -2 if the transcoding fails, or -1 otherwise
 * The value of @inlen after return is the number of octets consumed
 *     as the return value is positive, else unpredictiable.
 * The value of @outlen after return is the number of octets consumed.
 */
int
UTF8ToHtml(unsigned char* out, int *outlen,
              const unsigned char* in, int *inlen) {
    const unsigned char* processed = in;
    const unsigned char* outend;
    const unsigned char* outstart = out;
    const unsigned char* instart = in;
    const unsigned char* inend;
    unsigned int c, d;
    int trailing;

    if (in == NULL) {
        /*
	 * initialization nothing to do
	 */
	*outlen = 0;
	*inlen = 0;
	return(0);
    }
    inend = in + (*inlen);
    outend = out + (*outlen);
    while (in < inend) {
	d = *in++;
	if      (d < 0x80)  { c= d; trailing= 0; }
	else if (d < 0xC0) {
	    /* trailing byte in leading position */
	    *outlen = out - outstart;
	    *inlen = processed - instart;
	    return(-2);
        } else if (d < 0xE0)  { c= d & 0x1F; trailing= 1; }
        else if (d < 0xF0)  { c= d & 0x0F; trailing= 2; }
        else if (d < 0xF8)  { c= d & 0x07; trailing= 3; }
	else {
	    /* no chance for this in Ascii */
	    *outlen = out - outstart;
	    *inlen = processed - instart;
	    return(-2);
	}

	if (inend - in < trailing) {
	    break;
	} 

	for ( ; trailing; trailing--) {
	    if ((in >= inend) || (((d= *in++) & 0xC0) != 0x80))
		break;
	    c <<= 6;
	    c |= d & 0x3F;
	}

	/* assertion: c is a single UTF-4 value */
	if (c < 0x80) {
	    if (out + 1 >= outend)
		break;
	    *out++ = c;
	} else {
	    int len;
	    const htmlEntityDesc * ent;

	    /*
	     * Try to lookup a predefined HTML entity for it
	     */

	    ent = htmlEntityValueLookup(c);
	    if (ent == NULL) {
		/* no chance for this in Ascii */
		*outlen = out - outstart;
		*inlen = processed - instart;
		return(-2);
	    }
	    len = strlen(ent->name);
	    if (out + 2 + len >= outend)
		break;
	    *out++ = '&';
	    memcpy(out, ent->name, len);
	    out += len;
	    *out++ = ';';
	}
	processed = in;
    }
    *outlen = out - outstart;
    *inlen = processed - instart;
    return(0);
}

/**
 * htmlEncodeEntities:
 * @out:  a pointer to an array of bytes to store the result
 * @outlen:  the length of @out
 * @in:  a pointer to an array of UTF-8 chars
 * @inlen:  the length of @in
 * @quoteChar: the quote character to escape (' or ") or zero.
 *
 * Take a block of UTF-8 chars in and try to convert it to an ASCII
 * plus HTML entities block of chars out.
 *
 * Returns 0 if success, -2 if the transcoding fails, or -1 otherwise
 * The value of @inlen after return is the number of octets consumed
 *     as the return value is positive, else unpredictiable.
 * The value of @outlen after return is the number of octets consumed.
 */
int
htmlEncodeEntities(unsigned char* out, int *outlen,
		   const unsigned char* in, int *inlen, int quoteChar) {
    const unsigned char* processed = in;
    const unsigned char* outend = out + (*outlen);
    const unsigned char* outstart = out;
    const unsigned char* instart = in;
    const unsigned char* inend = in + (*inlen);
    unsigned int c, d;
    int trailing;

    while (in < inend) {
	d = *in++;
	if      (d < 0x80)  { c= d; trailing= 0; }
	else if (d < 0xC0) {
	    /* trailing byte in leading position */
	    *outlen = out - outstart;
	    *inlen = processed - instart;
	    return(-2);
        } else if (d < 0xE0)  { c= d & 0x1F; trailing= 1; }
        else if (d < 0xF0)  { c= d & 0x0F; trailing= 2; }
        else if (d < 0xF8)  { c= d & 0x07; trailing= 3; }
	else {
	    /* no chance for this in Ascii */
	    *outlen = out - outstart;
	    *inlen = processed - instart;
	    return(-2);
	}

	if (inend - in < trailing)
	    break;

	while (trailing--) {
	    if (((d= *in++) & 0xC0) != 0x80) {
		*outlen = out - outstart;
		*inlen = processed - instart;
		return(-2);
	    }
	    c <<= 6;
	    c |= d & 0x3F;
	}

	/* assertion: c is a single UTF-4 value */
	if ((c < 0x80) && (c != (unsigned int) quoteChar) &&
	    (c != '&') && (c != '<') && (c != '>')) {
	    if (out >= outend)
		break;
	    *out++ = c;
	} else {
	    const htmlEntityDesc * ent;
	    const char *cp;
	    char nbuf[16];
	    int len;

	    /*
	     * Try to lookup a predefined HTML entity for it
	     */
	    ent = htmlEntityValueLookup(c);
	    if (ent == NULL) {
		sprintf(nbuf, "#%u", c);
		cp = nbuf;
	    }
	    else
		cp = ent->name;
	    len = strlen(cp);
	    if (out + 2 + len > outend)
		break;
	    *out++ = '&';
	    memcpy(out, cp, len);
	    out += len;
	    *out++ = ';';
	}
	processed = in;
    }
    *outlen = out - outstart;
    *inlen = processed - instart;
    return(0);
}

/**
 * htmlDecodeEntities:
 * @ctxt:  the parser context
 * @len:  the len to decode (in bytes !), -1 for no size limit
 * @end:  an end marker xmlChar, 0 if none
 * @end2:  an end marker xmlChar, 0 if none
 * @end3:  an end marker xmlChar, 0 if none
 *
 * Subtitute the HTML entities by their value
 *
 * DEPRECATED !!!!
 *
 * Returns A newly allocated string with the substitution done. The caller
 *      must deallocate it !
 */
xmlChar *
htmlDecodeEntities(htmlParserCtxtPtr ctxt ATTRIBUTE_UNUSED, int len ATTRIBUTE_UNUSED,
	  xmlChar end ATTRIBUTE_UNUSED, xmlChar  end2 ATTRIBUTE_UNUSED, xmlChar end3 ATTRIBUTE_UNUSED) {
    static int deprecated = 0;
    if (!deprecated) {
	xmlGenericError(xmlGenericErrorContext,
		"htmlDecodeEntities() deprecated function reached\n");
	deprecated = 1;
    }
    return(NULL);
#if 0
    xmlChar *name = NULL;
    xmlChar *buffer = NULL;
    unsigned int buffer_size = 0;
    unsigned int nbchars = 0;
    htmlEntityDescPtr ent;
    unsigned int max = (unsigned int) len;
    int c,l;

    if (ctxt->depth > 40) {
	ctxt->errNo = XML_ERR_ENTITY_LOOP;
	if ((ctxt->sax != NULL) && (ctxt->sax->error != NULL))
	    ctxt->sax->error(ctxt->userData,
		"Detected entity reference loop\n");
	ctxt->wellFormed = 0;
	ctxt->disableSAX = 1;
	return(NULL);
    }

    /*
     * allocate a translation buffer.
     */
    buffer_size = HTML_PARSER_BIG_BUFFER_SIZE;
    buffer = (xmlChar *) xmlMalloc(buffer_size * sizeof(xmlChar));
    if (buffer == NULL) {
	perror("xmlDecodeEntities: malloc failed");
	return(NULL);
    }

    /*
     * Ok loop until we reach one of the ending char or a size limit.
     */
    c = CUR_CHAR(l);
    while ((nbchars < max) && (c != end) &&
           (c != end2) && (c != end3)) {

	if (c == 0) break;
        if (((c == '&') && (ctxt->token != '&')) && (NXT(1) == '#')) {
	    int val = htmlParseCharRef(ctxt);
	    COPY_BUF(0,buffer,nbchars,val);
	    NEXTL(l);
	} else if ((c == '&') && (ctxt->token != '&')) {
	    ent = htmlParseEntityRef(ctxt, &name);
	    if (name != NULL) {
		if (ent != NULL) {
		    int val = ent->value;
		    COPY_BUF(0,buffer,nbchars,val);
		    NEXTL(l);
		} else {
		    const xmlChar *cur = name;

		    buffer[nbchars++] = '&';
		    if (nbchars > buffer_size - HTML_PARSER_BUFFER_SIZE) {
			growBuffer(buffer);
		    }
		    while (*cur != 0) {
			buffer[nbchars++] = *cur++;
		    }
		    buffer[nbchars++] = ';';
		}
	    }
	} else {
	    COPY_BUF(l,buffer,nbchars,c);
	    NEXTL(l);
	    if (nbchars > buffer_size - HTML_PARSER_BUFFER_SIZE) {
	      growBuffer(buffer);
	    }
	}
	c = CUR_CHAR(l);
    }
    buffer[nbchars++] = 0;
    return(buffer);
#endif
}

/************************************************************************
 *									*
 *		Commodity functions to handle streams			*
 *									*
 ************************************************************************/

/**
 * htmlNewInputStream:
 * @ctxt:  an HTML parser context
 *
 * Create a new input stream structure
 * Returns the new input stream or NULL
 */
static htmlParserInputPtr
htmlNewInputStream(htmlParserCtxtPtr ctxt) {
    htmlParserInputPtr input;

    input = (xmlParserInputPtr) xmlMalloc(sizeof(htmlParserInput));
    if (input == NULL) {
        ctxt->errNo = XML_ERR_NO_MEMORY;
	if ((ctxt->sax != NULL) && (ctxt->sax->error != NULL))
	    ctxt->sax->error(ctxt->userData, 
	                     "malloc: couldn't allocate a new input stream\n");
	return(NULL);
    }
    memset(input, 0, sizeof(htmlParserInput));
    input->filename = NULL;
    input->directory = NULL;
    input->base = NULL;
    input->cur = NULL;
    input->buf = NULL;
    input->line = 1;
    input->col = 1;
    input->buf = NULL;
    input->free = NULL;
    input->version = NULL;
    input->consumed = 0;
    input->length = 0;
    return(input);
}


/************************************************************************
 *									*
 *		Commodity functions, cleanup needed ?			*
 *									*
 ************************************************************************/

/**
 * areBlanks:
 * @ctxt:  an HTML parser context
 * @str:  a xmlChar *
 * @len:  the size of @str
 *
 * Is this a sequence of blank chars that one can ignore ?
 *
 * Returns 1 if ignorable 0 otherwise.
 */

static int areBlanks(htmlParserCtxtPtr ctxt, const xmlChar *str, int len) {
    int i;
    xmlNodePtr lastChild;

    for (i = 0;i < len;i++)
        if (!(IS_BLANK(str[i]))) return(0);

    if (CUR == 0) return(1);
    if (CUR != '<') return(0);
    if (ctxt->name == NULL)
	return(1);
    if (xmlStrEqual(ctxt->name, BAD_CAST"html"))
	return(1);
    if (xmlStrEqual(ctxt->name, BAD_CAST"head"))
	return(1);
    if (xmlStrEqual(ctxt->name, BAD_CAST"body"))
	return(1);
    if (ctxt->node == NULL) return(0);
    lastChild = xmlGetLastChild(ctxt->node);
    if (lastChild == NULL) {
        if ((ctxt->node->type != XML_ELEMENT_NODE) &&
            (ctxt->node->content != NULL)) return(0);
    } else if (xmlNodeIsText(lastChild)) {
        return(0);
    } else if (xmlStrEqual(lastChild->name, BAD_CAST"b")) {
        return(0);
    } else if (xmlStrEqual(lastChild->name, BAD_CAST"bold")) {
        return(0);
    } else if (xmlStrEqual(lastChild->name, BAD_CAST"em")) {
        return(0);
    }
    return(1);
}

/**
 * htmlNewDocNoDtD:
 * @URI:  URI for the dtd, or NULL
 * @ExternalID:  the external ID of the DTD, or NULL
 *
 * Creates a new HTML document without a DTD node if @URI and @ExternalID
 * are NULL
 *
 * Returns a new document, do not intialize the DTD if not provided
 */
htmlDocPtr
htmlNewDocNoDtD(const xmlChar *URI, const xmlChar *ExternalID) {
    xmlDocPtr cur;

    /*
     * Allocate a new document and fill the fields.
     */
    cur = (xmlDocPtr) xmlMalloc(sizeof(xmlDoc));
    if (cur == NULL) {
        xmlGenericError(xmlGenericErrorContext,
		"xmlNewDoc : malloc failed\n");
	return(NULL);
    }
    memset(cur, 0, sizeof(xmlDoc));

    cur->type = XML_HTML_DOCUMENT_NODE;
    cur->version = NULL;
    cur->intSubset = NULL;
    cur->doc = cur;
    cur->name = NULL;
    cur->children = NULL; 
    cur->extSubset = NULL;
    cur->oldNs = NULL;
    cur->encoding = NULL;
    cur->standalone = 1;
    cur->compression = 0;
    cur->ids = NULL;
    cur->refs = NULL;
    cur->_private = NULL;
    if ((ExternalID != NULL) ||
	(URI != NULL))
	xmlCreateIntSubset(cur, BAD_CAST "HTML", ExternalID, URI);
    return(cur);
}

/**
 * htmlNewDoc:
 * @URI:  URI for the dtd, or NULL
 * @ExternalID:  the external ID of the DTD, or NULL
 *
 * Creates a new HTML document
 *
 * Returns a new document
 */
htmlDocPtr
htmlNewDoc(const xmlChar *URI, const xmlChar *ExternalID) {
    if ((URI == NULL) && (ExternalID == NULL))
	return(htmlNewDocNoDtD(
		    BAD_CAST "http://www.w3.org/TR/REC-html40/loose.dtd",
		    BAD_CAST "-//W3C//DTD HTML 4.0 Transitional//EN"));

    return(htmlNewDocNoDtD(URI, ExternalID));
}


/************************************************************************
 *									*
 *			The parser itself				*
 *	Relates to http://www.w3.org/TR/html40				*
 *									*
 ************************************************************************/

/************************************************************************
 *									*
 *			The parser itself				*
 *									*
 ************************************************************************/

/**
 * htmlParseHTMLName:
 * @ctxt:  an HTML parser context
 *
 * parse an HTML tag or attribute name, note that we convert it to lowercase
 * since HTML names are not case-sensitive.
 *
 * Returns the Tag Name parsed or NULL
 */

static xmlChar *
htmlParseHTMLName(htmlParserCtxtPtr ctxt) {
    xmlChar *ret = NULL;
    int i = 0;
    xmlChar loc[HTML_PARSER_BUFFER_SIZE];

    if (!IS_LETTER(CUR) && (CUR != '_') &&
        (CUR != ':')) return(NULL);

    while ((i < HTML_PARSER_BUFFER_SIZE) &&
           ((IS_LETTER(CUR)) || (IS_DIGIT(CUR)) ||
	   (CUR == ':') || (CUR == '-') || (CUR == '_'))) {
	if ((CUR >= 'A') && (CUR <= 'Z')) loc[i] = CUR + 0x20;
        else loc[i] = CUR;
	i++;
	
	NEXT;
    }
    
    ret = xmlStrndup(loc, i);

    return(ret);
}

/**
 * htmlParseName:
 * @ctxt:  an HTML parser context
 *
 * parse an HTML name, this routine is case sensistive.
 *
 * Returns the Name parsed or NULL
 */

static xmlChar *
htmlParseName(htmlParserCtxtPtr ctxt) {
    xmlChar buf[HTML_MAX_NAMELEN];
    int len = 0;

    GROW;
    if (!IS_LETTER(CUR) && (CUR != '_')) {
	return(NULL);
    }

    while ((IS_LETTER(CUR)) || (IS_DIGIT(CUR)) ||
           (CUR == '.') || (CUR == '-') ||
	   (CUR == '_') || (CUR == ':') || 
	   (IS_COMBINING(CUR)) ||
	   (IS_EXTENDER(CUR))) {
	buf[len++] = CUR;
	NEXT;
	if (len >= HTML_MAX_NAMELEN) {
	    xmlGenericError(xmlGenericErrorContext, 
	       "htmlParseName: reached HTML_MAX_NAMELEN limit\n");
	    while ((IS_LETTER(CUR)) || (IS_DIGIT(CUR)) ||
		   (CUR == '.') || (CUR == '-') ||
		   (CUR == '_') || (CUR == ':') || 
		   (IS_COMBINING(CUR)) ||
		   (IS_EXTENDER(CUR)))
		 NEXT;
	    break;
	}
    }
    return(xmlStrndup(buf, len));
}

/**
 * htmlParseHTMLAttribute:
 * @ctxt:  an HTML parser context
 * @stop:  a char stop value
 * 
 * parse an HTML attribute value till the stop (quote), if
 * stop is 0 then it stops at the first space
 *
 * Returns the attribute parsed or NULL
 */

static xmlChar *
htmlParseHTMLAttribute(htmlParserCtxtPtr ctxt, const xmlChar stop) {
    xmlChar *buffer = NULL;
    int buffer_size = 0;
    xmlChar *out = NULL;
    xmlChar *name = NULL;

    xmlChar *cur = NULL;
    const htmlEntityDesc * ent;

    /*
     * allocate a translation buffer.
     */
    buffer_size = HTML_PARSER_BUFFER_SIZE;
    buffer = (xmlChar *) xmlMalloc(buffer_size * sizeof(xmlChar));
    if (buffer == NULL) {
	perror("htmlParseHTMLAttribute: malloc failed");
	return(NULL);
    }
    out = buffer;

    /*
     * Ok loop until we reach one of the ending chars
     */
    while ((CUR != 0) && (CUR != stop) && (CUR != '>')) {
	if ((stop == 0) && (IS_BLANK(CUR))) break;
        if (CUR == '&') {
	    if (NXT(1) == '#') {
		unsigned int c;
		int bits;

		c = htmlParseCharRef(ctxt);
		if      (c <    0x80)
		        { *out++  = c;                bits= -6; }
		else if (c <   0x800)
		        { *out++  =((c >>  6) & 0x1F) | 0xC0;  bits=  0; }
		else if (c < 0x10000)
		        { *out++  =((c >> 12) & 0x0F) | 0xE0;  bits=  6; }
		else                 
		        { *out++  =((c >> 18) & 0x07) | 0xF0;  bits= 12; }
	 
		for ( ; bits >= 0; bits-= 6) {
		    *out++  = ((c >> bits) & 0x3F) | 0x80;
		}
	    } else {
		ent = htmlParseEntityRef(ctxt, &name);
		if (name == NULL) {
		    *out++ = '&';
		    if (out - buffer > buffer_size - 100) {
			int indx = out - buffer;

			growBuffer(buffer);
			out = &buffer[indx];
		    }
		} else if (ent == NULL) {
		    *out++ = '&';
		    cur = name;
		    while (*cur != 0) {
			if (out - buffer > buffer_size - 100) {
			    int indx = out - buffer;

			    growBuffer(buffer);
			    out = &buffer[indx];
			}
			*out++ = *cur++;
		    }
		    xmlFree(name);
		} else {
		    unsigned int c;
		    int bits;

		    if (out - buffer > buffer_size - 100) {
			int indx = out - buffer;

			growBuffer(buffer);
			out = &buffer[indx];
		    }
		    c = (xmlChar)ent->value;
		    if      (c <    0x80)
			{ *out++  = c;                bits= -6; }
		    else if (c <   0x800)
			{ *out++  =((c >>  6) & 0x1F) | 0xC0;  bits=  0; }
		    else if (c < 0x10000)
			{ *out++  =((c >> 12) & 0x0F) | 0xE0;  bits=  6; }
		    else                 
			{ *out++  =((c >> 18) & 0x07) | 0xF0;  bits= 12; }
	     
		    for ( ; bits >= 0; bits-= 6) {
			*out++  = ((c >> bits) & 0x3F) | 0x80;
		    }
		    xmlFree(name);
		}
	    }
	} else {
	    unsigned int c;
	    int bits, l;

	    if (out - buffer > buffer_size - 100) {
		int indx = out - buffer;

		growBuffer(buffer);
		out = &buffer[indx];
	    }
	    c = CUR_CHAR(l);
	    if      (c <    0x80)
		    { *out++  = c;                bits= -6; }
	    else if (c <   0x800)
		    { *out++  =((c >>  6) & 0x1F) | 0xC0;  bits=  0; }
	    else if (c < 0x10000)
		    { *out++  =((c >> 12) & 0x0F) | 0xE0;  bits=  6; }
	    else                 
		    { *out++  =((c >> 18) & 0x07) | 0xF0;  bits= 12; }
     
	    for ( ; bits >= 0; bits-= 6) {
		*out++  = ((c >> bits) & 0x3F) | 0x80;
	    }
	    NEXT;
	}
    }
    *out++ = 0;
    return(buffer);
}

/**
 * htmlParseEntityRef:
 * @ctxt:  an HTML parser context
 * @str:  location to store the entity name
 *
 * parse an HTML ENTITY references
 *
 * [68] EntityRef ::= '&' Name ';'
 *
 * Returns the associated htmlEntityDescPtr if found, or NULL otherwise,
 *         if non-NULL *str will have to be freed by the caller.
 */
const htmlEntityDesc *
htmlParseEntityRef(htmlParserCtxtPtr ctxt, xmlChar **str) {
    xmlChar *name;
    const htmlEntityDesc * ent = NULL;
    *str = NULL;

    if (CUR == '&') {
        NEXT;
        name = htmlParseName(ctxt);
	if (name == NULL) {
	    if ((ctxt->sax != NULL) && (ctxt->sax->error != NULL))
	        ctxt->sax->error(ctxt->userData, "htmlParseEntityRef: no name\n");
	    ctxt->wellFormed = 0;
	} else {
	    GROW;
	    if (CUR == ';') {
		*str = name;

		/*
		 * Lookup the entity in the table.
		 */
		ent = htmlEntityLookup(name);
		if (ent != NULL) /* OK that's ugly !!! */
		    NEXT;
	    } else {
		if ((ctxt->sax != NULL) && (ctxt->sax->error != NULL))
		    ctxt->sax->error(ctxt->userData,
		                     "htmlParseEntityRef: expecting ';'\n");
		*str = name;
	    }
	}
    }
    return(ent);
}

/**
 * htmlParseAttValue:
 * @ctxt:  an HTML parser context
 *
 * parse a value for an attribute
 * Note: the parser won't do substitution of entities here, this
 * will be handled later in xmlStringGetNodeList, unless it was
 * asked for ctxt->replaceEntities != 0 
 *
 * Returns the AttValue parsed or NULL.
 */

static xmlChar *
htmlParseAttValue(htmlParserCtxtPtr ctxt) {
    xmlChar *ret = NULL;

    if (CUR == '"') {
        NEXT;
	ret = htmlParseHTMLAttribute(ctxt, '"');
        if (CUR != '"') {
	    if ((ctxt->sax != NULL) && (ctxt->sax->error != NULL))
		ctxt->sax->error(ctxt->userData, "AttValue: ' expected\n");
	    ctxt->wellFormed = 0;
	} else
	    NEXT;
    } else if (CUR == '\'') {
        NEXT;
	ret = htmlParseHTMLAttribute(ctxt, '\'');
        if (CUR != '\'') {
	    if ((ctxt->sax != NULL) && (ctxt->sax->error != NULL))
		ctxt->sax->error(ctxt->userData, "AttValue: ' expected\n");
	    ctxt->wellFormed = 0;
	} else
	    NEXT;
    } else {
        /*
	 * That's an HTMLism, the attribute value may not be quoted
	 */
	ret = htmlParseHTMLAttribute(ctxt, 0);
	if (ret == NULL) {
	    if ((ctxt->sax != NULL) && (ctxt->sax->error != NULL))
		ctxt->sax->error(ctxt->userData, "AttValue: no value found\n");
	    ctxt->wellFormed = 0;
	}
    }
    return(ret);
}

/**
 * htmlParseSystemLiteral:
 * @ctxt:  an HTML parser context
 * 
 * parse an HTML Literal
 *
 * [11] SystemLiteral ::= ('"' [^"]* '"') | ("'" [^']* "'")
 *
 * Returns the SystemLiteral parsed or NULL
 */

static xmlChar *
htmlParseSystemLiteral(htmlParserCtxtPtr ctxt) {
    const xmlChar *q;
    xmlChar *ret = NULL;

    if (CUR == '"') {
        NEXT;
	q = CUR_PTR;
	while ((IS_CHAR(CUR)) && (CUR != '"'))
	    NEXT;
	if (!IS_CHAR(CUR)) {
	    if ((ctxt->sax != NULL) && (ctxt->sax->error != NULL))
	        ctxt->sax->error(ctxt->userData, "Unfinished SystemLiteral\n");
	    ctxt->wellFormed = 0;
	} else {
	    ret = xmlStrndup(q, CUR_PTR - q);
	    NEXT;
        }
    } else if (CUR == '\'') {
        NEXT;
	q = CUR_PTR;
	while ((IS_CHAR(CUR)) && (CUR != '\''))
	    NEXT;
	if (!IS_CHAR(CUR)) {
	    if ((ctxt->sax != NULL) && (ctxt->sax->error != NULL))
	        ctxt->sax->error(ctxt->userData, "Unfinished SystemLiteral\n");
	    ctxt->wellFormed = 0;
	} else {
	    ret = xmlStrndup(q, CUR_PTR - q);
	    NEXT;
        }
    } else {
	if ((ctxt->sax != NULL) && (ctxt->sax->error != NULL))
	    ctxt->sax->error(ctxt->userData,
	                     "SystemLiteral \" or ' expected\n");
	ctxt->wellFormed = 0;
    }
    
    return(ret);
}

/**
 * htmlParsePubidLiteral:
 * @ctxt:  an HTML parser context
 *
 * parse an HTML public literal
 *
 * [12] PubidLiteral ::= '"' PubidChar* '"' | "'" (PubidChar - "'")* "'"
 *
 * Returns the PubidLiteral parsed or NULL.
 */

static xmlChar *
htmlParsePubidLiteral(htmlParserCtxtPtr ctxt) {
    const xmlChar *q;
    xmlChar *ret = NULL;
    /*
     * Name ::= (Letter | '_') (NameChar)*
     */
    if (CUR == '"') {
        NEXT;
	q = CUR_PTR;
	while (IS_PUBIDCHAR(CUR)) NEXT;
	if (CUR != '"') {
	    if ((ctxt->sax != NULL) && (ctxt->sax->error != NULL))
	        ctxt->sax->error(ctxt->userData, "Unfinished PubidLiteral\n");
	    ctxt->wellFormed = 0;
	} else {
	    ret = xmlStrndup(q, CUR_PTR - q);
	    NEXT;
	}
    } else if (CUR == '\'') {
        NEXT;
	q = CUR_PTR;
	while ((IS_LETTER(CUR)) && (CUR != '\''))
	    NEXT;
	if (!IS_LETTER(CUR)) {
	    if ((ctxt->sax != NULL) && (ctxt->sax->error != NULL))
	        ctxt->sax->error(ctxt->userData, "Unfinished PubidLiteral\n");
	    ctxt->wellFormed = 0;
	} else {
	    ret = xmlStrndup(q, CUR_PTR - q);
	    NEXT;
	}
    } else {
	if ((ctxt->sax != NULL) && (ctxt->sax->error != NULL))
	    ctxt->sax->error(ctxt->userData, "SystemLiteral \" or ' expected\n");
	ctxt->wellFormed = 0;
    }
    
    return(ret);
}

/**
 * htmlParseScript:
 * @ctxt:  an HTML parser context
 *
 * parse the content of an HTML SCRIPT or STYLE element
 * http://www.w3.org/TR/html4/sgml/dtd.html#Script
 * http://www.w3.org/TR/html4/sgml/dtd.html#StyleSheet
 * http://www.w3.org/TR/html4/types.html#type-script
 * http://www.w3.org/TR/html4/types.html#h-6.15
 * http://www.w3.org/TR/html4/appendix/notes.html#h-B.3.2.1
 *
 * Script data ( %Script; in the DTD) can be the content of the SCRIPT
 * element and the value of intrinsic event attributes. User agents must
 * not evaluate script data as HTML markup but instead must pass it on as
 * data to a script engine.
 * NOTES:
 * - The content is passed like CDATA
 * - the attributes for style and scripting "onXXX" are also described
 *   as CDATA but SGML allows entities references in attributes so their
 *   processing is identical as other attributes
 */
static void
htmlParseScript(htmlParserCtxtPtr ctxt) {
    xmlChar buf[HTML_PARSER_BIG_BUFFER_SIZE + 1];
    int nbchar = 0;
    xmlChar cur;

    SHRINK;
    cur = CUR;
    while (IS_CHAR(cur)) {
	if ((cur == '<') && (NXT(1) == '/')) {
	    /*
	     * One should break here, the specification is clear:
	     * Authors should therefore escape "</" within the content.
	     * Escape mechanisms are specific to each scripting or
	     * style sheet language.
	     */
	    if (((NXT(2) >= 'A') && (NXT(2) <= 'Z')) ||
	        ((NXT(2) >= 'a') && (NXT(2) <= 'z')))
		break; /* while */
	}
	buf[nbchar++] = cur;
	if (nbchar >= HTML_PARSER_BIG_BUFFER_SIZE) {
	    if (ctxt->sax->cdataBlock!= NULL) {
		/*
		 * Insert as CDATA, which is the same as HTML_PRESERVE_NODE
		 */
		ctxt->sax->cdataBlock(ctxt->userData, buf, nbchar);
	    }
	    nbchar = 0;
	}
	NEXT;
	cur = CUR;
    }
    if (!(IS_CHAR(cur))) {
	if ((ctxt->sax != NULL) && (ctxt->sax->error != NULL))
	    ctxt->sax->error(ctxt->userData,
		"Invalid char in CDATA 0x%X\n", cur);
	ctxt->wellFormed = 0;
	NEXT;
    }

    if ((nbchar != 0) && (ctxt->sax != NULL) && (!ctxt->disableSAX)) {
	if (ctxt->sax->cdataBlock!= NULL) {
	    /*
	     * Insert as CDATA, which is the same as HTML_PRESERVE_NODE
	     */
	    ctxt->sax->cdataBlock(ctxt->userData, buf, nbchar);
	}
    }
}


/**
 * htmlParseCharData:
 * @ctxt:  an HTML parser context
 *
 * parse a CharData section.
 * if we are within a CDATA section ']]>' marks an end of section.
 *
 * [14] CharData ::= [^<&]* - ([^<&]* ']]>' [^<&]*)
 */

static void
htmlParseCharData(htmlParserCtxtPtr ctxt) {
    xmlChar buf[HTML_PARSER_BIG_BUFFER_SIZE + 5];
    int nbchar = 0;
    int cur, l;

    SHRINK;
    cur = CUR_CHAR(l);
    while (((cur != '<') || (ctxt->token == '<')) &&
           ((cur != '&') || (ctxt->token == '&')) && 
	   (IS_CHAR(cur))) {
	COPY_BUF(l,buf,nbchar,cur);
	if (nbchar >= HTML_PARSER_BIG_BUFFER_SIZE) {
	    /*
	     * Ok the segment is to be consumed as chars.
	     */
	    if ((ctxt->sax != NULL) && (!ctxt->disableSAX)) {
		if (areBlanks(ctxt, buf, nbchar)) {
		    if (ctxt->sax->ignorableWhitespace != NULL)
			ctxt->sax->ignorableWhitespace(ctxt->userData,
			                               buf, nbchar);
		} else {
		    htmlCheckParagraph(ctxt);
		    if (ctxt->sax->characters != NULL)
			ctxt->sax->characters(ctxt->userData, buf, nbchar);
		}
	    }
	    nbchar = 0;
	}
	NEXTL(l);
	cur = CUR_CHAR(l);
    }
    if (nbchar != 0) {
	/*
	 * Ok the segment is to be consumed as chars.
	 */
	if ((ctxt->sax != NULL) && (!ctxt->disableSAX)) {
	    if (areBlanks(ctxt, buf, nbchar)) {
		if (ctxt->sax->ignorableWhitespace != NULL)
		    ctxt->sax->ignorableWhitespace(ctxt->userData, buf, nbchar);
	    } else {
		htmlCheckParagraph(ctxt);
		if (ctxt->sax->characters != NULL)
		    ctxt->sax->characters(ctxt->userData, buf, nbchar);
	    }
	}
    } else {
	/*
	 * Loop detection
	 */
	if (cur == 0)
	    ctxt->instate = XML_PARSER_EOF;
    }
}

/**
 * htmlParseExternalID:
 * @ctxt:  an HTML parser context
 * @publicID:  a xmlChar** receiving PubidLiteral
 *
 * Parse an External ID or a Public ID
 *
 * [75] ExternalID ::= 'SYSTEM' S SystemLiteral
 *                   | 'PUBLIC' S PubidLiteral S SystemLiteral
 *
 * [83] PublicID ::= 'PUBLIC' S PubidLiteral
 *
 * Returns the function returns SystemLiteral and in the second
 *                case publicID receives PubidLiteral, is strict is off
 *                it is possible to return NULL and have publicID set.
 */

static xmlChar *
htmlParseExternalID(htmlParserCtxtPtr ctxt, xmlChar **publicID) {
    xmlChar *URI = NULL;

    if ((UPPER == 'S') && (UPP(1) == 'Y') &&
         (UPP(2) == 'S') && (UPP(3) == 'T') &&
	 (UPP(4) == 'E') && (UPP(5) == 'M')) {
        SKIP(6);
	if (!IS_BLANK(CUR)) {
	    if ((ctxt->sax != NULL) && (ctxt->sax->error != NULL))
		ctxt->sax->error(ctxt->userData,
		    "Space required after 'SYSTEM'\n");
	    ctxt->wellFormed = 0;
	}
        SKIP_BLANKS;
	URI = htmlParseSystemLiteral(ctxt);
	if (URI == NULL) {
	    if ((ctxt->sax != NULL) && (ctxt->sax->error != NULL))
	        ctxt->sax->error(ctxt->userData,
	          "htmlParseExternalID: SYSTEM, no URI\n");
	    ctxt->wellFormed = 0;
        }
    } else if ((UPPER == 'P') && (UPP(1) == 'U') &&
	       (UPP(2) == 'B') && (UPP(3) == 'L') &&
	       (UPP(4) == 'I') && (UPP(5) == 'C')) {
        SKIP(6);
	if (!IS_BLANK(CUR)) {
	    if ((ctxt->sax != NULL) && (ctxt->sax->error != NULL))
		ctxt->sax->error(ctxt->userData,
		    "Space required after 'PUBLIC'\n");
	    ctxt->wellFormed = 0;
	}
        SKIP_BLANKS;
	*publicID = htmlParsePubidLiteral(ctxt);
	if (*publicID == NULL) {
	    if ((ctxt->sax != NULL) && (ctxt->sax->error != NULL))
	        ctxt->sax->error(ctxt->userData, 
	          "htmlParseExternalID: PUBLIC, no Public Identifier\n");
	    ctxt->wellFormed = 0;
	}
        SKIP_BLANKS;
        if ((CUR == '"') || (CUR == '\'')) {
	    URI = htmlParseSystemLiteral(ctxt);
	}
    }
    return(URI);
}

/**
 * htmlParseComment:
 * @ctxt:  an HTML parser context
 *
 * Parse an XML (SGML) comment <!-- .... -->
 *
 * [15] Comment ::= '<!--' ((Char - '-') | ('-' (Char - '-')))* '-->'
 */
static void
htmlParseComment(htmlParserCtxtPtr ctxt) {
    xmlChar *buf = NULL;
    int len;
    int size = HTML_PARSER_BUFFER_SIZE;
    int q, ql;
    int r, rl;
    int cur, l;
    xmlParserInputState state;

    /*
     * Check that there is a comment right here.
     */
    if ((RAW != '<') || (NXT(1) != '!') ||
        (NXT(2) != '-') || (NXT(3) != '-')) return;

    state = ctxt->instate;
    ctxt->instate = XML_PARSER_COMMENT;
    SHRINK;
    SKIP(4);
    buf = (xmlChar *) xmlMalloc(size * sizeof(xmlChar));
    if (buf == NULL) {
	xmlGenericError(xmlGenericErrorContext,
		"malloc of %d byte failed\n", size);
	ctxt->instate = state;
	return;
    }
    q = CUR_CHAR(ql);
    NEXTL(ql);
    r = CUR_CHAR(rl);
    NEXTL(rl);
    cur = CUR_CHAR(l);
    len = 0;
    while (IS_CHAR(cur) &&
           ((cur != '>') ||
	    (r != '-') || (q != '-'))) {
	if (len + 5 >= size) {
	    size *= 2;
	    buf = (xmlChar *) xmlRealloc(buf, size * sizeof(xmlChar));
	    if (buf == NULL) {
		xmlGenericError(xmlGenericErrorContext,
			"realloc of %d byte failed\n", size);
		ctxt->instate = state;
		return;
	    }
	}
	COPY_BUF(ql,buf,len,q);
	q = r;
	ql = rl;
	r = cur;
	rl = l;
	NEXTL(l);
	cur = CUR_CHAR(l);
	if (cur == 0) {
	    SHRINK;
	    GROW;
	    cur = CUR_CHAR(l);
	}
    }
    buf[len] = 0;
    if (!IS_CHAR(cur)) {
	ctxt->errNo = XML_ERR_COMMENT_NOT_FINISHED;
	if ((ctxt->sax != NULL) && (ctxt->sax->error != NULL))
	    ctxt->sax->error(ctxt->userData,
	                     "Comment not terminated \n<!--%.50s\n", buf);
	ctxt->wellFormed = 0;
	xmlFree(buf);
    } else {
        NEXT;
	if ((ctxt->sax != NULL) && (ctxt->sax->comment != NULL) &&
	    (!ctxt->disableSAX))
	    ctxt->sax->comment(ctxt->userData, buf);
	xmlFree(buf);
    }
    ctxt->instate = state;
}

/**
 * htmlParseCharRef:
 * @ctxt:  an HTML parser context
 *
 * parse Reference declarations
 *
 * [66] CharRef ::= '&#' [0-9]+ ';' |
 *                  '&#x' [0-9a-fA-F]+ ';'
 *
 * Returns the value parsed (as an int)
 */
int
htmlParseCharRef(htmlParserCtxtPtr ctxt) {
    int val = 0;

    if ((CUR == '&') && (NXT(1) == '#') &&
        (NXT(2) == 'x')) {
	SKIP(3);
	while (CUR != ';') {
	    if ((CUR >= '0') && (CUR <= '9')) 
	        val = val * 16 + (CUR - '0');
	    else if ((CUR >= 'a') && (CUR <= 'f'))
	        val = val * 16 + (CUR - 'a') + 10;
	    else if ((CUR >= 'A') && (CUR <= 'F'))
	        val = val * 16 + (CUR - 'A') + 10;
	    else {
	        if ((ctxt->sax != NULL) && (ctxt->sax->error != NULL))
		    ctxt->sax->error(ctxt->userData, 
		         "htmlParseCharRef: invalid hexadecimal value\n");
		ctxt->wellFormed = 0;
		return(0);
	    }
	    NEXT;
	}
	if (CUR == ';')
	    NEXT;
    } else if  ((CUR == '&') && (NXT(1) == '#')) {
	SKIP(2);
	while (CUR != ';') {
	    if ((CUR >= '0') && (CUR <= '9')) 
	        val = val * 10 + (CUR - '0');
	    else {
	        if ((ctxt->sax != NULL) && (ctxt->sax->error != NULL))
		    ctxt->sax->error(ctxt->userData, 
		         "htmlParseCharRef: invalid decimal value\n");
		ctxt->wellFormed = 0;
		return(0);
	    }
	    NEXT;
	}
	if (CUR == ';')
	    NEXT;
    } else {
	if ((ctxt->sax != NULL) && (ctxt->sax->error != NULL))
	    ctxt->sax->error(ctxt->userData, "htmlParseCharRef: invalid value\n");
	ctxt->wellFormed = 0;
    }
    /*
     * Check the value IS_CHAR ...
     */
    if (IS_CHAR(val)) {
        return(val);
    } else {
	if ((ctxt->sax != NULL) && (ctxt->sax->error != NULL))
	    ctxt->sax->error(ctxt->userData, "htmlParseCharRef: invalid xmlChar value %d\n",
	                     val);
	ctxt->wellFormed = 0;
    }
    return(0);
}


/**
 * htmlParseDocTypeDecl :
 * @ctxt:  an HTML parser context
 *
 * parse a DOCTYPE declaration
 *
 * [28] doctypedecl ::= '<!DOCTYPE' S Name (S ExternalID)? S? 
 *                      ('[' (markupdecl | PEReference | S)* ']' S?)? '>'
 */

static void
htmlParseDocTypeDecl(htmlParserCtxtPtr ctxt) {
    xmlChar *name;
    xmlChar *ExternalID = NULL;
    xmlChar *URI = NULL;

    /*
     * We know that '<!DOCTYPE' has been detected.
     */
    SKIP(9);

    SKIP_BLANKS;

    /*
     * Parse the DOCTYPE name.
     */
    name = htmlParseName(ctxt);
    if (name == NULL) {
	if ((ctxt->sax != NULL) && (ctxt->sax->error != NULL))
	    ctxt->sax->error(ctxt->userData, "htmlParseDocTypeDecl : no DOCTYPE name !\n");
	ctxt->wellFormed = 0;
    }
    /*
     * Check that upper(name) == "HTML" !!!!!!!!!!!!!
     */

    SKIP_BLANKS;

    /*
     * Check for SystemID and ExternalID
     */
    URI = htmlParseExternalID(ctxt, &ExternalID);
    SKIP_BLANKS;

    /*
     * We should be at the end of the DOCTYPE declaration.
     */
    if (CUR != '>') {
	if ((ctxt->sax != NULL) && (ctxt->sax->error != NULL))
	    ctxt->sax->error(ctxt->userData, "DOCTYPE improperly terminated\n");
	ctxt->wellFormed = 0;
        /* We shouldn't try to resynchronize ... */
    }
    NEXT;

    /*
     * Create or update the document accordingly to the DOCTYPE
     */
    if ((ctxt->sax != NULL) && (ctxt->sax->internalSubset != NULL) &&
	(!ctxt->disableSAX))
	ctxt->sax->internalSubset(ctxt->userData, name, ExternalID, URI);

    /*
     * Cleanup, since we don't use all those identifiers
     */
    if (URI != NULL) xmlFree(URI);
    if (ExternalID != NULL) xmlFree(ExternalID);
    if (name != NULL) xmlFree(name);
}

/**
 * htmlParseAttribute:
 * @ctxt:  an HTML parser context
 * @value:  a xmlChar ** used to store the value of the attribute
 *
 * parse an attribute
 *
 * [41] Attribute ::= Name Eq AttValue
 *
 * [25] Eq ::= S? '=' S?
 *
 * With namespace:
 *
 * [NS 11] Attribute ::= QName Eq AttValue
 *
 * Also the case QName == xmlns:??? is handled independently as a namespace
 * definition.
 *
 * Returns the attribute name, and the value in *value.
 */

static xmlChar *
htmlParseAttribute(htmlParserCtxtPtr ctxt, xmlChar **value) {
    xmlChar *name, *val = NULL;

    *value = NULL;
    name = htmlParseHTMLName(ctxt);
    if (name == NULL) {
	if ((ctxt->sax != NULL) && (ctxt->sax->error != NULL))
	    ctxt->sax->error(ctxt->userData, "error parsing attribute name\n");
	ctxt->wellFormed = 0;
        return(NULL);
    }

    /*
     * read the value
     */
    SKIP_BLANKS;
    if (CUR == '=') {
        NEXT;
	SKIP_BLANKS;
	val = htmlParseAttValue(ctxt);
	/******
    } else {
        * TODO : some attribute must have values, some may not
	if ((ctxt->sax != NULL) && (ctxt->sax->error != NULL))
	    ctxt->sax->warning(ctxt->userData,
	       "No value for attribute %s\n", name); */
    }

    *value = val;
    return(name);
}

/**
 * htmlCheckEncoding:
 * @ctxt:  an HTML parser context
 * @attvalue: the attribute value
 *
 * Checks an http-equiv attribute from a Meta tag to detect
 * the encoding
 * If a new encoding is detected the parser is switched to decode
 * it and pass UTF8
 */
static void
htmlCheckEncoding(htmlParserCtxtPtr ctxt, const xmlChar *attvalue) {
    const xmlChar *encoding;

    if ((ctxt == NULL) || (attvalue == NULL))
	return;

    /* do not change encoding */	
    if (ctxt->input->encoding != NULL)
        return;

    encoding = xmlStrcasestr(attvalue, BAD_CAST"charset=");
    if (encoding != NULL) {
	encoding += 8;
    } else {
	encoding = xmlStrcasestr(attvalue, BAD_CAST"charset =");
	if (encoding != NULL)
	    encoding += 9;
    }
    if (encoding != NULL) {
	xmlCharEncoding enc;
	xmlCharEncodingHandlerPtr handler;

	while ((*encoding == ' ') || (*encoding == '\t')) encoding++;

	if (ctxt->input->encoding != NULL)
	    xmlFree((xmlChar *) ctxt->input->encoding);
	ctxt->input->encoding = xmlStrdup(encoding);

	enc = xmlParseCharEncoding((const char *) encoding);
	/*
	 * registered set of known encodings
	 */
	if (enc != XML_CHAR_ENCODING_ERROR) {
	    xmlSwitchEncoding(ctxt, enc);
	    ctxt->charset = XML_CHAR_ENCODING_UTF8;
	} else {
	    /*
	     * fallback for unknown encodings
	     */
	    handler = xmlFindCharEncodingHandler((const char *) encoding);
	    if (handler != NULL) {
		xmlSwitchToEncoding(ctxt, handler);
		ctxt->charset = XML_CHAR_ENCODING_UTF8;
	    } else {
		ctxt->errNo = XML_ERR_UNSUPPORTED_ENCODING;
	    }
	}

	if ((ctxt->input->buf != NULL) &&
	    (ctxt->input->buf->encoder != NULL) &&
	    (ctxt->input->buf->raw != NULL) &&
	    (ctxt->input->buf->buffer != NULL)) {
	    int nbchars;
	    int processed;

	    /*
	     * convert as much as possible to the parser reading buffer.
	     */
	    processed = ctxt->input->cur - ctxt->input->base;
	    xmlBufferShrink(ctxt->input->buf->buffer, processed);
	    nbchars = xmlCharEncInFunc(ctxt->input->buf->encoder,
		                       ctxt->input->buf->buffer,
				       ctxt->input->buf->raw);
	    if (nbchars < 0) {
		ctxt->errNo = XML_ERR_INVALID_ENCODING;
		if ((ctxt->sax != NULL) && (ctxt->sax->error != NULL))
		    ctxt->sax->error(ctxt->userData, 
		     "htmlCheckEncoding: encoder error\n");
	    }
	    ctxt->input->base =
	    ctxt->input->cur = ctxt->input->buf->buffer->content;
	}
    }
}

/**
 * htmlCheckMeta:
 * @ctxt:  an HTML parser context
 * @atts:  the attributes values
 *
 * Checks an attributes from a Meta tag
 */
static void
htmlCheckMeta(htmlParserCtxtPtr ctxt, const xmlChar **atts) {
    int i;
    const xmlChar *att, *value;
    int http = 0;
    const xmlChar *content = NULL;

    if ((ctxt == NULL) || (atts == NULL))
	return;

    i = 0;
    att = atts[i++];
    while (att != NULL) {
	value = atts[i++];
	if ((value != NULL) && (!xmlStrcasecmp(att, BAD_CAST"http-equiv"))
	 && (!xmlStrcasecmp(value, BAD_CAST"Content-Type")))
	    http = 1;
	else if ((value != NULL) && (!xmlStrcasecmp(att, BAD_CAST"content")))
	    content = value;
	att = atts[i++];
    }
    if ((http) && (content != NULL))
	htmlCheckEncoding(ctxt, content);

}

/**
 * htmlParseStartTag:
 * @ctxt:  an HTML parser context
 * 
 * parse a start of tag either for rule element or
 * EmptyElement. In both case we don't parse the tag closing chars.
 *
 * [40] STag ::= '<' Name (S Attribute)* S? '>'
 *
 * [44] EmptyElemTag ::= '<' Name (S Attribute)* S? '/>'
 *
 * With namespace:
 *
 * [NS 8] STag ::= '<' QName (S Attribute)* S? '>'
 *
 * [NS 10] EmptyElement ::= '<' QName (S Attribute)* S? '/>'
 *
 */

static void
htmlParseStartTag(htmlParserCtxtPtr ctxt) {
    xmlChar *name;
    xmlChar *attname;
    xmlChar *attvalue;
    const xmlChar **atts = NULL;
    int nbatts = 0;
    int maxatts = 0;
    int meta = 0;
    int i;

    if (CUR != '<') return;
    NEXT;

    GROW;
    name = htmlParseHTMLName(ctxt);
    if (name == NULL) {
	if ((ctxt->sax != NULL) && (ctxt->sax->error != NULL))
	    ctxt->sax->error(ctxt->userData, 
	     "htmlParseStartTag: invalid element name\n");
	ctxt->wellFormed = 0;
	/* Dump the bogus tag like browsers do */
	while ((IS_CHAR(CUR)) && (CUR != '>'))
	    NEXT;
        return;
    }
    if (xmlStrEqual(name, BAD_CAST"meta"))
	meta = 1;

    /*
     * Check for auto-closure of HTML elements.
     */
    htmlAutoClose(ctxt, name);

    /*
     * Check for implied HTML elements.
     */
    htmlCheckImplied(ctxt, name);

    /*
     * Avoid html at any level > 0, head at any level != 1
     * or any attempt to recurse body
     */
    if ((ctxt->nameNr > 0) && (xmlStrEqual(name, BAD_CAST"html"))) {
	if ((ctxt->sax != NULL) && (ctxt->sax->error != NULL))
	    ctxt->sax->error(ctxt->userData, 
	     "htmlParseStartTag: misplaced <html> tag\n");
	ctxt->wellFormed = 0;
	xmlFree(name);
	return;
    }
    if ((ctxt->nameNr != 1) && 
	(xmlStrEqual(name, BAD_CAST"head"))) {
	if ((ctxt->sax != NULL) && (ctxt->sax->error != NULL))
	    ctxt->sax->error(ctxt->userData, 
	     "htmlParseStartTag: misplaced <head> tag\n");
	ctxt->wellFormed = 0;
	xmlFree(name);
	return;
    }
    if (xmlStrEqual(name, BAD_CAST"body")) {
	int indx;
	for (indx = 0;indx < ctxt->nameNr;indx++) {
	    if (xmlStrEqual(ctxt->nameTab[indx], BAD_CAST"body")) {
		if ((ctxt->sax != NULL) && (ctxt->sax->error != NULL))
		    ctxt->sax->error(ctxt->userData, 
		     "htmlParseStartTag: misplaced <body> tag\n");
		ctxt->wellFormed = 0;
		xmlFree(name);
		return;
	    }
	}
    }

    /*
     * Now parse the attributes, it ends up with the ending
     *
     * (S Attribute)* S?
     */
    SKIP_BLANKS;
    while ((IS_CHAR(CUR)) &&
           (CUR != '>') && 
	   ((CUR != '/') || (NXT(1) != '>'))) {
	long cons = ctxt->nbChars;

	GROW;
	attname = htmlParseAttribute(ctxt, &attvalue);
        if (attname != NULL) {

	    /*
	     * Well formedness requires at most one declaration of an attribute
	     */
	    for (i = 0; i < nbatts;i += 2) {
	        if (xmlStrEqual(atts[i], attname)) {
		    if ((ctxt->sax != NULL) && (ctxt->sax->error != NULL))
			ctxt->sax->error(ctxt->userData,
			                 "Attribute %s redefined\n",
			                 attname);
		    ctxt->wellFormed = 0;
		    xmlFree(attname);
		    if (attvalue != NULL)
			xmlFree(attvalue);
		    goto failed;
		}
	    }

	    /*
	     * Add the pair to atts
	     */
	    if (atts == NULL) {
	        maxatts = 10;
	        atts = (const xmlChar **) xmlMalloc(maxatts * sizeof(xmlChar *));
		if (atts == NULL) {
		    xmlGenericError(xmlGenericErrorContext,
			    "malloc of %ld byte failed\n",
			    maxatts * (long)sizeof(xmlChar *));
		    if (name != NULL) xmlFree(name);
		    return;
		}
	    } else if (nbatts + 4 > maxatts) {
	        maxatts *= 2;
	        atts = (const xmlChar **) xmlRealloc((void *) atts,
			                             maxatts * sizeof(xmlChar *));
		if (atts == NULL) {
		    xmlGenericError(xmlGenericErrorContext,
			    "realloc of %ld byte failed\n",
			    maxatts * (long)sizeof(xmlChar *));
		    if (name != NULL) xmlFree(name);
		    return;
		}
	    }
	    atts[nbatts++] = attname;
	    atts[nbatts++] = attvalue;
	    atts[nbatts] = NULL;
	    atts[nbatts + 1] = NULL;
	}
	else {
	    /* Dump the bogus attribute string up to the next blank or
	     * the end of the tag. */
	    while ((IS_CHAR(CUR)) && !(IS_BLANK(CUR)) && (CUR != '>')
	     && ((CUR != '/') || (NXT(1) != '>')))
		NEXT;
	}

failed:
	SKIP_BLANKS;
        if (cons == ctxt->nbChars) {
	    if ((ctxt->sax != NULL) && (ctxt->sax->error != NULL))
	        ctxt->sax->error(ctxt->userData, 
	         "htmlParseStartTag: problem parsing attributes\n");
	    ctxt->wellFormed = 0;
	    break;
	}
    }

    /*
     * Handle specific association to the META tag
     */
    if (meta)
	htmlCheckMeta(ctxt, atts);

    /*
     * SAX: Start of Element !
     */
    htmlnamePush(ctxt, xmlStrdup(name));
#ifdef DEBUG
    xmlGenericError(xmlGenericErrorContext,"Start of element %s: pushed %s\n", name, ctxt->name);
#endif    
    if ((ctxt->sax != NULL) && (ctxt->sax->startElement != NULL))
        ctxt->sax->startElement(ctxt->userData, name, atts);

    if (atts != NULL) {
        for (i = 0;i < nbatts;i++) {
	    if (atts[i] != NULL)
		xmlFree((xmlChar *) atts[i]);
	}
	xmlFree((void *) atts);
    }
    if (name != NULL) xmlFree(name);
}

/**
 * htmlParseEndTag:
 * @ctxt:  an HTML parser context
 *
 * parse an end of tag
 *
 * [42] ETag ::= '</' Name S? '>'
 *
 * With namespace
 *
 * [NS 9] ETag ::= '</' QName S? '>'
 *
 * Returns 1 if the current level should be closed.
 */

static int
htmlParseEndTag(htmlParserCtxtPtr ctxt) {
    xmlChar *name;
    xmlChar *oldname;
    int i, ret;

    if ((CUR != '<') || (NXT(1) != '/')) {
	if ((ctxt->sax != NULL) && (ctxt->sax->error != NULL))
	    ctxt->sax->error(ctxt->userData, "htmlParseEndTag: '</' not found\n");
	ctxt->wellFormed = 0;
	return(0);
    }
    SKIP(2);

    name = htmlParseHTMLName(ctxt);
    if (name == NULL) return(0);

    /*
     * We should definitely be at the ending "S? '>'" part
     */
    SKIP_BLANKS;
    if ((!IS_CHAR(CUR)) || (CUR != '>')) {
	if ((ctxt->sax != NULL) && (ctxt->sax->error != NULL))
	    ctxt->sax->error(ctxt->userData, "End tag : expected '>'\n");
	ctxt->wellFormed = 0;
    } else
	NEXT;

    /*
     * If the name read is not one of the element in the parsing stack
     * then return, it's just an error.
     */
    for (i = (ctxt->nameNr - 1);i >= 0;i--) {
        if (xmlStrEqual(name, ctxt->nameTab[i])) break;
    }
    if (i < 0) {
	if ((ctxt->sax != NULL) && (ctxt->sax->error != NULL))
	    ctxt->sax->error(ctxt->userData,
	     "Unexpected end tag : %s\n", name);
	xmlFree(name);
	ctxt->wellFormed = 0;
	return(0);
    }


    /*
     * Check for auto-closure of HTML elements.
     */

    htmlAutoCloseOnClose(ctxt, name);

    /*
     * Well formedness constraints, opening and closing must match.
     * With the exception that the autoclose may have popped stuff out
     * of the stack.
     */
    if (!xmlStrEqual(name, ctxt->name)) {
#ifdef DEBUG
	xmlGenericError(xmlGenericErrorContext,"End of tag %s: expecting %s\n", name, ctxt->name);
#endif
        if ((ctxt->name != NULL) && 
	    (!xmlStrEqual(ctxt->name, name))) {
	    if ((ctxt->sax != NULL) && (ctxt->sax->error != NULL))
		ctxt->sax->error(ctxt->userData,
		 "Opening and ending tag mismatch: %s and %s\n",
		                 name, ctxt->name);
	    ctxt->wellFormed = 0;
        }
    }

    /*
     * SAX: End of Tag
     */
    oldname = ctxt->name;
    if ((oldname != NULL) && (xmlStrEqual(oldname, name))) {
	if ((ctxt->sax != NULL) && (ctxt->sax->endElement != NULL))
	    ctxt->sax->endElement(ctxt->userData, name);
	oldname = htmlnamePop(ctxt);
	if (oldname != NULL) {
#ifdef DEBUG
	    xmlGenericError(xmlGenericErrorContext,"End of tag %s: popping out %s\n", name, oldname);
#endif
	    xmlFree(oldname);
#ifdef DEBUG
	} else {
	    xmlGenericError(xmlGenericErrorContext,"End of tag %s: stack empty !!!\n", name);
#endif
	}
	ret = 1;
    } else {
	ret = 0;
    }

    if (name != NULL)
	xmlFree(name);

    return(ret);
}


/**
 * htmlParseReference:
 * @ctxt:  an HTML parser context
 * 
 * parse and handle entity references in content,
 * this will end-up in a call to character() since this is either a
 * CharRef, or a predefined entity.
 */
static void
htmlParseReference(htmlParserCtxtPtr ctxt) {
    const htmlEntityDesc * ent;
    xmlChar out[6];
    xmlChar *name;
    if (CUR != '&') return;

    if (NXT(1) == '#') {
	unsigned int c;
	int bits, i = 0;

	c = htmlParseCharRef(ctxt);
	if (c == 0)
	    return;

        if      (c <    0x80) { out[i++]= c;                bits= -6; }
        else if (c <   0x800) { out[i++]=((c >>  6) & 0x1F) | 0xC0;  bits=  0; }
        else if (c < 0x10000) { out[i++]=((c >> 12) & 0x0F) | 0xE0;  bits=  6; }
        else                  { out[i++]=((c >> 18) & 0x07) | 0xF0;  bits= 12; }
 
        for ( ; bits >= 0; bits-= 6) {
            out[i++]= ((c >> bits) & 0x3F) | 0x80;
        }
	out[i] = 0;

	htmlCheckParagraph(ctxt);
	if ((ctxt->sax != NULL) && (ctxt->sax->characters != NULL))
	    ctxt->sax->characters(ctxt->userData, out, i);
    } else {
	ent = htmlParseEntityRef(ctxt, &name);
	if (name == NULL) {
	    htmlCheckParagraph(ctxt);
	    if ((ctxt->sax != NULL) && (ctxt->sax->characters != NULL))
	        ctxt->sax->characters(ctxt->userData, BAD_CAST "&", 1);
	    return;
	}
	if ((ent == NULL) || (ent->value <= 0)) {
	    htmlCheckParagraph(ctxt);
	    if ((ctxt->sax != NULL) && (ctxt->sax->characters != NULL)) {
		ctxt->sax->characters(ctxt->userData, BAD_CAST "&", 1);
		ctxt->sax->characters(ctxt->userData, name, xmlStrlen(name));
		/* ctxt->sax->characters(ctxt->userData, BAD_CAST ";", 1); */
	    }
	} else {
	    unsigned int c;
	    int bits, i = 0;

	    c = ent->value;
	    if      (c <    0x80)
	            { out[i++]= c;                bits= -6; }
	    else if (c <   0x800)
	            { out[i++]=((c >>  6) & 0x1F) | 0xC0;  bits=  0; }
	    else if (c < 0x10000)
	            { out[i++]=((c >> 12) & 0x0F) | 0xE0;  bits=  6; }
	    else                 
	            { out[i++]=((c >> 18) & 0x07) | 0xF0;  bits= 12; }
     
	    for ( ; bits >= 0; bits-= 6) {
		out[i++]= ((c >> bits) & 0x3F) | 0x80;
	    }
	    out[i] = 0;

	    htmlCheckParagraph(ctxt);
	    if ((ctxt->sax != NULL) && (ctxt->sax->characters != NULL))
		ctxt->sax->characters(ctxt->userData, out, i);
	}
	xmlFree(name);
    }
}

/**
 * htmlParseContent:
 * @ctxt:  an HTML parser context
 * @name:  the node name
 *
 * Parse a content: comment, sub-element, reference or text.
 *
 */

static void
htmlParseContent(htmlParserCtxtPtr ctxt) {
    xmlChar *currentNode;
    int depth;

    currentNode = xmlStrdup(ctxt->name);
    depth = ctxt->nameNr;
    while (1) {
	long cons = ctxt->nbChars;

        GROW;
	/*
	 * Our tag or one of it's parent or children is ending.
	 */
        if ((CUR == '<') && (NXT(1) == '/')) {
	    if (htmlParseEndTag(ctxt) &&
		((currentNode != NULL) || (ctxt->nameNr == 0))) {
		if (currentNode != NULL)
		    xmlFree(currentNode);
		return;
	    }
	    continue; /* while */
        }

	/*
	 * Has this node been popped out during parsing of
	 * the next element
	 */
        if ((ctxt->nameNr > 0) && (depth >= ctxt->nameNr) &&
	    (!xmlStrEqual(currentNode, ctxt->name)))
	     {
	    if (currentNode != NULL) xmlFree(currentNode);
	    return;
	}

	if ((CUR != 0) && ((xmlStrEqual(currentNode, BAD_CAST"script")) ||
	    (xmlStrEqual(currentNode, BAD_CAST"style")))) {
	    /*
	     * Handle SCRIPT/STYLE separately
	     */
	    htmlParseScript(ctxt);
	} else {
	    /*
	     * Sometimes DOCTYPE arrives in the middle of the document
	     */
	    if ((CUR == '<') && (NXT(1) == '!') &&
		(UPP(2) == 'D') && (UPP(3) == 'O') &&
		(UPP(4) == 'C') && (UPP(5) == 'T') &&
		(UPP(6) == 'Y') && (UPP(7) == 'P') &&
		(UPP(8) == 'E')) {
		if ((ctxt->sax != NULL) && (ctxt->sax->error != NULL))
		    ctxt->sax->error(ctxt->userData,
				     "Misplaced DOCTYPE declaration\n");
		ctxt->wellFormed = 0;
		htmlParseDocTypeDecl(ctxt);
	    }

	    /*
	     * First case :  a comment
	     */
	    if ((CUR == '<') && (NXT(1) == '!') &&
		(NXT(2) == '-') && (NXT(3) == '-')) {
		htmlParseComment(ctxt);
	    }

	    /*
	     * Second case :  a sub-element.
	     */
	    else if (CUR == '<') {
		htmlParseElement(ctxt);
	    }

	    /*
	     * Third case : a reference. If if has not been resolved,
	     *    parsing returns it's Name, create the node 
	     */
	    else if (CUR == '&') {
		htmlParseReference(ctxt);
	    }

	    /*
	     * Fourth : end of the resource
	     */
	    else if (CUR == 0) {
		htmlAutoCloseOnEnd(ctxt);
		break;
	    }

	    /*
	     * Last case, text. Note that References are handled directly.
	     */
	    else {
		htmlParseCharData(ctxt);
	    }

	    if (cons == ctxt->nbChars) {
		if (ctxt->node != NULL) {
		    if ((ctxt->sax != NULL) && (ctxt->sax->error != NULL))
			ctxt->sax->error(ctxt->userData,
					 "detected an error in element content\n");
		    ctxt->wellFormed = 0;
		}
		break;
	    }
	}
        GROW;
    }
    if (currentNode != NULL) xmlFree(currentNode);
}

/**
 * htmlParseElement:
 * @ctxt:  an HTML parser context
 *
 * parse an HTML element, this is highly recursive
 *
 * [39] element ::= EmptyElemTag | STag content ETag
 *
 * [41] Attribute ::= Name Eq AttValue
 */

void
htmlParseElement(htmlParserCtxtPtr ctxt) {
    xmlChar *name;
    xmlChar *currentNode = NULL;
    const htmlElemDesc * info;
    htmlParserNodeInfo node_info;
    xmlChar *oldname;
    int depth = ctxt->nameNr;
    const xmlChar *oldptr;

    /* Capture start position */
    if (ctxt->record_info) {
        node_info.begin_pos = ctxt->input->consumed +
                          (CUR_PTR - ctxt->input->base);
	node_info.begin_line = ctxt->input->line;
    }

    oldname = xmlStrdup(ctxt->name);
    htmlParseStartTag(ctxt);
    name = ctxt->name;
#ifdef DEBUG
    if (oldname == NULL)
	xmlGenericError(xmlGenericErrorContext,
		"Start of element %s\n", name);
    else if (name == NULL)	
	xmlGenericError(xmlGenericErrorContext,
		"Start of element failed, was %s\n", oldname);
    else	
	xmlGenericError(xmlGenericErrorContext,
		"Start of element %s, was %s\n", name, oldname);
#endif
    if (((depth == ctxt->nameNr) && (xmlStrEqual(oldname, ctxt->name))) ||
        (name == NULL)) {
	if (CUR == '>')
	    NEXT;
	if (oldname != NULL)
	    xmlFree(oldname);
        return;
    }
    if (oldname != NULL)
	xmlFree(oldname);

    /*
     * Lookup the info for that element.
     */
    info = htmlTagLookup(name);
    if (info == NULL) {
	if ((ctxt->sax != NULL) && (ctxt->sax->error != NULL))
	    ctxt->sax->error(ctxt->userData, "Tag %s invalid\n",
			     name);
	ctxt->wellFormed = 0;
    } else if (info->depr) {
/***************************
	if ((ctxt->sax != NULL) && (ctxt->sax->warning != NULL))
	    ctxt->sax->warning(ctxt->userData, "Tag %s is deprecated\n",
			       name);
 ***************************/
    }

    /*
     * Check for an Empty Element labelled the XML/SGML way
     */
    if ((CUR == '/') && (NXT(1) == '>')) {
        SKIP(2);
	if ((ctxt->sax != NULL) && (ctxt->sax->endElement != NULL))
	    ctxt->sax->endElement(ctxt->userData, name);
	oldname = htmlnamePop(ctxt);
#ifdef DEBUG
        xmlGenericError(xmlGenericErrorContext,"End of tag the XML way: popping out %s\n", oldname);
#endif
	if (oldname != NULL)
	    xmlFree(oldname);
	return;
    }

    if (CUR == '>') {
        NEXT;
    } else {
	if ((ctxt->sax != NULL) && (ctxt->sax->error != NULL))
	    ctxt->sax->error(ctxt->userData,
		             "Couldn't find end of Start Tag %s\n",
	                     name);
	ctxt->wellFormed = 0;

	/*
	 * end of parsing of this node.
	 */
	if (xmlStrEqual(name, ctxt->name)) { 
	    nodePop(ctxt);
	    oldname = htmlnamePop(ctxt);
#ifdef DEBUG
	    xmlGenericError(xmlGenericErrorContext,"End of start tag problem: popping out %s\n", oldname);
#endif
	    if (oldname != NULL)
		xmlFree(oldname);
	}    

	/*
	 * Capture end position and add node
	 */
	if ( currentNode != NULL && ctxt->record_info ) {
	   node_info.end_pos = ctxt->input->consumed +
			      (CUR_PTR - ctxt->input->base);
	   node_info.end_line = ctxt->input->line;
	   node_info.node = ctxt->node;
	   xmlParserAddNodeInfo(ctxt, &node_info);
	}
	return;
    }

    /*
     * Check for an Empty Element from DTD definition
     */
    if ((info != NULL) && (info->empty)) {
	if ((ctxt->sax != NULL) && (ctxt->sax->endElement != NULL))
	    ctxt->sax->endElement(ctxt->userData, name);
	oldname = htmlnamePop(ctxt);
#ifdef DEBUG
	xmlGenericError(xmlGenericErrorContext,"End of empty tag %s : popping out %s\n", name, oldname);
#endif
	if (oldname != NULL)
	    xmlFree(oldname);
	return;
    }

    /*
     * Parse the content of the element:
     */
    currentNode = xmlStrdup(ctxt->name);
    depth = ctxt->nameNr;
    while (IS_CHAR(CUR)) {
	oldptr = ctxt->input->cur;
	htmlParseContent(ctxt);
	if (oldptr==ctxt->input->cur) break;
	if (ctxt->nameNr < depth) break; 
    }	

    /*
     * Capture end position and add node
     */
    if ( currentNode != NULL && ctxt->record_info ) {
       node_info.end_pos = ctxt->input->consumed +
                          (CUR_PTR - ctxt->input->base);
       node_info.end_line = ctxt->input->line;
       node_info.node = ctxt->node;
       xmlParserAddNodeInfo(ctxt, &node_info);
    }
    if (!IS_CHAR(CUR)) {
	htmlAutoCloseOnEnd(ctxt);
    }

    if (currentNode != NULL)
	xmlFree(currentNode);
}

/**
 * htmlParseDocument :
 * @ctxt:  an HTML parser context
 * 
 * parse an HTML document (and build a tree if using the standard SAX
 * interface).
 *
 * Returns 0, -1 in case of error. the parser context is augmented
 *                as a result of the parsing.
 */

static int
htmlParseDocument(htmlParserCtxtPtr ctxt) {
    xmlDtdPtr dtd;

    xmlInitParser();

    htmlDefaultSAXHandlerInit();
    ctxt->html = 1;

    GROW;
    /*
     * SAX: beginning of the document processing.
     */
    if ((ctxt->sax) && (ctxt->sax->setDocumentLocator))
        ctxt->sax->setDocumentLocator(ctxt->userData, &xmlDefaultSAXLocator);

    /*
     * Wipe out everything which is before the first '<'
     */
    SKIP_BLANKS;
    if (CUR == 0) {
	if ((ctxt->sax != NULL) && (ctxt->sax->error != NULL))
	    ctxt->sax->error(ctxt->userData, "Document is empty\n");
	ctxt->wellFormed = 0;
    }

    if ((ctxt->sax) && (ctxt->sax->startDocument) && (!ctxt->disableSAX))
	ctxt->sax->startDocument(ctxt->userData);


    /*
     * Parse possible comments before any content
     */
    while ((CUR == '<') && (NXT(1) == '!') &&
           (NXT(2) == '-') && (NXT(3) == '-')) {
        htmlParseComment(ctxt);	   
	SKIP_BLANKS;
    }	   


    /*
     * Then possibly doc type declaration(s) and more Misc
     * (doctypedecl Misc*)?
     */
    if ((CUR == '<') && (NXT(1) == '!') &&
	(UPP(2) == 'D') && (UPP(3) == 'O') &&
	(UPP(4) == 'C') && (UPP(5) == 'T') &&
	(UPP(6) == 'Y') && (UPP(7) == 'P') &&
	(UPP(8) == 'E')) {
	htmlParseDocTypeDecl(ctxt);
    }
    SKIP_BLANKS;

    /*
     * Parse possible comments before any content
     */
    while ((CUR == '<') && (NXT(1) == '!') &&
           (NXT(2) == '-') && (NXT(3) == '-')) {
        htmlParseComment(ctxt);	   
	SKIP_BLANKS;
    }	   

    /*
     * Time to start parsing the tree itself
     */
    htmlParseContent(ctxt);

    /*
     * autoclose
     */
    if (CUR == 0)
	htmlAutoCloseOnEnd(ctxt);


    /*
     * SAX: end of the document processing.
     */
    if ((ctxt->sax) && (ctxt->sax->endDocument != NULL))
        ctxt->sax->endDocument(ctxt->userData);

    if (ctxt->myDoc != NULL) {
	dtd = xmlGetIntSubset(ctxt->myDoc);
	if (dtd == NULL)
	    ctxt->myDoc->intSubset = 
		xmlCreateIntSubset(ctxt->myDoc, BAD_CAST "HTML", 
		    BAD_CAST "-//W3C//DTD HTML 4.0 Transitional//EN",
		    BAD_CAST "http://www.w3.org/TR/REC-html40/loose.dtd");
    }
    if (! ctxt->wellFormed) return(-1);
    return(0);
}


/************************************************************************
 *									*
 *			Parser contexts handling			*
 *									*
 ************************************************************************/

/**
 * xmlInitParserCtxt:
 * @ctxt:  an HTML parser context
 *
 * Initialize a parser context
 */

static void
htmlInitParserCtxt(htmlParserCtxtPtr ctxt)
{
    htmlSAXHandler *sax;

    if (ctxt == NULL) return;
    memset(ctxt, 0, sizeof(htmlParserCtxt));

    sax = (htmlSAXHandler *) xmlMalloc(sizeof(htmlSAXHandler));
    if (sax == NULL) {
        xmlGenericError(xmlGenericErrorContext,
		"htmlInitParserCtxt: out of memory\n");
    }
    else
        memset(sax, 0, sizeof(htmlSAXHandler));

    /* Allocate the Input stack */
    ctxt->inputTab = (htmlParserInputPtr *) 
                      xmlMalloc(5 * sizeof(htmlParserInputPtr));
    if (ctxt->inputTab == NULL) {
        xmlGenericError(xmlGenericErrorContext,
		"htmlInitParserCtxt: out of memory\n");
	ctxt->inputNr = 0;
	ctxt->inputMax = 0;
	ctxt->input = NULL;
	return;
    }
    ctxt->inputNr = 0;
    ctxt->inputMax = 5;
    ctxt->input = NULL;
    ctxt->version = NULL;
    ctxt->encoding = NULL;
    ctxt->standalone = -1;
    ctxt->instate = XML_PARSER_START;

    /* Allocate the Node stack */
    ctxt->nodeTab = (htmlNodePtr *) xmlMalloc(10 * sizeof(htmlNodePtr));
    if (ctxt->nodeTab == NULL) {
        xmlGenericError(xmlGenericErrorContext,
		"htmlInitParserCtxt: out of memory\n");
	ctxt->nodeNr = 0;
	ctxt->nodeMax = 0;
	ctxt->node = NULL;
	ctxt->inputNr = 0;
	ctxt->inputMax = 0;
	ctxt->input = NULL;
	return;
    }
    ctxt->nodeNr = 0;
    ctxt->nodeMax = 10;
    ctxt->node = NULL;

    /* Allocate the Name stack */
    ctxt->nameTab = (xmlChar **) xmlMalloc(10 * sizeof(xmlChar *));
    if (ctxt->nameTab == NULL) {
        xmlGenericError(xmlGenericErrorContext,
		"htmlInitParserCtxt: out of memory\n");
	ctxt->nameNr = 0;
	ctxt->nameMax = 10;
	ctxt->name = NULL;
	ctxt->nodeNr = 0;
	ctxt->nodeMax = 0;
	ctxt->node = NULL;
	ctxt->inputNr = 0;
	ctxt->inputMax = 0;
	ctxt->input = NULL;
	return;
    }
    ctxt->nameNr = 0;
    ctxt->nameMax = 10;
    ctxt->name = NULL;

    if (sax == NULL) ctxt->sax = &htmlDefaultSAXHandler;
    else {
        ctxt->sax = sax;
	memcpy(sax, &htmlDefaultSAXHandler, sizeof(htmlSAXHandler));
    }
    ctxt->userData = ctxt;
    ctxt->myDoc = NULL;
    ctxt->wellFormed = 1;
    ctxt->replaceEntities = 0;
    ctxt->linenumbers = xmlLineNumbersDefaultValue;
    ctxt->html = 1;
    ctxt->record_info = 0;
    ctxt->validate = 0;
    ctxt->nbChars = 0;
    ctxt->checkIndex = 0;
    ctxt->catalogs = NULL;
    xmlInitNodeInfoSeq(&ctxt->node_seq);
}

/**
 * htmlFreeParserCtxt:
 * @ctxt:  an HTML parser context
 *
 * Free all the memory used by a parser context. However the parsed
 * document in ctxt->myDoc is not freed.
 */

void
htmlFreeParserCtxt(htmlParserCtxtPtr ctxt)
{
    xmlFreeParserCtxt(ctxt);
}

/**
 * htmlCreateDocParserCtxt :
 * @cur:  a pointer to an array of xmlChar
 * @encoding:  a free form C string describing the HTML document encoding, or NULL
 *
 * Create a parser context for an HTML document.
 *
 * TODO: check the need to add encoding handling there
 *
 * Returns the new parser context or NULL
 */
static htmlParserCtxtPtr
htmlCreateDocParserCtxt(xmlChar *cur, const char *encoding ATTRIBUTE_UNUSED) {
    htmlParserCtxtPtr ctxt;
    htmlParserInputPtr input;
    /* htmlCharEncoding enc; */

    ctxt = (htmlParserCtxtPtr) xmlMalloc(sizeof(htmlParserCtxt));
    if (ctxt == NULL) {
        perror("malloc");
	return(NULL);
    }
    htmlInitParserCtxt(ctxt);
    input = (htmlParserInputPtr) xmlMalloc(sizeof(htmlParserInput));
    if (input == NULL) {
        perror("malloc");
	xmlFree(ctxt);
	return(NULL);
    }
    memset(input, 0, sizeof(htmlParserInput));

    input->line = 1;
    input->col = 1;
    input->base = cur;
    input->cur = cur;

    inputPush(ctxt, input);
    return(ctxt);
}

/************************************************************************
 *									*
 * 		Progressive parsing interfaces				*
 *									*
 ************************************************************************/

/**
 * htmlParseLookupSequence:
 * @ctxt:  an HTML parser context
 * @first:  the first char to lookup
 * @next:  the next char to lookup or zero
 * @third:  the next char to lookup or zero
 *
 * Try to find if a sequence (first, next, third) or  just (first next) or
 * (first) is available in the input stream.
 * This function has a side effect of (possibly) incrementing ctxt->checkIndex
 * to avoid rescanning sequences of bytes, it DOES change the state of the
 * parser, do not use liberally.
 * This is basically similar to xmlParseLookupSequence()
 *
 * Returns the index to the current parsing point if the full sequence
 *      is available, -1 otherwise.
 */
static int
htmlParseLookupSequence(htmlParserCtxtPtr ctxt, xmlChar first,
                       xmlChar next, xmlChar third) {
    int base, len;
    htmlParserInputPtr in;
    const xmlChar *buf;

    in = ctxt->input;
    if (in == NULL) return(-1);
    base = in->cur - in->base;
    if (base < 0) return(-1);
    if (ctxt->checkIndex > base)
        base = ctxt->checkIndex;
    if (in->buf == NULL) {
	buf = in->base;
	len = in->length;
    } else {
	buf = in->buf->buffer->content;
	len = in->buf->buffer->use;
    }
    /* take into account the sequence length */
    if (third) len -= 2;
    else if (next) len --;
    for (;base < len;base++) {
        if (buf[base] == first) {
	    if (third != 0) {
		if ((buf[base + 1] != next) ||
		    (buf[base + 2] != third)) continue;
	    } else if (next != 0) {
		if (buf[base + 1] != next) continue;
	    }
	    ctxt->checkIndex = 0;
#ifdef DEBUG_PUSH
	    if (next == 0)
		xmlGenericError(xmlGenericErrorContext,
			"HPP: lookup '%c' found at %d\n",
			first, base);
	    else if (third == 0)
		xmlGenericError(xmlGenericErrorContext,
			"HPP: lookup '%c%c' found at %d\n",
			first, next, base);
	    else 
		xmlGenericError(xmlGenericErrorContext,
			"HPP: lookup '%c%c%c' found at %d\n",
			first, next, third, base);
#endif
	    return(base - (in->cur - in->base));
	}
    }
    ctxt->checkIndex = base;
#ifdef DEBUG_PUSH
    if (next == 0)
	xmlGenericError(xmlGenericErrorContext,
		"HPP: lookup '%c' failed\n", first);
    else if (third == 0)
	xmlGenericError(xmlGenericErrorContext,
		"HPP: lookup '%c%c' failed\n", first, next);
    else	
	xmlGenericError(xmlGenericErrorContext,
		"HPP: lookup '%c%c%c' failed\n", first, next, third);
#endif
    return(-1);
}

/**
 * htmlParseTryOrFinish:
 * @ctxt:  an HTML parser context
 * @terminate:  last chunk indicator
 *
 * Try to progress on parsing
 *
 * Returns zero if no parsing was possible
 */
static int
htmlParseTryOrFinish(htmlParserCtxtPtr ctxt, int terminate) {
    int ret = 0;
    htmlParserInputPtr in;
    int avail = 0;
    xmlChar cur, next;

#ifdef DEBUG_PUSH
    switch (ctxt->instate) {
	case XML_PARSER_EOF:
	    xmlGenericError(xmlGenericErrorContext,
		    "HPP: try EOF\n"); break;
	case XML_PARSER_START:
	    xmlGenericError(xmlGenericErrorContext,
		    "HPP: try START\n"); break;
	case XML_PARSER_MISC:
	    xmlGenericError(xmlGenericErrorContext,
		    "HPP: try MISC\n");break;
	case XML_PARSER_COMMENT:
	    xmlGenericError(xmlGenericErrorContext,
		    "HPP: try COMMENT\n");break;
	case XML_PARSER_PROLOG:
	    xmlGenericError(xmlGenericErrorContext,
		    "HPP: try PROLOG\n");break;
	case XML_PARSER_START_TAG:
	    xmlGenericError(xmlGenericErrorContext,
		    "HPP: try START_TAG\n");break;
	case XML_PARSER_CONTENT:
	    xmlGenericError(xmlGenericErrorContext,
		    "HPP: try CONTENT\n");break;
	case XML_PARSER_CDATA_SECTION:
	    xmlGenericError(xmlGenericErrorContext,
		    "HPP: try CDATA_SECTION\n");break;
	case XML_PARSER_END_TAG:
	    xmlGenericError(xmlGenericErrorContext,
		    "HPP: try END_TAG\n");break;
	case XML_PARSER_ENTITY_DECL:
	    xmlGenericError(xmlGenericErrorContext,
		    "HPP: try ENTITY_DECL\n");break;
	case XML_PARSER_ENTITY_VALUE:
	    xmlGenericError(xmlGenericErrorContext,
		    "HPP: try ENTITY_VALUE\n");break;
	case XML_PARSER_ATTRIBUTE_VALUE:
	    xmlGenericError(xmlGenericErrorContext,
		    "HPP: try ATTRIBUTE_VALUE\n");break;
	case XML_PARSER_DTD:
	    xmlGenericError(xmlGenericErrorContext,
		    "HPP: try DTD\n");break;
	case XML_PARSER_EPILOG:
	    xmlGenericError(xmlGenericErrorContext,
		    "HPP: try EPILOG\n");break;
	case XML_PARSER_PI:
	    xmlGenericError(xmlGenericErrorContext,
		    "HPP: try PI\n");break;
	case XML_PARSER_SYSTEM_LITERAL:
	    xmlGenericError(xmlGenericErrorContext,
		    "HPP: try SYSTEM_LITERAL\n");break;
    }
#endif

    while (1) {

	in = ctxt->input;
	if (in == NULL) break;
	if (in->buf == NULL)
	    avail = in->length - (in->cur - in->base);
	else
	    avail = in->buf->buffer->use - (in->cur - in->base);
	if ((avail == 0) && (terminate)) {
	    htmlAutoCloseOnEnd(ctxt);
	    if ((ctxt->nameNr == 0) && (ctxt->instate != XML_PARSER_EOF)) { 
		/*
		 * SAX: end of the document processing.
		 */
		ctxt->instate = XML_PARSER_EOF;
		if ((ctxt->sax) && (ctxt->sax->endDocument != NULL))
		    ctxt->sax->endDocument(ctxt->userData);
	    }
	}
        if (avail < 1)
	    goto done;
        switch (ctxt->instate) {
            case XML_PARSER_EOF:
	        /*
		 * Document parsing is done !
		 */
	        goto done;
            case XML_PARSER_START:
	        /*
		 * Very first chars read from the document flow.
		 */
		cur = in->cur[0];
		if (IS_BLANK(cur)) {
		    SKIP_BLANKS;
		    if (in->buf == NULL)
			avail = in->length - (in->cur - in->base);
		    else
			avail = in->buf->buffer->use - (in->cur - in->base);
		}
		if ((ctxt->sax) && (ctxt->sax->setDocumentLocator))
		    ctxt->sax->setDocumentLocator(ctxt->userData,
						  &xmlDefaultSAXLocator);
		if ((ctxt->sax) && (ctxt->sax->startDocument) &&
	            (!ctxt->disableSAX))
		    ctxt->sax->startDocument(ctxt->userData);

		cur = in->cur[0];
		next = in->cur[1];
		if ((cur == '<') && (next == '!') &&
		    (UPP(2) == 'D') && (UPP(3) == 'O') &&
		    (UPP(4) == 'C') && (UPP(5) == 'T') &&
		    (UPP(6) == 'Y') && (UPP(7) == 'P') &&
		    (UPP(8) == 'E')) {
		    if ((!terminate) &&
		        (htmlParseLookupSequence(ctxt, '>', 0, 0) < 0))
			goto done;
#ifdef DEBUG_PUSH
		    xmlGenericError(xmlGenericErrorContext,
			    "HPP: Parsing internal subset\n");
#endif
		    htmlParseDocTypeDecl(ctxt);
		    ctxt->instate = XML_PARSER_PROLOG;
#ifdef DEBUG_PUSH
		    xmlGenericError(xmlGenericErrorContext,
			    "HPP: entering PROLOG\n");
#endif
                } else {
		    ctxt->instate = XML_PARSER_MISC;
		}
#ifdef DEBUG_PUSH
		xmlGenericError(xmlGenericErrorContext,
			"HPP: entering MISC\n");
#endif
		break;
            case XML_PARSER_MISC:
		SKIP_BLANKS;
		if (in->buf == NULL)
		    avail = in->length - (in->cur - in->base);
		else
		    avail = in->buf->buffer->use - (in->cur - in->base);
		if (avail < 2)
		    goto done;
		cur = in->cur[0];
		next = in->cur[1];
	        if ((cur == '<') && (next == '!') &&
		    (in->cur[2] == '-') && (in->cur[3] == '-')) {
		    if ((!terminate) &&
		        (htmlParseLookupSequence(ctxt, '-', '-', '>') < 0))
			goto done;
#ifdef DEBUG_PUSH
		    xmlGenericError(xmlGenericErrorContext,
			    "HPP: Parsing Comment\n");
#endif
		    htmlParseComment(ctxt);
		    ctxt->instate = XML_PARSER_MISC;
		} else if ((cur == '<') && (next == '!') &&
		    (UPP(2) == 'D') && (UPP(3) == 'O') &&
		    (UPP(4) == 'C') && (UPP(5) == 'T') &&
		    (UPP(6) == 'Y') && (UPP(7) == 'P') &&
		    (UPP(8) == 'E')) {
		    if ((!terminate) &&
		        (htmlParseLookupSequence(ctxt, '>', 0, 0) < 0))
			goto done;
#ifdef DEBUG_PUSH
		    xmlGenericError(xmlGenericErrorContext,
			    "HPP: Parsing internal subset\n");
#endif
		    htmlParseDocTypeDecl(ctxt);
		    ctxt->instate = XML_PARSER_PROLOG;
#ifdef DEBUG_PUSH
		    xmlGenericError(xmlGenericErrorContext,
			    "HPP: entering PROLOG\n");
#endif
		} else if ((cur == '<') && (next == '!') &&
		           (avail < 9)) {
		    goto done;
		} else {
		    ctxt->instate = XML_PARSER_START_TAG;
#ifdef DEBUG_PUSH
		    xmlGenericError(xmlGenericErrorContext,
			    "HPP: entering START_TAG\n");
#endif
		}
		break;
            case XML_PARSER_PROLOG:
		SKIP_BLANKS;
		if (in->buf == NULL)
		    avail = in->length - (in->cur - in->base);
		else
		    avail = in->buf->buffer->use - (in->cur - in->base);
		if (avail < 2) 
		    goto done;
		cur = in->cur[0];
		next = in->cur[1];
		if ((cur == '<') && (next == '!') &&
		    (in->cur[2] == '-') && (in->cur[3] == '-')) {
		    if ((!terminate) &&
		        (htmlParseLookupSequence(ctxt, '-', '-', '>') < 0))
			goto done;
#ifdef DEBUG_PUSH
		    xmlGenericError(xmlGenericErrorContext,
			    "HPP: Parsing Comment\n");
#endif
		    htmlParseComment(ctxt);
		    ctxt->instate = XML_PARSER_PROLOG;
		} else if ((cur == '<') && (next == '!') &&
		           (avail < 4)) {
		    goto done;
		} else {
		    ctxt->instate = XML_PARSER_START_TAG;
#ifdef DEBUG_PUSH
		    xmlGenericError(xmlGenericErrorContext,
			    "HPP: entering START_TAG\n");
#endif
		}
		break;
            case XML_PARSER_EPILOG:
		if (in->buf == NULL)
		    avail = in->length - (in->cur - in->base);
		else
		    avail = in->buf->buffer->use - (in->cur - in->base);
		if (avail < 1)
		    goto done;
		cur = in->cur[0];
		if (IS_BLANK(cur)) {
		    htmlParseCharData(ctxt);
		    goto done;
		}
		if (avail < 2)
		    goto done;
		next = in->cur[1];
	        if ((cur == '<') && (next == '!') &&
		    (in->cur[2] == '-') && (in->cur[3] == '-')) {
		    if ((!terminate) &&
		        (htmlParseLookupSequence(ctxt, '-', '-', '>') < 0))
			goto done;
#ifdef DEBUG_PUSH
		    xmlGenericError(xmlGenericErrorContext,
			    "HPP: Parsing Comment\n");
#endif
		    htmlParseComment(ctxt);
		    ctxt->instate = XML_PARSER_EPILOG;
		} else if ((cur == '<') && (next == '!') &&
		           (avail < 4)) {
		    goto done;
		} else {
		    ctxt->errNo = XML_ERR_DOCUMENT_END;
		    ctxt->wellFormed = 0;
		    ctxt->instate = XML_PARSER_EOF;
#ifdef DEBUG_PUSH
		    xmlGenericError(xmlGenericErrorContext,
			    "HPP: entering EOF\n");
#endif
		    if ((ctxt->sax) && (ctxt->sax->endDocument != NULL))
			ctxt->sax->endDocument(ctxt->userData);
		    goto done;
		}
		break;
            case XML_PARSER_START_TAG: {
	        xmlChar *name, *oldname;
		int depth = ctxt->nameNr;
		const htmlElemDesc * info;

		if (avail < 2)
		    goto done;
		cur = in->cur[0];
	        if (cur != '<') {
		    ctxt->instate = XML_PARSER_CONTENT;
#ifdef DEBUG_PUSH
		    xmlGenericError(xmlGenericErrorContext,
			    "HPP: entering CONTENT\n");
#endif
		    break;
		}
		if (in->cur[1] == '/') {
		    ctxt->instate = XML_PARSER_END_TAG;
		    ctxt->checkIndex = 0;
#ifdef DEBUG_PUSH
		    xmlGenericError(xmlGenericErrorContext,
			    "HPP: entering END_TAG\n");
#endif
		    break;
		}
		if ((!terminate) &&
		    (htmlParseLookupSequence(ctxt, '>', 0, 0) < 0))
		    goto done;

		oldname = xmlStrdup(ctxt->name);
		htmlParseStartTag(ctxt);
		name = ctxt->name;
#ifdef DEBUG
		if (oldname == NULL)
		    xmlGenericError(xmlGenericErrorContext,
			    "Start of element %s\n", name);
		else if (name == NULL)	
		    xmlGenericError(xmlGenericErrorContext,
			    "Start of element failed, was %s\n",
		            oldname);
		else	
		    xmlGenericError(xmlGenericErrorContext,
			    "Start of element %s, was %s\n",
		            name, oldname);
#endif
		if (((depth == ctxt->nameNr) &&
		     (xmlStrEqual(oldname, ctxt->name))) ||
		    (name == NULL)) {
		    if (CUR == '>')
			NEXT;
		    if (oldname != NULL)
			xmlFree(oldname);
		    break;
		}
		if (oldname != NULL)
		    xmlFree(oldname);

		/*
		 * Lookup the info for that element.
		 */
		info = htmlTagLookup(name);
		if (info == NULL) {
		    if ((ctxt->sax != NULL) && (ctxt->sax->error != NULL))
			ctxt->sax->error(ctxt->userData, "Tag %s invalid\n",
					 name);
		    ctxt->wellFormed = 0;
		} else if (info->depr) {
		    /***************************
		    if ((ctxt->sax != NULL) && (ctxt->sax->warning != NULL))
			ctxt->sax->warning(ctxt->userData,
			                   "Tag %s is deprecated\n",
					   name);
		     ***************************/
		}

		/*
		 * Check for an Empty Element labelled the XML/SGML way
		 */
		if ((CUR == '/') && (NXT(1) == '>')) {
		    SKIP(2);
		    if ((ctxt->sax != NULL) && (ctxt->sax->endElement != NULL))
			ctxt->sax->endElement(ctxt->userData, name);
		    oldname = htmlnamePop(ctxt);
#ifdef DEBUG
		    xmlGenericError(xmlGenericErrorContext,"End of tag the XML way: popping out %s\n",
		            oldname);
#endif
		    if (oldname != NULL)
			xmlFree(oldname);
		    ctxt->instate = XML_PARSER_CONTENT;
#ifdef DEBUG_PUSH
		    xmlGenericError(xmlGenericErrorContext,
			    "HPP: entering CONTENT\n");
#endif
		    break;
		}

		if (CUR == '>') {
		    NEXT;
		} else {
		    if ((ctxt->sax != NULL) && (ctxt->sax->error != NULL))
			ctxt->sax->error(ctxt->userData, 
			                 "Couldn't find end of Start Tag %s\n",
					 name);
		    ctxt->wellFormed = 0;

		    /*
		     * end of parsing of this node.
		     */
		    if (xmlStrEqual(name, ctxt->name)) { 
			nodePop(ctxt);
			oldname = htmlnamePop(ctxt);
#ifdef DEBUG
			xmlGenericError(xmlGenericErrorContext,
			 "End of start tag problem: popping out %s\n", oldname);
#endif
			if (oldname != NULL)
			    xmlFree(oldname);
		    }    

		    ctxt->instate = XML_PARSER_CONTENT;
#ifdef DEBUG_PUSH
		    xmlGenericError(xmlGenericErrorContext,
			    "HPP: entering CONTENT\n");
#endif
		    break;
		}

		/*
		 * Check for an Empty Element from DTD definition
		 */
		if ((info != NULL) && (info->empty)) {
		    if ((ctxt->sax != NULL) && (ctxt->sax->endElement != NULL))
			ctxt->sax->endElement(ctxt->userData, name);
		    oldname = htmlnamePop(ctxt);
#ifdef DEBUG
		    xmlGenericError(xmlGenericErrorContext,"End of empty tag %s : popping out %s\n", name, oldname);
#endif
		    if (oldname != NULL)
			xmlFree(oldname);
		}
		ctxt->instate = XML_PARSER_CONTENT;
#ifdef DEBUG_PUSH
		xmlGenericError(xmlGenericErrorContext,
			"HPP: entering CONTENT\n");
#endif
                break;
	    }
            case XML_PARSER_CONTENT: {
		long cons;
                /*
		 * Handle preparsed entities and charRef
		 */
		if (ctxt->token != 0) {
		    xmlChar chr[2] = { 0 , 0 } ;

		    chr[0] = (xmlChar) ctxt->token;
		    htmlCheckParagraph(ctxt);
		    if ((ctxt->sax != NULL) && (ctxt->sax->characters != NULL))
			ctxt->sax->characters(ctxt->userData, chr, 1);
		    ctxt->token = 0;
		    ctxt->checkIndex = 0;
		}
		if ((avail == 1) && (terminate)) {
		    cur = in->cur[0];
		    if ((cur != '<') && (cur != '&')) {
			if (ctxt->sax != NULL) {
			    if (IS_BLANK(cur)) {
				if (ctxt->sax->ignorableWhitespace != NULL)
				    ctxt->sax->ignorableWhitespace(
					    ctxt->userData, &cur, 1);
			    } else {
				htmlCheckParagraph(ctxt);
				if (ctxt->sax->characters != NULL)
				    ctxt->sax->characters(
					    ctxt->userData, &cur, 1);
			    }
			}
			ctxt->token = 0;
			ctxt->checkIndex = 0;
			NEXT;
			break;
		    }
		}
		if (avail < 2)
		    goto done;
		cur = in->cur[0];
		next = in->cur[1];
		cons = ctxt->nbChars;
		if ((xmlStrEqual(ctxt->name, BAD_CAST"script")) ||
		    (xmlStrEqual(ctxt->name, BAD_CAST"style"))) {
		    /*
		     * Handle SCRIPT/STYLE separately
		     */
		    if ((!terminate) &&
		        (htmlParseLookupSequence(ctxt, '<', '/', 0) < 0))
			goto done;
		    htmlParseScript(ctxt);
		    if ((cur == '<') && (next == '/')) {
			ctxt->instate = XML_PARSER_END_TAG;
			ctxt->checkIndex = 0;
#ifdef DEBUG_PUSH
			xmlGenericError(xmlGenericErrorContext,
				"HPP: entering END_TAG\n");
#endif
			break;
		    }
		} else {
		    /*
		     * Sometimes DOCTYPE arrives in the middle of the document
		     */
		    if ((cur == '<') && (next == '!') &&
			(UPP(2) == 'D') && (UPP(3) == 'O') &&
			(UPP(4) == 'C') && (UPP(5) == 'T') &&
			(UPP(6) == 'Y') && (UPP(7) == 'P') &&
			(UPP(8) == 'E')) {
			if ((!terminate) &&
			    (htmlParseLookupSequence(ctxt, '>', 0, 0) < 0))
			    goto done;
			if ((ctxt->sax != NULL) && (ctxt->sax->error != NULL))
			    ctxt->sax->error(ctxt->userData,
				 "Misplaced DOCTYPE declaration\n");
			ctxt->wellFormed = 0;
			htmlParseDocTypeDecl(ctxt);
		    } else if ((cur == '<') && (next == '!') &&
			(in->cur[2] == '-') && (in->cur[3] == '-')) {
			if ((!terminate) &&
			    (htmlParseLookupSequence(ctxt, '-', '-', '>') < 0))
			    goto done;
#ifdef DEBUG_PUSH
			xmlGenericError(xmlGenericErrorContext,
				"HPP: Parsing Comment\n");
#endif
			htmlParseComment(ctxt);
			ctxt->instate = XML_PARSER_CONTENT;
		    } else if ((cur == '<') && (next == '!') && (avail < 4)) {
			goto done;
		    } else if ((cur == '<') && (next == '/')) {
			ctxt->instate = XML_PARSER_END_TAG;
			ctxt->checkIndex = 0;
#ifdef DEBUG_PUSH
			xmlGenericError(xmlGenericErrorContext,
				"HPP: entering END_TAG\n");
#endif
			break;
		    } else if (cur == '<') {
			ctxt->instate = XML_PARSER_START_TAG;
			ctxt->checkIndex = 0;
#ifdef DEBUG_PUSH
			xmlGenericError(xmlGenericErrorContext,
				"HPP: entering START_TAG\n");
#endif
			break;
		    } else if (cur == '&') {
			if ((!terminate) &&
			    (htmlParseLookupSequence(ctxt, ';', 0, 0) < 0))
			    goto done;
#ifdef DEBUG_PUSH
			xmlGenericError(xmlGenericErrorContext,
				"HPP: Parsing Reference\n");
#endif
			/* TODO: check generation of subtrees if noent !!! */
			htmlParseReference(ctxt);
		    } else {
			/* TODO Avoid the extra copy, handle directly !!!!!! */
			/*
			 * Goal of the following test is :
			 *  - minimize calls to the SAX 'character' callback
			 *    when they are mergeable
			 */
			if ((ctxt->inputNr == 1) &&
			    (avail < HTML_PARSER_BIG_BUFFER_SIZE)) {
			    if ((!terminate) &&
				(htmlParseLookupSequence(ctxt, '<', 0, 0) < 0))
				goto done;
			}
			ctxt->checkIndex = 0;
#ifdef DEBUG_PUSH
			xmlGenericError(xmlGenericErrorContext,
				"HPP: Parsing char data\n");
#endif
			htmlParseCharData(ctxt);
		    }
		}
		if (cons == ctxt->nbChars) {
		    if (ctxt->node != NULL) {
			if ((ctxt->sax != NULL) && (ctxt->sax->error != NULL))
			    ctxt->sax->error(ctxt->userData,
				 "detected an error in element content\n");
			ctxt->wellFormed = 0;
		    }
		    NEXT;
		    break;
		}

		break;
	    }
            case XML_PARSER_END_TAG:
		if (avail < 2)
		    goto done;
		if ((!terminate) &&
		    (htmlParseLookupSequence(ctxt, '>', 0, 0) < 0))
		    goto done;
		htmlParseEndTag(ctxt);
		if (ctxt->nameNr == 0) {
		    ctxt->instate = XML_PARSER_EPILOG;
		} else {
		    ctxt->instate = XML_PARSER_CONTENT;
		}
		ctxt->checkIndex = 0;
#ifdef DEBUG_PUSH
		xmlGenericError(xmlGenericErrorContext,
			"HPP: entering CONTENT\n");
#endif
	        break;
            case XML_PARSER_CDATA_SECTION:
		xmlGenericError(xmlGenericErrorContext,
			"HPP: internal error, state == CDATA\n");
		ctxt->instate = XML_PARSER_CONTENT;
		ctxt->checkIndex = 0;
#ifdef DEBUG_PUSH
		xmlGenericError(xmlGenericErrorContext,
			"HPP: entering CONTENT\n");
#endif
		break;
            case XML_PARSER_DTD:
		xmlGenericError(xmlGenericErrorContext,
			"HPP: internal error, state == DTD\n");
		ctxt->instate = XML_PARSER_CONTENT;
		ctxt->checkIndex = 0;
#ifdef DEBUG_PUSH
		xmlGenericError(xmlGenericErrorContext,
			"HPP: entering CONTENT\n");
#endif
		break;
            case XML_PARSER_COMMENT:
		xmlGenericError(xmlGenericErrorContext,
			"HPP: internal error, state == COMMENT\n");
		ctxt->instate = XML_PARSER_CONTENT;
		ctxt->checkIndex = 0;
#ifdef DEBUG_PUSH
		xmlGenericError(xmlGenericErrorContext,
			"HPP: entering CONTENT\n");
#endif
		break;
            case XML_PARSER_PI:
		xmlGenericError(xmlGenericErrorContext,
			"HPP: internal error, state == PI\n");
		ctxt->instate = XML_PARSER_CONTENT;
		ctxt->checkIndex = 0;
#ifdef DEBUG_PUSH
		xmlGenericError(xmlGenericErrorContext,
			"HPP: entering CONTENT\n");
#endif
		break;
            case XML_PARSER_ENTITY_DECL:
		xmlGenericError(xmlGenericErrorContext,
			"HPP: internal error, state == ENTITY_DECL\n");
		ctxt->instate = XML_PARSER_CONTENT;
		ctxt->checkIndex = 0;
#ifdef DEBUG_PUSH
		xmlGenericError(xmlGenericErrorContext,
			"HPP: entering CONTENT\n");
#endif
		break;
            case XML_PARSER_ENTITY_VALUE:
		xmlGenericError(xmlGenericErrorContext,
			"HPP: internal error, state == ENTITY_VALUE\n");
		ctxt->instate = XML_PARSER_CONTENT;
		ctxt->checkIndex = 0;
#ifdef DEBUG_PUSH
		xmlGenericError(xmlGenericErrorContext,
			"HPP: entering DTD\n");
#endif
		break;
            case XML_PARSER_ATTRIBUTE_VALUE:
		xmlGenericError(xmlGenericErrorContext,
			"HPP: internal error, state == ATTRIBUTE_VALUE\n");
		ctxt->instate = XML_PARSER_START_TAG;
		ctxt->checkIndex = 0;
#ifdef DEBUG_PUSH
		xmlGenericError(xmlGenericErrorContext,
			"HPP: entering START_TAG\n");
#endif
		break;
	    case XML_PARSER_SYSTEM_LITERAL:
		xmlGenericError(xmlGenericErrorContext,
			"HPP: internal error, state == XML_PARSER_SYSTEM_LITERAL\n");
		ctxt->instate = XML_PARSER_CONTENT;
		ctxt->checkIndex = 0;
#ifdef DEBUG_PUSH
		xmlGenericError(xmlGenericErrorContext,
			"HPP: entering CONTENT\n");
#endif
		break;
	    case XML_PARSER_IGNORE:
		xmlGenericError(xmlGenericErrorContext,
			"HPP: internal error, state == XML_PARSER_IGNORE\n");
		ctxt->instate = XML_PARSER_CONTENT;
		ctxt->checkIndex = 0;
#ifdef DEBUG_PUSH
		xmlGenericError(xmlGenericErrorContext,
			"HPP: entering CONTENT\n");
#endif
		break;
	}
    }
done:    
    if ((avail == 0) && (terminate)) {
	htmlAutoCloseOnEnd(ctxt);
	if ((ctxt->nameNr == 0) && (ctxt->instate != XML_PARSER_EOF)) { 
	    /*
	     * SAX: end of the document processing.
	     */
	    ctxt->instate = XML_PARSER_EOF;
	    if ((ctxt->sax) && (ctxt->sax->endDocument != NULL))
		ctxt->sax->endDocument(ctxt->userData);
	}
    }
    if ((ctxt->myDoc != NULL) &&
	((terminate) || (ctxt->instate == XML_PARSER_EOF) ||
	 (ctxt->instate == XML_PARSER_EPILOG))) {
	xmlDtdPtr dtd;
	dtd = xmlGetIntSubset(ctxt->myDoc);
	if (dtd == NULL)
	    ctxt->myDoc->intSubset = 
		xmlCreateIntSubset(ctxt->myDoc, BAD_CAST "HTML", 
		    BAD_CAST "-//W3C//DTD HTML 4.0 Transitional//EN",
		    BAD_CAST "http://www.w3.org/TR/REC-html40/loose.dtd");
    }
#ifdef DEBUG_PUSH
    xmlGenericError(xmlGenericErrorContext, "HPP: done %d\n", ret);
#endif
    return(ret);
}

/**
 * htmlParseChunk:
 * @ctxt:  an XML parser context
 * @chunk:  an char array
 * @size:  the size in byte of the chunk
 * @terminate:  last chunk indicator
 *
 * Parse a Chunk of memory
 *
 * Returns zero if no error, the xmlParserErrors otherwise.
 */
int
htmlParseChunk(htmlParserCtxtPtr ctxt, const char *chunk, int size,
              int terminate) {
    if ((size > 0) && (chunk != NULL) && (ctxt->input != NULL) &&
        (ctxt->input->buf != NULL) && (ctxt->instate != XML_PARSER_EOF))  {
	int base = ctxt->input->base - ctxt->input->buf->buffer->content;
	int cur = ctxt->input->cur - ctxt->input->base;
	
	xmlParserInputBufferPush(ctxt->input->buf, size, chunk);	      
	ctxt->input->base = ctxt->input->buf->buffer->content + base;
	ctxt->input->cur = ctxt->input->base + cur;
#ifdef DEBUG_PUSH
	xmlGenericError(xmlGenericErrorContext, "HPP: pushed %d\n", size);
#endif

	if ((terminate) || (ctxt->input->buf->buffer->use > 80))
	    htmlParseTryOrFinish(ctxt, terminate);
    } else if (ctxt->instate != XML_PARSER_EOF) {
	xmlParserInputBufferPush(ctxt->input->buf, 0, "");
        htmlParseTryOrFinish(ctxt, terminate);
    }
    if (terminate) {
	if ((ctxt->instate != XML_PARSER_EOF) &&
	    (ctxt->instate != XML_PARSER_EPILOG) &&
	    (ctxt->instate != XML_PARSER_MISC)) {
	    ctxt->errNo = XML_ERR_DOCUMENT_END;
	    ctxt->wellFormed = 0;
	} 
	if (ctxt->instate != XML_PARSER_EOF) {
	    if ((ctxt->sax) && (ctxt->sax->endDocument != NULL))
		ctxt->sax->endDocument(ctxt->userData);
	}
	ctxt->instate = XML_PARSER_EOF;
    }
    return((xmlParserErrors) ctxt->errNo);	      
}

/************************************************************************
 *									*
 *			User entry points				*
 *									*
 ************************************************************************/

/**
 * htmlCreatePushParserCtxt :
 * @sax:  a SAX handler
 * @user_data:  The user data returned on SAX callbacks
 * @chunk:  a pointer to an array of chars
 * @size:  number of chars in the array
 * @filename:  an optional file name or URI
 * @enc:  an optional encoding
 *
 * Create a parser context for using the HTML parser in push mode
 * To allow content encoding detection, @size should be >= 4
 * The value of @filename is used for fetching external entities
 * and error/warning reports.
 *
 * Returns the new parser context or NULL
 */
htmlParserCtxtPtr
htmlCreatePushParserCtxt(htmlSAXHandlerPtr sax, void *user_data, 
                         const char *chunk, int size, const char *filename,
			 xmlCharEncoding enc) {
    htmlParserCtxtPtr ctxt;
    htmlParserInputPtr inputStream;
    xmlParserInputBufferPtr buf;

    xmlInitParser();

    buf = xmlAllocParserInputBuffer(enc);
    if (buf == NULL) return(NULL);

    ctxt = (htmlParserCtxtPtr) xmlMalloc(sizeof(htmlParserCtxt));
    if (ctxt == NULL) {
	xmlFree(buf);
	return(NULL);
    }
    memset(ctxt, 0, sizeof(htmlParserCtxt));
    htmlInitParserCtxt(ctxt);
    if (sax != NULL) {
	if (ctxt->sax != &htmlDefaultSAXHandler)
	    xmlFree(ctxt->sax);
	ctxt->sax = (htmlSAXHandlerPtr) xmlMalloc(sizeof(htmlSAXHandler));
	if (ctxt->sax == NULL) {
	    xmlFree(buf);
	    xmlFree(ctxt);
	    return(NULL);
	}
	memcpy(ctxt->sax, sax, sizeof(htmlSAXHandler));
	if (user_data != NULL)
	    ctxt->userData = user_data;
    }	
    if (filename == NULL) {
	ctxt->directory = NULL;
    } else {
        ctxt->directory = xmlParserGetDirectory(filename);
    }

    inputStream = htmlNewInputStream(ctxt);
    if (inputStream == NULL) {
	xmlFreeParserCtxt(ctxt);
	return(NULL);
    }

    if (filename == NULL)
	inputStream->filename = NULL;
    else
	inputStream->filename = xmlMemStrdup(filename);
    inputStream->buf = buf;
    inputStream->base = inputStream->buf->buffer->content;
    inputStream->cur = inputStream->buf->buffer->content;

    inputPush(ctxt, inputStream);

    if ((size > 0) && (chunk != NULL) && (ctxt->input != NULL) &&
        (ctxt->input->buf != NULL))  {	      
	xmlParserInputBufferPush(ctxt->input->buf, size, chunk);	      
#ifdef DEBUG_PUSH
	xmlGenericError(xmlGenericErrorContext, "HPP: pushed %d\n", size);
#endif
    }

    return(ctxt);
}

/**
 * htmlSAXParseDoc :
 * @cur:  a pointer to an array of xmlChar
 * @encoding:  a free form C string describing the HTML document encoding, or NULL
 * @sax:  the SAX handler block
 * @userData: if using SAX, this pointer will be provided on callbacks. 
 *
 * Parse an HTML in-memory document. If sax is not NULL, use the SAX callbacks
 * to handle parse events. If sax is NULL, fallback to the default DOM
 * behavior and return a tree.
 * 
 * Returns the resulting document tree unless SAX is NULL or the document is
 *     not well formed.
 */

htmlDocPtr
htmlSAXParseDoc(xmlChar *cur, const char *encoding, htmlSAXHandlerPtr sax, void *userData) {
    htmlDocPtr ret;
    htmlParserCtxtPtr ctxt;

    xmlInitParser();

    if (cur == NULL) return(NULL);


    ctxt = htmlCreateDocParserCtxt(cur, encoding);
    if (ctxt == NULL) return(NULL);
    if (sax != NULL) { 
        ctxt->sax = sax;
        ctxt->userData = userData;
    }

    htmlParseDocument(ctxt);
    ret = ctxt->myDoc;
    if (sax != NULL) {
	ctxt->sax = NULL;
	ctxt->userData = NULL;
    }
    htmlFreeParserCtxt(ctxt);
    
    return(ret);
}

/**
 * htmlParseDoc :
 * @cur:  a pointer to an array of xmlChar
 * @encoding:  a free form C string describing the HTML document encoding, or NULL
 *
 * parse an HTML in-memory document and build a tree.
 * 
 * Returns the resulting document tree
 */

htmlDocPtr
htmlParseDoc(xmlChar *cur, const char *encoding) {
    return(htmlSAXParseDoc(cur, encoding, NULL, NULL));
}


/**
 * htmlCreateFileParserCtxt :
 * @filename:  the filename
 * @encoding:  a free form C string describing the HTML document encoding, or NULL
 *
 * Create a parser context for a file content. 
 * Automatic support for ZLIB/Compress compressed document is provided
 * by default if found at compile-time.
 *
 * Returns the new parser context or NULL
 */
htmlParserCtxtPtr
htmlCreateFileParserCtxt(const char *filename, const char *encoding)
{
    htmlParserCtxtPtr ctxt;
    htmlParserInputPtr inputStream;
    xmlParserInputBufferPtr buf;
    /* htmlCharEncoding enc; */
    xmlChar *content, *content_line = (xmlChar *) "charset=";

    buf = xmlParserInputBufferCreateFilename(filename, XML_CHAR_ENCODING_NONE);
    if (buf == NULL) return(NULL);

    ctxt = (htmlParserCtxtPtr) xmlMalloc(sizeof(htmlParserCtxt));
    if (ctxt == NULL) {
        perror("malloc");
	return(NULL);
    }
    memset(ctxt, 0, sizeof(htmlParserCtxt));
    htmlInitParserCtxt(ctxt);
    inputStream = (htmlParserInputPtr) xmlMalloc(sizeof(htmlParserInput));
    if (inputStream == NULL) {
        perror("malloc");
	xmlFree(ctxt);
	return(NULL);
    }
    memset(inputStream, 0, sizeof(htmlParserInput));

    inputStream->filename = xmlMemStrdup(filename);
    inputStream->line = 1;
    inputStream->col = 1;
    inputStream->buf = buf;
    inputStream->directory = NULL;

    inputStream->base = inputStream->buf->buffer->content;
    inputStream->cur = inputStream->buf->buffer->content;
    inputStream->free = NULL;

    inputPush(ctxt, inputStream);
    
    /* set encoding */
    if (encoding) {
        content = xmlMalloc (xmlStrlen(content_line) + strlen(encoding) + 1);
	if (content) {  
	    strcpy ((char *)content, (char *)content_line);
            strcat ((char *)content, (char *)encoding);
            htmlCheckEncoding (ctxt, content);
	    xmlFree (content);
	}
    }
    
    return(ctxt);
}

/**
 * htmlSAXParseFile :
 * @filename:  the filename
 * @encoding:  a free form C string describing the HTML document encoding, or NULL
 * @sax:  the SAX handler block
 * @userData: if using SAX, this pointer will be provided on callbacks. 
 *
 * parse an HTML file and build a tree. Automatic support for ZLIB/Compress
 * compressed document is provided by default if found at compile-time.
 * It use the given SAX function block to handle the parsing callback.
 * If sax is NULL, fallback to the default DOM tree building routines.
 *
 * Returns the resulting document tree unless SAX is NULL or the document is
 *     not well formed.
 */

htmlDocPtr
htmlSAXParseFile(const char *filename, const char *encoding, htmlSAXHandlerPtr sax, 
                 void *userData) {
    htmlDocPtr ret;
    htmlParserCtxtPtr ctxt;
    htmlSAXHandlerPtr oldsax = NULL;

    xmlInitParser();

    ctxt = htmlCreateFileParserCtxt(filename, encoding);
    if (ctxt == NULL) return(NULL);
    if (sax != NULL) {
	oldsax = ctxt->sax;
        ctxt->sax = sax;
        ctxt->userData = userData;
    }

    htmlParseDocument(ctxt);

    ret = ctxt->myDoc;
    if (sax != NULL) {
        ctxt->sax = oldsax;
        ctxt->userData = NULL;
    }
    htmlFreeParserCtxt(ctxt);
    
    return(ret);
}

/**
 * htmlParseFile :
 * @filename:  the filename
 * @encoding:  a free form C string describing the HTML document encoding, or NULL
 *
 * parse an HTML file and build a tree. Automatic support for ZLIB/Compress
 * compressed document is provided by default if found at compile-time.
 *
 * Returns the resulting document tree
 */

htmlDocPtr
htmlParseFile(const char *filename, const char *encoding) {
    return(htmlSAXParseFile(filename, encoding, NULL, NULL));
}

/**
 * htmlHandleOmittedElem:
 * @val:  int 0 or 1 
 *
 * Set and return the previous value for handling HTML omitted tags.
 *
 * Returns the last value for 0 for no handling, 1 for auto insertion.
 */

int
htmlHandleOmittedElem(int val) {
    int old = htmlOmittedDefaultValue;

    htmlOmittedDefaultValue = val;
    return(old);
}

#endif /* LIBXML_HTML_ENABLED */
