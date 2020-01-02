{#-
 #  Jinja2 template for generating C client prototypes for Legato APIs.
 #
 #  Copyright (C) Sierra Wireless Inc.
 #}
{% extends "TEMPLATE_header.h" %}
{%- block InterfaceHeader %}{%- if imports %}

// Interface specific includes
{%- for import in imports %}
#include "{{import}}_interface.h"
{%- endfor %}
{%- endif %}

{%- endblock %}
{% block HeaderComments %}
{% for comment in fileComments -%}
{{comment|FormatHeaderComment}}
{% endfor %}
{%- endblock %}
{% block GenericFunctions -%}
//--------------------------------------------------------------------------------------------------
/**
 * Type for handler called when a server disconnects.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*{{apiName}}_DisconnectHandler_t)(void *);

//--------------------------------------------------------------------------------------------------
/**
 *
 * Connect the current client thread to the service providing this API. Block until the service is
 * available.
 *
 * For each thread that wants to use this API, either ConnectService or TryConnectService must be
 * called before any other functions in this API.  Normally, ConnectService is automatically called
 * for the main thread, but not for any other thread. For details, see @ref apiFilesC_client.
 *
 * This function is created automatically.
 */
//--------------------------------------------------------------------------------------------------
void {{apiName}}_ConnectService
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 *
 * Try to connect the current client thread to the service providing this API. Return with an error
 * if the service is not available.
 *
 * For each thread that wants to use this API, either ConnectService or TryConnectService must be
 * called before any other functions in this API.  Normally, ConnectService is automatically called
 * for the main thread, but not for any other thread. For details, see @ref apiFilesC_client.
 *
 * This function is created automatically.
 *
 * @return
 *  - LE_OK if the client connected successfully to the service.
 *  - LE_UNAVAILABLE if the server is not currently offering the service to which the client is
 *    bound.
 *  - LE_NOT_PERMITTED if the client interface is not bound to any service (doesn't have a binding).
 *  - LE_COMM_ERROR if the Service Directory cannot be reached.
 */
//--------------------------------------------------------------------------------------------------
le_result_t {{apiName}}_TryConnectService
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Set handler called when server disconnection is detected.
 *
 * When a server connection is lost, call this handler then exit with LE_FATAL.  If a program wants
 * to continue without exiting, it should call longjmp() from inside the handler.
 */
//--------------------------------------------------------------------------------------------------
LE_FULL_API void {{apiName}}_SetServerDisconnectHandler
(
    {{apiName}}_DisconnectHandler_t disconnectHandler,
    void *contextPtr
);

//--------------------------------------------------------------------------------------------------
/**
 *
 * Disconnect the current client thread from the service providing this API.
 *
 * Normally, this function doesn't need to be called. After this function is called, there's no
 * longer a connection to the service, and the functions in this API can't be used. For details, see
 * @ref apiFilesC_client.
 *
 * This function is created automatically.
 */
//--------------------------------------------------------------------------------------------------
void {{apiName}}_DisconnectService
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Return the sessionRef for the current thread.
 *
 * If the current thread does not have a session ref, then this is a fatal error.
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t _{{apiName}}_GetCurrentSessionRef
(
    void
);
{%- endblock %}
{% block AllFunctionDeclarations %}
#if IFGEN_PROVIDE_PROTOTYPES
{{ super() }}
#elif !IFGEN_TYPES_ONLY
{%- for function in functions %}
{# Currently function prototype is formatter is copied & pasted from interface header template.
 # Should this be abstracted into a common macro?  The prototype is always copy/pasted for
 # legibility in real C code #}
//--------------------------------------------------------------------------------------------------
{{function.comment|FormatHeaderComment}}
//--------------------------------------------------------------------------------------------------
LE_DECLARE_INLINE {{function.returnType|FormatType}} {{apiName}}_{{function.name}}
(
    {%- for parameter in function|CAPIParameters %}
    {{parameter|FormatParameter}}{% if not loop.last %},{% endif %}
        ///< [{{parameter.direction|FormatDirection}}]
             {{-parameter.comments|join("\n///<")|indent(8)}}
    {%-else%}
    void
    {%-endfor%}
)
{
    {% if function.returnType %}return{% endif %} ifgen_{{apiBaseName}}_{{function.name}}(
        _{{apiName}}_GetCurrentSessionRef()
        {%- for parameter in function|CAPIParameters %}{% if loop.first %},{% endif %}
        {{parameter|FormatParameterName}}{% if not loop.last %},{% endif %}
        {%- endfor %}
    );
}
{%- endfor %}
#endif
{%- endblock %}
