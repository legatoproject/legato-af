{#-
 #  Jinja2 template for generating server header. for Legato APIs.
 #
 #  Copyright (C) Sierra Wireless Inc.
 #}
{% extends "TEMPLATE_header.h" %}
{%- block InterfaceHeader %}
{%- if args.async %}

//--------------------------------------------------------------------------------------------------
/**
 * Command reference for async server-side function support.  The interface function receives the
 * reference, and must pass it to the corresponding respond function.
 */
//--------------------------------------------------------------------------------------------------
typedef struct {{apiName}}_ServerCmd* {{apiName}}_ServerCmdRef_t;
{%- endif %}{% if imports %}

// Interface specific includes
{%- for import in imports %}
#include "{{import}}_server.h"
{%- endfor %}
{%- endif %}
{%- endblock %}
{% block GenericFunctions -%}
{% if functions or handlers -%}
//--------------------------------------------------------------------------------------------------
/**
 * Get the server service reference
 */
//--------------------------------------------------------------------------------------------------
le_msg_ServiceRef_t {{apiName}}_GetServiceRef
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the client session reference for the current message
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t {{apiName}}_GetClientSessionRef
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the server and advertise the service.
 */
//--------------------------------------------------------------------------------------------------
void {{apiName}}_AdvertiseService
(
    void
);
{%- endif %}
{%- endblock %}
{% block FunctionDeclaration %}
{%- if not args.async %}
{{ super() }}
{%- else %}
{%- if function is EventFunction or function is HasCallbackFunction %}
{{ super() }}
{%- else %}
//--------------------------------------------------------------------------------------------------
/**
 * Server-side respond function for {{apiName}}_{{function.name}}
 */
//--------------------------------------------------------------------------------------------------
void {{apiName}}_{{function.name}}Respond
(
    {{apiName}}_ServerCmdRef_t _cmdRef
    {%- if function.returnType %},
    {{function.returnType|FormatType}} _result
    {%- endif %}
    {%- for parameter in function|CAPIParameters if parameter is OutParameter %},
    {{parameter|FormatParameter(forceInput=True)}}
    {%- endfor %}
);

//--------------------------------------------------------------------------------------------------
/**
 * Prototype for server-side async interface function
 */
//--------------------------------------------------------------------------------------------------
void {{apiName}}_{{function.name}}
(
    {{apiName}}_ServerCmdRef_t _cmdRef
    {%- for parameter in function|CAPIParameters if parameter is InParameter %},
    {{parameter|FormatParameter(forceInput=True)}}
    {%- endfor %}
);
{%- endif %}
{%- endif %}
{% endblock %}
