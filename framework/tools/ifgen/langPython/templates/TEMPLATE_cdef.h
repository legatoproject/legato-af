
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
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_OK = 0,                  ///< Successful.
    LE_NOT_FOUND = -1,          ///< Referenced item does not exist or could not be found.
    LE_OUT_OF_RANGE = -2,       ///< An index or other value is out of range.
    LE_NO_MEMORY = -3,          ///< Insufficient memory is available.
    LE_NOT_PERMITTED = -4,      ///< Current user does not have permission to perform requested action.
    LE_FAULT = -5,              ///< Unspecified internal error.
    LE_COMM_ERROR = -6,         ///< Communications error.
    LE_TIMEOUT = -7,            ///< A time-out occurred.
    LE_OVERFLOW = -8,           ///< An overflow occurred or would have occurred.
    LE_UNDERFLOW = -9,          ///< An underflow occurred or would have occurred.
    LE_WOULD_BLOCK = -10,       ///< Would have blocked if non-blocking behaviour was not requested.
    LE_DEADLOCK = -11,          ///< Would have caused a deadlock.
    LE_FORMAT_ERROR = -12,      ///< Format error.
    LE_DUPLICATE = -13,         ///< Duplicate entry found or operation already performed.
    LE_BAD_PARAMETER = -14,     ///< Parameter is invalid.
    LE_CLOSED = -15,            ///< The resource is closed.
    LE_BUSY = -16,              ///< The resource is busy.
    LE_UNSUPPORTED = -17,       ///< The underlying resource does not support this operation.
    LE_IO_ERROR = -18,          ///< An IO operation failed.
    LE_NOT_IMPLEMENTED = -19,   ///< Unimplemented functionality.
    LE_UNAVAILABLE = -20,       ///< A transient or temporary loss of a service or resource.
    LE_TERMINATED = -21,        ///< The process, operation, data stream, session, etc. has stopped.
    LE_IN_PROGRESS = -22,       ///< The operation is in progress.
    LE_SUSPENDED = -23,         ///< The operation is suspended.
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
