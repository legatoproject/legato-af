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
// ------------------------------------------------
// $Id$
// (c) Anyware Technologies 2008 www.anyware-tech.com
// ------------------------------------------------
package com.anwrt.protocol.test.server;

import java.io.BufferedReader;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;

import com.anwrt.commons.hessian.model.HessianEnvelope;
import com.anwrt.commons.hessian.model.HessianEnvelopeChunk;
import com.anwrt.commons.protocol.AwtHessianParser;
import com.anwrt.commons.protocol.AwtHessianSerializer;
import com.anwrt.commons.protocol.exceptions.AwtHessianSerializerException;
import com.anwrt.commons.protocol.model.AwtMessage;
import com.sun.net.httpserver.HttpExchange;
import com.sun.net.httpserver.HttpHandler;

public class HTTPHandlerImpl implements HttpHandler {
	private AwtHessianParser parser;

	private AwtHessianSerializer serializer;

	public HTTPHandlerImpl() {
		parser = new AwtHessianParser();
		serializer = new AwtHessianSerializer();
	}

	public void handle(HttpExchange httpExchange) {
		try {
			InputStream in = httpExchange.getRequestBody();

			System.out.println("Connection received from " + httpExchange.getRemoteAddress());
			System.out.println("Buffer size : " + in.available());

			// deserialize incoming messages
			deserialize(in);
			int option = getUserChoice();

			// create appropriate message
			byte[] data = null;
			switch (option) {
			case 1:
				data = serialize(MessageBuilderHelper.getCommandMessage());
				break;
			case 2:
				data = serialize(MessageBuilderHelper.getResponseMessage());
				break;
			case 3:
				data = serialize(MessageBuilderHelper.getEventMessage());
				break;
			case 4:
				data = serialize(MessageBuilderHelper.getDataMessage());
				break;
			case 5:
				data = serialize(MessageBuilderHelper.getCorrelatedDataMessage());
				break;
			default:
			case 6:
				// empty
				break;
			}

			// serialize user message and send it to the httpClient
			httpExchange.sendResponseHeaders(200, (data != null) ? data.length : 1);
			if (data != null) {
				httpExchange.getResponseBody().write(data);
			} else {
				httpExchange.getResponseBody().write(' ');
			}
			httpExchange.getResponseBody().flush();
		} catch (Throwable t) {
			t.printStackTrace();
		}
	}

	// deserialize data from the stream given as parameter
	private void deserialize(InputStream in) {
		try {
			System.out.println("Receive : \r\n" + parser.deserialize(parser.parseEnvelope(in)));
		} catch (Throwable t) {
			t.printStackTrace();
		}
	}

	// serialize the message given as parameter
	private byte[] serialize(AwtMessage message) throws IOException {
		ByteArrayOutputStream bos = new ByteArrayOutputStream();
		try {
			serializer.serialize(message).render(bos, serializer);

			HessianEnvelope envelope = new HessianEnvelope();
			envelope.addChunk(new HessianEnvelopeChunk(bos.toByteArray()));

			bos = new ByteArrayOutputStream();
			envelope.render(bos, serializer);
		} catch (AwtHessianSerializerException e) {
			e.printStackTrace();
		}
		return bos.toByteArray();
	}

	// prints all available options and returns user response
	private int getUserChoice() {
		int choice = -1;
		do {
			System.out.println("Available message response type:");
			System.out.println(" 1 - Command");
			System.out.println(" 2 - Response");
			System.out.println(" 3 - Event");
			System.out.println(" 4 - TimeStamped data");
			System.out.println(" 5 - Correlated data");
			System.out.println(" 6 - <empty> (default)");
			System.out.print("Choice:");
			try {
				choice = Integer.parseInt(new BufferedReader(new InputStreamReader(System.in)).readLine());
			} catch (Exception e) {
				choice = 6;
			}
		} while (choice < 1 || choice > 6);
		return choice;
	}
}