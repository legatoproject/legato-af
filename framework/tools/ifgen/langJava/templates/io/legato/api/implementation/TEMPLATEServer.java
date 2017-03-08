{%- set qualifiedTypes=true %}{# API types need to be qualified in this file #}
{%- import "pack.templ" as pack with context %}
// Generated server implementation of API {{apiName}}.
// This is a generated file, do not edit.

package io.legato.api.implementation;

import java.io.FileDescriptor;
import java.lang.AutoCloseable;
import io.legato.Ref;
import io.legato.Result;
import io.legato.Message;
import io.legato.MessageBuffer;
import io.legato.MessageEvent;
import io.legato.Service;
import io.legato.Protocol;
import io.legato.ServerSession;
import io.legato.SessionEvent;
{%- for import in imports %}
import io.legato.api.{{import}};
{%- else %}
{% endfor %}
import io.legato.api.{{apiName}};

public class {{apiName}}Server implements AutoCloseable
{
    private static final String protocolIdStr = "{{idString}}";
    private static final String serviceInstanceName = "{{apiName}}";
    {%- if apiName in [ "le_secStore", "secStoreGlobal", "secStoreAdmin", "le_fs" ] %}
    private static final int maxMsgSize = 8504;
    {%- elif apiName == "le_cfg" %}
    private static final int maxMsgSize = 1604;
    {%- else %}
    private static final int maxMsgSize = 1104;
    {%- endif %}

    private Service service;
    private ServerSession currentSession;
    private {{apiName}} serverImpl;

    public {{apiName}}Server({{apiName}} newServerImpl)
    {
        service = null;
        currentSession = null;
        serverImpl = newServerImpl;
    }

    public void open()
    {
        open(serviceInstanceName);
    }

    public void open(String serviceName)
    {
        currentSession = null;
        service = new Service(new Protocol(protocolIdStr, maxMsgSize), serviceName);

        service.setReceiveHandler(new MessageEvent()
            {
                public void handle(Message message)
                {
                    OnClientMessageReceived(message);
                }
            });

        service.addCloseHandler(new SessionEvent<ServerSession>()
            {
                public void handle(ServerSession session)
                {
                    // TODO: Clean up...
                }
            });

        service.advertiseService();
    }

    @Override
    public void close()
    {
        if (service != null)
        {
            currentSession = null;
            service.close();
            service = null;
        }
    }
    {%- for function in functions %}

    private void handle{{function.name}}(MessageBuffer buffer)
    {
        {%- for parameter in function.parameters %}
        {%- if parameter is InParameter %}
        {%- if parameter.apiType is HandlerType %}
        long {{parameter.name}}Context = buffer.readLongRef();
        {%- endif %}
        {{parameter.apiType|FormatType}} _{{parameter.name}} =
            {#- #} {{pack.UnpackValue(parameter.apiType, parameter.name, function.name)|indent(8)}};
        {%- else %}
        Ref<{{parameter.apiType|FormatBoxedType}}> _{{parameter.name}} =
          {#- #} new Ref<{{parameter.apiType|FormatBoxedType}}>({{parameter.apiType|DefaultValue}});
        {%- endif %}
        {%- endfor %}

        {% if function.returnType %}{{function.returnType|FormatType}} result = {% endif -%}
        serverImpl.{{function.name}}(
            {%- for parameter in function.parameters -%}
            _{{parameter.name}}{% if not loop.last %}, {% endif -%}
            {%- endfor %});
        buffer.resetPosition();
        buffer.writeInt(MessageID_{{function.name}});
        {%- if function.returnType %}

        {{pack.PackValue(function.returnType, "result")}}
        {%- endif %}
        {%- for parameter in function.parameters if parameter is OutParameter %}
        {%- if loop.first %}
{% endif %}
        {{pack.PackParameter(parameter, "_"+parameter.name+".getValue()")}}
        {%- endfor %}
    }
    {%- endfor %}
    {%- for function in functions %}
    private static final int MessageID_{{function.name}} = {{loop.index0}};
    {%- endfor %}

    private void OnClientMessageReceived(Message clientMessage)
    {
        MessageBuffer buffer = null;

        try
        {
            buffer = clientMessage.getBuffer();
            currentSession = clientMessage.getServerSession();
            int messageId = buffer.readInt();

            switch (messageId)
            {
                {%- for function in functions %}
                case MessageID_{{function.name}}:
                    handle{{function.name}}(buffer);
                    break;
                {%- endfor %}
            }

            clientMessage.respond();
        }
        finally
        {
            currentSession = null;

            if (buffer != null)
            {
                buffer.close();
            }
        }
    }
}
