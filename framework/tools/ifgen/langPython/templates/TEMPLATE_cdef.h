
{#- This goes into cffi.cdef() #}

//--------------------------------------------------------------------------------------------------
/**
 * Required types from le_basics.h
 *
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Standard result codes.
 *
 * @note All error codes are negative integers. They allow functions with signed
 *       integers to return non-negative values when successful or standard error codes on failure.
 * @deprecated the result code LE_NOT_POSSIBLE is scheduled to be removed.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_OK = 0,                  ///< Successful.
    LE_NOT_FOUND = -1,          ///< Referenced item does not exist or could not be found.
    LE_NOT_POSSIBLE = -2,       ///< @deprecated It is not possible to perform the requested action.
    LE_OUT_OF_RANGE = -3,       ///< An index or other value is out of range.
    LE_NO_MEMORY = -4,          ///< Insufficient memory is available.
    LE_NOT_PERMITTED = -5,      ///< Current user does not have permission to perform requested action.
    LE_FAULT = -6,              ///< Unspecified internal error.
    LE_COMM_ERROR = -7,         ///< Communications error.
    LE_TIMEOUT = -8,            ///< A time-out occurred.
    LE_OVERFLOW = -9,           ///< An overflow occurred or would have occurred.
    LE_UNDERFLOW = -10,         ///< An underflow occurred or would have occurred.
    LE_WOULD_BLOCK = -11,       ///< Would have blocked if non-blocking behaviour was not requested.
    LE_DEADLOCK = -12,          ///< Would have caused a deadlock.
    LE_FORMAT_ERROR = -13,      ///< Format error.
    LE_DUPLICATE = -14,         ///< Duplicate entry found or operation already performed.
    LE_BAD_PARAMETER = -15,     ///< Parameter is invalid.
    LE_CLOSED = -16,            ///< The resource is closed.
    LE_BUSY = -17,              ///< The resource is busy.
    LE_UNSUPPORTED = -18,       ///< The underlying resource does not support this operation.
    LE_IO_ERROR = -19,          ///< An IO operation failed.
    LE_NOT_IMPLEMENTED = -20,   ///< Unimplemented functionality.
    LE_UNAVAILABLE = -21,       ///< A transient or temporary loss of a service or resource.
    LE_TERMINATED = -22,        ///< The process, operation, data stream, session, etc. has stopped.
}
le_result_t;


//--------------------------------------------------------------------------------------------------
/**
 * ON/OFF type.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_OFF = 0,
    LE_ON  = 1,
}
le_onoff_t;


const char** {{apiName}}_ServiceInstanceNamePtr;

{% for define in definitions %}

{%- if define.value is string %}

{#- cffi does not support #define FOO "strings" #}

static char *const {{apiName|upper}}_{{define.name}} = "{{define.value}}";

{%- else %}

#define {{apiName|upper}}_{{define.name}} {{define.value}}

{%- endif %}
{%- endfor %}

{%- for type in types if type is not HandlerType %}

{%- if type is EnumType %}
typedef enum
{
    {%- for element in type.elements %}
    {{apiName|upper}}_{{element.name}} = {{element.value}}{%if not loop.last%},{%endif%}
        ///<{{element.comments|join("\n///<")|indent(8)}}
    {%- endfor %}
}
{{type|FormatType}};
{%- elif type is BitMaskType %}
typedef enum
{
    {%- for element in type.elements %}
    {{apiName|upper}}_{{element.name}} = {{"0x%x" % element.value}}{%if not loop.last%},{%endif%}
    {%- if element.comments %}        ///<{{element.comments|join("\n///<")|indent(8)}}{%endif%}
    {%- endfor %}
}
{{type|FormatType}};
{%- elif type is ReferenceType %}
typedef struct {{apiName}}_{{type.name}}* {{type|FormatType}};
{%- endif %}
{% endfor %}

{%- for handler in types if handler is HandlerType %}

typedef void (*{{handler|FormatType}})
(
    {%- for parameter in handler|CAPIParameters %}
    {{parameter|FormatParameter}}{% if not loop.last %},{% endif %}
    {%-endfor%}
);
{%- endfor %}
{%- for function in functions %}
{{function.returnType|FormatType}} {{apiName}}_{{function.name}}
(
    {%- for parameter in function|CAPIParameters %}
    {{parameter|FormatParameter}}{% if not loop.last %},{% endif %}
    {%-else%}
    void
    {%-endfor%}
);
{% if function is EventFunction and function.name.startswith('Add') %}
extern "Python" void {{apiName}}_{{function.event.name}}Py
(
    {%- for parameter in function.event|HandlerParamsForEvent%}
    {{parameter|FormatParameter}}{% if not loop.last %},{% endif %}
    {%-endfor-%}
    {#- Do not add a comma if there is no parameter #}
    {% if function.event|HandlerParamsForEvent|count != 0 %},{% endif %}
    void* contextPtr
);
{% endif %}
{%- endfor %}

typedef void (*{{apiName}}_DisconnectHandler_t)(void *);

void {{apiName}}_ConnectService
(
    void
);

le_result_t {{apiName}}_TryConnectService
(
    void
);

void {{apiName}}_SetServerDisconnectHandler
(
    {{apiName}}_DisconnectHandler_t disconnectHandler,
    void *contextPtr
);

void {{apiName}}_DisconnectService
(
    void
);
