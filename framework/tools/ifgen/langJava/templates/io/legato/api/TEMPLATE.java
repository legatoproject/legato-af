{%- for comment in fileComments %}
{{comment|FormatHeaderComment}}
{%- endfor %}

// Generated interface for API {{apiName}}.
// This is a generated file, do not edit.

package io.legato.api;

import io.legato.Ref;
import io.legato.Result;
import java.util.Map;
import java.util.HashMap;
import java.io.FileDescriptor;

public interface {{apiName}}
{
    {#- TODO: Re-add later.  For now omit for compatibility with previous java generator.
    {%- for definition in definitions %}
    {%- if definition.value is number %}
    public static final long {{definition.name}} = {{definition.value}};
    {%- else %}
    public static final String {{definition.name}} = "{{definition.value}}";
    {%- endif %}
    {%- endfor %} #}

    {%- for type in types %}
    {#- TODO: Is this exception necessary?  Maybe handler references should have a class as well #}
    {%- if type is ReferenceType and type is not HandlerReferenceType %}
    public class {{type.name}}Ref
    {
        private long nativeRef;

        public {{type.name}}Ref(long newRef)
        {
            nativeRef = newRef;
        }

        public long getRef()
        {
            return nativeRef;
        }
    }
    {%- elif type is BitMaskType %}
    public static class {{type.name}}
    {
        {%- for element in type.elements %}
        public static final int {{element.name}} = {{element.value}};
        {%- endfor %}

        private int value;

        public {{type.name}}(int newValue)
        {
            value = newValue;
        }

        public static {{type.name}} fromInt(int newValue)
        {
            return new {{type.name}}(newValue);
        }

        public int getValue()
        {
            return value;
        }
    }
    {%- elif type is EnumType %}
    public enum {{type.name}}
    {
        {%- for element in type.elements %}
        {{element.name}}({{element.value}}){% if loop.last %};{% else %},{% endif %}
        {%- endfor %}

        private int value;

        {{type.name}}(int newValue)
        {
            value = newValue;
        }

        public int getValue()
        {
            return value;
        }

        private static final Map<Integer, {{type.name}}> valueMap
             {#- #} = new HashMap<Integer, {{type.name}}>();

        static
        {
            for ({{type.name}} item : {{type.name}}.values())
            {
                valueMap.put(item.value, item);
            }
        }

        public static {{type.name}} fromInt(int newValue)
        {
            return valueMap.get(newValue);
        }
    }
    {%- elif type is HandlerType %}
    public interface {{type.name}}
    {
        public void handle
        (
            {%- for parameter in type.parameters %}
            {{parameter|FormatParameter}}{% if not loop.last %},{% endif %}
            {%- endfor %}
        );
    }
    {%- endif %}
    {%- endfor %}
    {%- for function in functions %}
    public {{function.returnType|FormatType}} {{function.name}}
    (
        {%- for parameter in function.parameters %}
        {{parameter|FormatParameter}}{% if not loop.last %},{% endif %}
        {%- endfor %}
    );
    {%- endfor %}
}
