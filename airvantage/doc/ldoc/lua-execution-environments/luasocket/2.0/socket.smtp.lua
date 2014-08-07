--[[----------------------------------------------------------------------------
 The _smtp_ namespace provides functionality to send e-mail messages.

 The high-level API consists of two functions: one to define an e-mail message,
 and another to actually send the message. Although almost all users will find
 that these functions provide more than enough functionality, the underlying
 implementation allows for even more control (if you bother to read the code).

 The implementation conforms to the Simple Mail Transfer Protocol,
 [RFC 2821](http://www.ietf.org/rfc/rfc2821.txt). Another RFC of interest is
 [RFC 2822](http://www.ietf.org/rfc/rfc2822.txt), which governs the Internet
 Message Format. Multipart messages (those that contain attachments) are part
 of the MIME standard, but described mainly in
 [RFC 2046](http://www.ietf.org/rfc/rfc2046.txt).

 In the description below, good understanding of
 [LTN012, Filters sources and sinks](http://lua-users.org/wiki/FiltersSourcesAndSinks)
 and the [MIME](http://w3.impa.br/~diego/software/luasocket/mime.html)
 module is assumed. In fact, the SMTP module was the main reason for their
 creation.

 To obtain the _smtp_ namespace, run:

	-- loads the SMTP module and everything it requires
	local smtp = require("socket.smtp")

 MIME headers are represented as a Lua table in the form:

	headers = {
	  field-1-name = field-1-value,
	  field-2-name = field-2-value,
	  field-3-name = field-3-value,
	  ...
	  field-n-name = field-n-value
	}

 Field names are case insensitive (as specified by the standard) and all
 functions work with lowercase field names. Field values are left unmodified.

 Note: MIME headers are independent of order. Therefore, there is no problem in
 representing them in a Lua table.

 The following constants can be set to control the default behavior of the SMTP
 module:

- _DOMAIN_: domain used to greet the server;
- _PORT_: default port used for the connection;
- _SERVER_: default server used for the connection;
- _TIMEOUT_: default timeout for all I/O operations;
- _ZONE_: default time zone.

 @module socket.smtp

]]

--[[0
]]

--[[----------------------------------------------------------------------------
 Sends a message to a recipient list.

 Since sending messages is not as simple as downloading an URL from a FTP or
 HTTP server, this function doesn't have a simple interface. However, see the
 @{#(socket.smtp).message} source factory for a very powerful way to define the message
 contents.
	smtp.send{
	  from = string,
	  rcpt = string or string-table,
	  source = LTN12 source,
	  [user = string,]
	  [password = string,]
	  [server = string,]
	  [port = number,]
	  [domain = string,]
	  [step = LTN12 pump step,]
	  [create = function]
	}

 Note: SMTP servers can be very picky with the format of e-mail addresses. To be
 safe, use only addresses of the form _"<fulano@example.com>"_ in the from and
 _rcpt_ arguments to the _send_ function. In headers, e-mail addresses can take
 whatever form you like.

 Big note: There is a good deal of misconception with the use of the destination
 address field headers, i.e., the _'To'_, _'Cc'_, and, more importantly, the
 _'Bcc'_ headers. Do **not** add a _'Bcc'_ header to your messages because it
 will probably do the exact opposite of what you expect.

 Only recipients specified in the rcpt list will receive a copy of the message.
 Each recipient of an SMTP mail message receives a copy of the message body
 along with the headers, and nothing more. The headers are part of the message
 and should be produced by the
 [LTN12](http://lua-users.org/wiki/FiltersSourcesAndSinks) _source_ function.
 The _rcpt_ list is **not** part of the message and will not be sent to anyone.

 [RFC 2822](http://www.ietf.org/rfc/rfc2822.txt) has two important and short
 sections, "3.6.3. Destination address fields" and "5. Security considerations",
 explaining the proper use of these headers. Here is a summary of what it says:

 - _To_: contains the address(es) of the primary recipient(s) of the message;
 - _Cc_: (where the "Cc" means "Carbon Copy" in the sense of making a copy on a
  typewriter using carbon paper) contains the addresses of others who are to
  receive the message, though the content of the message may not be directed at
  them;
 - _Bcc_: (where the "Bcc" means "Blind Carbon Copy") contains addresses of
  recipients of the message whose addresses are not to be revealed to other
  recipients of the message. 

 The LuaSocket @{socket#socket.send} function does not care or interpret the
 headers you send, but it gives you full control over what is sent and to whom
 it is sent:

 - If someone is to receive the message, the e-mail address has to be in the
  recipient list. This is the only parameter that controls who gets a copy of
  the message;
 - If there are multiple recipients, none of them will automatically know that
  someone else got that message. That is, the default behavior is similar to the
  Bcc field of popular e-mail clients;
 - It is up to you to add the To header with the list of primary recipients so
  that other recipients can see it;
 - It is also up to you to add the Cc header with the list of additional
  recipients so that everyone else sees it;
 - Adding a header Bcc is nonsense, unless it is empty. Otherwise, everyone
  receiving the message will see it and that is exactly what you don't want to
  happen! 

 I hope this clarifies the issue. Otherwise, please refer to
 [RFC 2821](http://www.ietf.org/rfc/rfc2821.txt) and
 [RFC 2822](http://www.ietf.org/rfc/rfc2822.txt).

 @function [parent=#socket.smtp] send
 @param #table arguments The sender is given by the e-mail address in the
  _from_ field. _Rcpt_ is a Lua table with one entry for each recipient e-mail
  address, or a string in case there is just one recipient. The contents of the
  message are given by a simple
  [LTN12](http://lua-users.org/wiki/FiltersSourcesAndSinks) _source_.
  Several arguments are optional:

 - _user, password_: User and password for authentication. The function will
  attempt LOGIN and PLAIN authentication methods if supported by the server
  (both are unsafe);
 - _server_: Server to connect to. Defaults to "localhost";
 - _port_: Port to connect to. Defaults to 25;
 - _domain_: Domain name used to greet the server; Defaults to the local machine
  host name;
 - _step_: [LTN12](http://lua-users.org/wiki/FiltersSourcesAndSinks) _pump_ step
  function used to pass data from the source to the server. Defaults to the
  LTN12 _pump.step_ function;
 - _create_: An optional function to be used instead of socket.tcp when the
  communications socket is created.
 @return #number If successful, the function returns 1.
 @return #nil, #string Otherwise, the function returns nil followed by an error
  message.
 @usage
-- load the smtp support
local smtp = require("socket.smtp")

-- Connects to server "localhost" and sends a message to users
-- "fulano@example.com",  "beltrano@example.com", 
-- and "sicrano@example.com".
-- Note that "fulano" is the primary recipient, "beltrano" receives a
-- carbon copy and neither of them knows that "sicrano" received a blind
-- carbon copy of the message.
from = "<luasocket@example.com>"

rcpt = {
  "<fulano@example.com>",
  "<beltrano@example.com>",
  "<sicrano@example.com>"
}

mesgt = {
  headers = {
    to = "Fulano da Silva <fulano@example.com>",
    cc = '"Beltrano F. Nunes" <beltrano@example.com>',
    subject = "My first message"
  },
  body = "I hope this works. If it does, I can send you another 1000 copies."
}

r, e = smtp.send{
  from = from,
  rcpt = rcpt, 
  source = smtp.message(mesgt)
}

]]

--[[----------------------------------------------------------------------------
 Provides a simple [LTN12](http://lua-users.org/wiki/FiltersSourcesAndSinks) source that sends an SMTP message body, possibly multipart (arbitrarily deep).

 @function [parent=#socket.smtp] message
 @param #table mesgt The only parameter of the function is describing the
  message. Mesgt has the following form (notice the recursive structure):
	mesgt = {
	  headers = header-table,
	  body = LTN12 source or string or multipart-mesgt
	}
	 
	multipart-mesgt = {
	  [preamble = string,]
	  [1] = mesgt,
	  [2] = mesgt,
	  ...
	  [n] = mesgt,
	  [epilogue = string,]
	}
 For a simple message, all that is needed is a set of _headers_ and the _body_.
 The message _body_ can be given as a string or as a _simple_
 [LTN12](http://lua-users.org/wiki/FiltersSourcesAndSinks) source. For multipart
 messages, the body is a table that recursively defines each part as an
 independent message, plus an optional _preamble_ and _epilogue_.
 @returns A simple [LTN12](http://lua-users.org/wiki/FiltersSourcesAndSinks)
  source that produces the message contents as defined by _mesgt_, chunk by
  chunk. Hopefully, the following example will make things clear. When in doubt,
  refer to the appropriate RFC as listed in the introduction.
 @usage
-- load the smtp support and its friends
local smtp = require("socket.smtp")
local mime = require("mime")
local ltn12 = require("ltn12")

-- creates a source to send a message with two parts. The first part is 
-- plain text, the second part is a PNG image, encoded as base64.
source = smtp.message{
  headers = {
     -- Remember that headers are *ignored* by smtp.send. 
     from = "Sicrano de Oliveira <sicrano@example.com>",
     to = "Fulano da Silva <fulano@example.com>",
     subject = "Here is a message with attachments"
  },
  body = {
    preamble = "If your client doesn't understand attachments, \r\n" ..
               "it will still display the preamble and the epilogue.\r\n" ..
               "Preamble will probably appear even in a MIME enabled client.",
    -- first part: no headers means plain text, us-ascii.
    -- The mime.eol low-level filter normalizes end-of-line markers.
    [1] = { 
      body = mime.eol(0, [=[
        Lines in a message body should always end with CRLF. 
        The smtp module will *NOT* perform translation. However, the 
        send function *DOES* perform SMTP stuffing, whereas the message
        function does *NOT*.
      ]=])
    },
    -- second part: headers describe content to be a png image, 
    -- sent under the base64 transfer content encoding.
    -- notice that nothing happens until the message is actually sent. 
    -- small chunks are loaded into memory right before transmission and 
    -- translation happens on the fly.
    [2] = { 
      headers = {
        ["content-type"] = 'image/png; name="image.png"',
        ["content-disposition"] = 'attachment; filename="image.png"',
        ["content-description"] = 'a beautiful image',
        ["content-transfer-encoding"] = "BASE64"
      },
      body = ltn12.source.chain(
        ltn12.source.file(io.open("image.png", "rb")),
        ltn12.filter.chain(
          mime.encode("base64"),
          mime.wrap()
        )
      )
    },
    epilogue = "This might also show up, but after the attachments"
  }
}

-- finally send it
r, e = smtp.send{
    from = "<sicrano@example.com>",
    rcpt = "<fulano@example.com>",
    source = source,
}

]]

return nil
