/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Sierra Wireless - initial API and implementation
 *******************************************************************************/
/**
 *
 */
package com.anwrt.protocol.test.server;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.lang.Double;
import java.lang.Float;
import java.lang.Integer;
import java.util.StringTokenizer;

import com.anwrt.commons.protocol.model.AwtMessage;
import com.anwrt.commons.protocol.model.data.AwtData;
import com.anwrt.commons.protocol.model.data.composite.AwtCommand;
import com.anwrt.commons.protocol.model.data.composite.AwtEvent;
import com.anwrt.commons.protocol.model.data.composite.AwtResponse;
import com.anwrt.commons.protocol.model.data.timeStamped.AwtTimeStampedValue;

/**
 * <br>
 * creation 17 juil. 2009
 *
 * @author <a href="mailto:david.francois@anyware-tech.com">David FRANCOIS</a>
 *
 */
public class MessageBuilderHelper
{
    private static final BufferedReader reader = new BufferedReader(new InputStreamReader(System.in));

    public static AwtMessage getCommandMessage() throws IOException
    {
        System.out.print("Command name : ");
        String cmdName = reader.readLine();

        System.out.print("Path : ");
        String path = reader.readLine();

        System.out.print("Arguments arg1,arg2,arg3... : ");
        String cmdArgs = reader.readLine();

        System.out.print("TicketId : ");
        String ticketId = reader.readLine();

        StringTokenizer tokenizer = new StringTokenizer(cmdArgs.toString(), ",");
        List<String> argList = new ArrayList<String>();
        while (tokenizer.hasMoreElements())
        {
            argList.add(tokenizer.nextToken());
        }

        AwtCommand command = new AwtCommand();
        command.setArgs(argList);
        command.setName(cmdName.toString());

        return getMessage(path, ticketId, AwtMessage.SERVER_COMMAND, command);
    }

    public static AwtMessage getResponseMessage() throws IOException
    {
        System.out.print("Path : ");
        String path = reader.readLine();

        System.out.print("Data : ");
        String data = reader.readLine();

        System.out.print("TicketId to be ackwnoledged : ");
        String ticketId = reader.readLine();

        System.out.print("Response status code : ");
        String statusCode = reader.readLine();

        AwtResponse response = new AwtResponse();
        response.setData(data.toString());
        response.setTicketId(parseInt(ticketId, null));
        response.setStatusCode(parseInt(statusCode, 0));

        return getMessage(path, ticketId, AwtMessage.RESPONSE_MESSAGE, response);
    }

    public static AwtMessage getEventMessage() throws IOException
    {
        System.out.print("Path : ");
        String path = reader.readLine();
        System.out.print("Data : ");
        String data = reader.readLine();

        System.out.print("Type : ");
        String type = reader.readLine();

        System.out.print("TicketId : ");
        String ticketId = reader.readLine();

        AwtEvent event = new AwtEvent();
        event.setData(data.toString());
        event.setCode(0);
        event.setType(parseInt(type, 0));

        return getMessage(path, ticketId, AwtMessage.EVENT_MESSAGE, event);
    }

    public static AwtMessage getDataMessage() throws IOException
    {
        System.out.print("Path : ");
        String path = reader.readLine();

        System.out.print("Data Name : ");
        String data = reader.readLine();

        System.out.print("Data Value : ");
        String dataVal = reader.readLine();

        System.out.print("TicketId : ");
        String ticketId = reader.readLine();

        AwtTimeStampedValue value = new AwtTimeStampedValue();
        value.setData(dataVal.toString());
        value.setTimeStamp((int) (System.currentTimeMillis() / 1000));

        HashMap<String, AwtData> map = new HashMap<String, AwtData>();
        map.put(data, value);

        return getMessage(path, ticketId,AwtMessage.DATA_MESSAGE, map);
    }

    public static AwtMessage getCorrelatedDataMessage() throws IOException
    {
        Object value = null;

        System.out.print("Path : ");
        String path = reader.readLine();

        System.out.print("Data Name : ");
        String data = reader.readLine();

        System.out.print("Data Value : ");
        String dataVal = reader.readLine();

        // try to convert the dataValue to an Integer
        try
        {
            value = Integer.parseInt(dataVal);
        }
        catch (NumberFormatException e)
        {
            //the user dataValue is not an integer, let it as a string
            value = dataVal;
        }

        // try to convert the dataValue to a float
        try
        {
            value = Float.parseFloat(dataVal);
        }
        catch (NumberFormatException e)
        {
            //the user dataValue is not an integer, let it as a string
            value = dataVal;
        }

        // try to convert the dataValue to a Double
        try
        {
            value = Double.parseDouble(dataVal);
        }
        catch (NumberFormatException e)
        {
            //the user dataValue is not an integer, let it as a string
            value = dataVal;
        }

        System.out.print("TicketId : ");
        String ticketId = reader.readLine();

        HashMap<String, Object> map = new HashMap<String, Object>();
        map.put(data, value);

        return getMessage(path, ticketId, AwtMessage.CORRELATED_DATA_MESSAGE, map);
    }

    public static AwtMessage getMessage(String path, String ticketId, int type, Object body)
    {
        AwtMessage message = new AwtMessage();
        message.setPath(path.toString());
        message.setTicketId(parseInt(ticketId, null));
        message.setType(type);
        message.setBody(body);

        return message;
    }

    /*
     * try to parse the string value given as parameter to an integer. If an
     * error occurred, return the second argument.
     */
    private static Integer parseInt(String value, Integer errorValue)
    {
        try
        {
            return Integer.parseInt(value.toString().trim());
        }
        catch (NumberFormatException e)
        {
            return errorValue;
        }
    }
}
