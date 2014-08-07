--[[----------------------------------------------------------------------------
 The _mime_ namespace offers filters that apply and remove common content transfer encodings, such as Base64 and Quoted-Printable.

 It also provides functions to break text into lines and change the end-of-line
 convention. MIME is described mainly in RFC 2045, 2046, 2047, 2048, and 2049.

 All functionality provided by the MIME module follows the ideas presented in
 LTN012, Filters sources and sinks.

To obtain the mime namespace, run:

	-- loads the MIME module and everything it requires
	local mime = require("mime")

 @module mime
]]

--[[----------------------------------------------------------------------------
 Converts most common end-of-line markers to a specific given marker.

 Note: There is no perfect solution to this problem. Different end-of-line
 markers are an evil that will probably plague developers forever. This function
 , however, will work perfectly for text created with any of the most common
 end-of-line markers, i.e. the Mac OS (CR), the Unix (LF), or the DOS (CRLF)
 conventions. Even if the data has mixed end-of-line markers, the function will
 still work well, although it doesn't guarantee that the number of empty lines
 will be correct.

 @function [parent=#mime] normalize
 @param marker The new marker. It defaults to CRLF, the canonic end-of-line
  marker defined by the MIME standard.
 @return The function returns a filter that performs the conversion. 
]]

--[[----------------------------------------------------------------------------
 Returns a filter that decodes data from a given transfer content encoding.
 
 @function [parent=#mime] decode
 @param #string style Can be **"base64"** or **"quoted-printable"**.
]]

--[[----------------------------------------------------------------------------
 Although both transfer content encodings specify a limit for the line length,
 the encoding filters do not break text into lines (for added flexibility).
 Below is a filter that converts binary data to the Base64 transfer content
 encoding and breaks it into lines of the correct size.

	base64 = ltn12.filter.chain(
	  mime.encode("base64"),
	  mime.wrap("base64")
	)
 Note: Text data _has_ to be converted to canonic form before being encoded.

	base64 = ltn12.filter.chain(
	  mime.normalize(),
	  mime.encode("base64"),
	  mime.wrap("base64")
	)
 @function [parent=#mime] encode
 @param #string style Can be **"base64"** or **"quoted-printable"**.
 @param #string mode (optional) In the Quoted-Printable case, the user can
  specify whether the data is textual or binary, by passing the mode strings
  _"text"_ or _"binary"_. Mode defaults to _"text"_. 
]]

--[[----------------------------------------------------------------------------
 Creates and returns a filter that performs stuffing of SMTP messages.

 Note: The @{#smtp.send} function uses this filter automatically. You don't need
 to chain it with your source, or apply it to your message body. 
 @function [parent=#mime] stuff
]]

--[[----------------------------------------------------------------------------
 Returns a filter that breaks data into lines.
 
 For example, to create an encoding filter for the Quoted-Printable transfer
 content encoding of text data, do the following:

	qp = ltn12.filter.chain(
	  mime.normalize(),
	  mime.encode("quoted-printable"),
	  mime.wrap("quoted-printable")
	)

 Note: To break into lines with a different end-of-line convention, apply a
 normalization filter after the line break filter.
 
 @function [parent=#mime] wrap
 @param #string style The _"text"_ line-wrap filter simply breaks text into
  lines by inserting CRLF end-of-line markers at appropriate positions. The
  _"base64"_ line-wrap filter works just like the default _"text"_ line-wrap
  filter with default length. The function can also wrap "quoted-printable"
  lines, taking care not to break lines in the middle of an escaped character.
  In that case, the line length is fixed at 76. 
 @param #number length (optional) Available when _style_ is _"text"_, defaults
  76. 
]]

--[[----------------------------------------------------------------------------
 Low-level filter to perform Base64 encoding.

 Note: The simplest use of this function is to encode a string into it's Base64
 transfer content encoding. Notice the extra parenthesis around the call to
 _mime.b64_, to discard the second return value. 
	print((mime.b64("diego:password")))
	--> ZGllZ286cGFzc3dvcmQ=

 @function [parent=#mime] b64
 @param #string C
 @param D (optional)
 @return With
	A, B = mime.b64(C [, D])
  _A_ is the encoded version of the largest prefix of _C..D_ that can be encoded
  unambiguously. B has the remaining bytes of _C..D_, before encoding. If _D_ is
  _#nil_, _A_ is padded with the encoding of the remaining bytes of _C_. 
]]

--[[----------------------------------------------------------------------------
 Low-level filter to perform SMTP stuffing and enable transmission of messages containing the sequence "CRLF.CRLF". 

 Note: The message body is defined to begin with an implicit CRLF. Therefore, to
 stuff a message correctly, the first _m_ should have the value 2.
	print((string.gsub(mime.dot(2, ".\r\nStuffing the message.\r\n.\r\n."), "\r\n", "\\n")))
	--> ..\nStuffing the message.\n..\n..

 Note: The @{smtp#smtp.send} function uses this filter automatically. You don't
 need to apply it again.
 
 @function [parent=#mime] dot
 @param #number m
 @param B (optional)
 @return With
	A, n = mime.dot(m [, B])
  _A_ is the stuffed version of _B_. _'n'_ gives the number of characters from
  the sequence CRLF seen in the end of _B_. _'m'_ should tell the same, but for
  the previous chunk. 
]]

--[[----------------------------------------------------------------------------
 Low-level filter to perform end-of-line marker translation.

 For each chunk, the function needs to know if the last character of the
 previous chunk could be part of an end-of-line marker or not. This is the
 context the function receives besides the chunk. An updated version of the
 context is returned after each new chunk.

 @function [parent=#mime] eol
 @param #number C The ASCII value of the last character of the previous chunk,
  if it was a candidate for line break, or 0 otherwise. 
 @param D (optional)
 @param #string marker (optional) Gives the new end-of-line marker and defaults
  to CRLF. 
 @return With
	A, B = mime.eol(C [, D, marker])
  _A_ is the translated version of _D_. _C_ is the ASCII value of the last
  character of the previous chunk, if it was a candidate for line break, or 0
  otherwise. _B_ is the same as _C_, but for the current chunk. _Marker_ gives
  the new end-of-line marker and defaults to CRLF.
 @usage
-- translates the end-of-line marker to UNIX
unix = mime.eol(0, dos, "\n") 
]]

--[[----------------------------------------------------------------------------
 Low-level filter to perform Quoted-Printable encoding.
 
 Note: The simplest use of this function is to encode a string into it's
 Quoted-Printable transfer content encoding. Notice the extra parenthesis around
 the call to @{#mime.qp}, to discard the second return value.
	print((mime.qp("maçã")))
	--> ma=E7=E3=

 @function [parent=#mime] qp
 @param #string C
 @param #string D (optional)
 @param #string marker
 @return With
	A, B = mime.qp(C [, D, marker])
 _A_ is the encoded version of the largest prefix of _C..D_ that can be encoded
 unambiguously. _B_ has the remaining bytes of _C..D_, before encoding. If _D_
 is _#nil_, _A_ is padded with the encoding of the remaining bytes of _C_.
 Throughout encoding, occurrences of CRLF are replaced by the _marker_, which
 itself defaults to CRLF.
]]

--[[----------------------------------------------------------------------------
 Low-level filter to break Quoted-Printable text into lines.

 Note: Besides breaking text into lines, this function makes sure the line
 breaks don't fall in the middle of an escaped character combination. Also, this
 function only breaks lines that are bigger than _length_ bytes.
 
 @function [parent=#mime] qpwrp
 @param #number n
 @param #string B (optional)
 @param #number length (optional)
 @return With
	A, m = mime.qpwrp(n [, B, length])
 _A_ is the decoded version of the largest prefix of _C..D_ that can be decoded
 unambiguously. _B_ has the remaining bytes of C..D, before decoding. If _D_ is
 _#nil_, A is the empty string and B returns whatever couldn't be decoded. 
]]

--[[----------------------------------------------------------------------------
 Low-level filter to perform Base64 decoding.
 
 Note: The simplest use of this function is to decode a string from it's Base64
 transfer content encoding. Notice the extra parenthesis around the call to
 @{#mime.unqp}, to discard the second return value.
	print((mime.unb64("ZGllZ286cGFzc3dvcmQ=")))
	--> diego:password

 @function [parent=#mime] unb64
 @param #string C
 @param #string D (optional)
 @return With
	A, B = mime.unb64(C [, D])
  _A_ is the decoded version of the largest prefix of _C..D_ that can be decoded
  unambiguously. B has the remaining bytes of _C..D_, before decoding. If _D_ is
  _#nil_, _A_ is the empty string and _B_ returns whatever couldn't be decoded. 
]]

--[[----------------------------------------------------------------------------
 Low-level filter to remove the Quoted-Printable transfer content encoding from data.

 Note: The simplest use of this function is to decode a string from it's
 Quoted-Printable transfer content encoding. Notice the extra parenthesis around
 the call to @#mime.unqp}, to discard the second return value.
	print((mime.qp("ma=E7=E3=")))
	--> maçã

 @function [parent=#mime] unqp
 @param C
 @param D (optional)
 @return With
	print((mime.qp("ma=E7=E3=")))
	--> maçã
  _A_ is the decoded version of the largest prefix of _C..D_ that can be decoded
  unambiguously. _B_ has the remaining bytes of _C..D_, before decoding. If _D_
  is _#nil_, _A_ is augmented with the encoding of the remaining bytes of _C_. 
]]

--[[----------------------------------------------------------------------------
 Low-level filter to break text into lines with CRLF marker.

 Text is assumed to be in the @{#mime.normalize} form.
 
 Note: This function only breaks lines that are bigger than _length_ bytes.
 The resulting line length does not include the CRLF marker.
 
 @function [parent=#mime] unqp
 @param #number n
 @param B (optional)
 @param #number length (optional)
 @return With
	A, m = mime.wrp(n [, B, length])
 _A_ is a copy of _B_, broken into lines of at most length bytes (defaults to
 76). _'n'_ should tell how many bytes are left for the first line of _B_ and
 _'m'_ returns the number of bytes left in the last line of _A_. 
]]

return nil
