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
package com.sw.readyagent.tcpremote.bridges;

import java.io.InputStream;
import java.io.OutputStream;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ExecutorService;

import com.sw.readyagent.tcpremote.ClientReader;
import com.sw.readyagent.tcpremote.TCPRemoteServer;
import com.sw.readyagent.tcpremote.utils.PeerAddress;
import com.sw.readyagent.tcpremote.utils.SocketTriplet;

/**
 * @author Gilles
 * 
 */
public class ClientServiceBridge implements Runnable {

	private PeerAddress service;
	private SocketTriplet device;
	private ConcurrentHashMap<Integer, SocketTriplet> clients;
	private ExecutorService executor;
	private ServerSocket client_server_socket = null;

	/*
	 * (non-Javadoc)
	 * 
	 * @see java.lang.Runnable#run()
	 */
	@Override
	public void run() {
		while (true) {
			Socket client_socket = null;
			try {
				client_socket = client_server_socket.accept();
			} catch (Exception e) {
				System.out.println("server failed to accept client connection");
				continue;
			}
			InputStream client_in = null;
			OutputStream client_out = null;
			try {
				client_in = client_socket.getInputStream();
				client_out = client_socket.getOutputStream();
			} catch (Exception e) {
				System.out.println("server failed to get input/output stream for client");
				continue;
			}
			SocketTriplet newClient = new SocketTriplet(client_socket, client_in, client_out);

			// register client
			Integer key = TCPRemoteServer.getClientId();
			if (clients.get(key) != null) {
				clients.get(key).close();
			}
			clients.put(key, newClient);
			System.out.println("+client (" + key + ") connected (distant service at " + service + ")");

			// start client reader
			executor.execute(new ClientReader(key, clients, device));

			// send CONNECT to DeviceAppSide
			device.sendCmd(key, TCPRemoteServer.CONNECT, service.getHostname(), service.getPortnumber());
		}
	}

	public ClientServiceBridge(PeerAddress service, SocketTriplet device, ConcurrentHashMap<Integer, SocketTriplet> clients,
			ExecutorService executor, ServerSocket client_server_socket) {
		super();
		this.service = service;
		this.device = device;
		this.clients = clients;
		this.executor = executor;
		this.client_server_socket = client_server_socket;
	}

}
