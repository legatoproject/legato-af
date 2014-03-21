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

import java.io.IOException;
import java.net.InetSocketAddress;

import com.sun.net.httpserver.HttpServer;

public class ServerSimu
{
    public static void main(String[] args) throws IOException
    {
    	Integer portnumber = new Integer(8070);
    	if (args.length > 0) {
    		portnumber = Integer.valueOf(args[0]);
    	}
    	HTTPHandlerImpl httpHandler = new HTTPHandlerImpl();
        InetSocketAddress serverAddress = new InetSocketAddress(portnumber.intValue());
		HttpServer server = HttpServer.create(serverAddress, 16);
        server.createContext("/device/com", httpHandler);
        server.start();
        System.out.println("+server started at '" + serverAddress + "'");
        System.out.println("+url: 'http://" + serverAddress.getHostName() + ":" + serverAddress.getPort() + "/device/com" + "'");
    }
}
