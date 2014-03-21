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
package com.sw.readyagent.tcpremote;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.UnknownHostException;
import java.util.Iterator;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import com.sw.readyagent.tcpremote.bridges.ClientServiceBridge;
import com.sw.readyagent.tcpremote.bridges.DeviceServerBridge;
import com.sw.readyagent.tcpremote.utils.PeerAddress;
import com.sw.readyagent.tcpremote.utils.SocketTriplet;

/**
 * @author Gilles
 * 
 */
// TODO generate client ID
public class TCPRemoteServer {

	public final static int BUF_LENGTH = 8192;

	public final static char CONNECT = 1;
	public final static char LISTEN = 4;
	public final static char DATA = 3;
	public final static char CLOSE = 2;

	// server sockets
	private static ServerSocket device_server_socket = null;
	private static ConcurrentHashMap<PeerAddress, ServerSocket> client_server_sockets = new ConcurrentHashMap<PeerAddress, ServerSocket>();

	// device
	private static SocketTriplet device;

	// clients
	private static ConcurrentHashMap<Integer, SocketTriplet> clients = new ConcurrentHashMap<Integer, SocketTriplet>();

	// executor
	private static ExecutorService executor;

	private static int clientId = 0;

	public static synchronized Integer getClientId() {
		clientId = (clientId + 1) % 256;
		return new Integer(clientId);
	}

	/**
	 * @param args
	 *            (server_address:server_port, host_address:host_port,
	 *            local_address:local_port)
	 */
	public static void main(String[] args) {
		// check args
		if (args.length < 3) {
			System.out.println("parameters error\n\t usage: java -jar TCPRemoteServer.jar <server_address:server_port>"
					+ "\n\t<client_address:client_port> <service_address:service_port>");
			System.exit(-1);
		}

		// get conf for command line
		PeerAddress server = null;
		try {
			server = new PeerAddress(args[0], true);
		} catch (UnknownHostException e) {
			System.out.println("server address is unresolved");
			System.exit(-1);
		}

		// open server socket
		try {
			device_server_socket = new ServerSocket(server.getPortnumber(), -1, server.getHostinet());
		} catch (IOException e) {
			System.out.println("could not listen on port " + server.getPortnumber());
			System.exit(-1);
		}
		System.out.println("[server at " + server + " waiting for device connection]");
		System.out.println("+waiting for device connection at " + server);
		Socket device_socket = null;
		try {
			device_socket = device_server_socket.accept();
		} catch (IOException e) {
			System.out.println("accept failed: " + server.getPortnumber());
			System.exit(-1);
		}
		InputStream device_in = null;
		OutputStream device_out = null;
		try {
			device_in = device_socket.getInputStream();
			device_out = device_socket.getOutputStream();
		} catch (IOException e) {
			System.out.println("server failed to get input/output stream for device");
			System.exit(-1);
		}
		device = new SocketTriplet(device_socket, device_in, device_out);
		System.out.println("+device connected on " + server + "...waiting for (" + (int) LISTEN + ")");

		// wait LISTEN from DeviceAppSide
		if (device.waitForCmd(LISTEN) == false)
			System.exit(-1);

		// start device loop
		executor = Executors.newFixedThreadPool(24);
		executor.execute(new DeviceServerBridge(device, clients));

		for (int i = 1; i < args.length; i = i + 2) {
			if (i + 1 >= args.length) {
				break;
			}
			PeerAddress client = null;
			PeerAddress service = null;
			try {
				client = new PeerAddress(args[i], true);
			} catch (UnknownHostException e) {
				System.out.println("client address is unresolved");
				System.exit(-1);
			}

			try {
				service = new PeerAddress(args[i + 1], false);
			} catch (UnknownHostException e) {
				System.out.println("service address is unresolved");
				System.exit(-1);
			}

			System.out.println("[client server at " + client + " -> distant service at " + service + "]");

			// start client server (to receive client connections)
			mountClientServer(client, service);
		}

		try {
			while (System.in.read() != 'q') {
			}

		} catch (IOException e) {
		} finally {
			executor.shutdown();
		}
		System.out.println("server end");
	}

	private static void mountClientServer(PeerAddress client, PeerAddress service) {
		try {
			ServerSocket sockserv = new ServerSocket(client.getPortnumber(), -1, client.getHostinet());
			client_server_sockets.put(client, sockserv);
			executor.execute(new ClientServiceBridge(service, device, clients, executor, sockserv));
			System.out.println("+waiting for clients connection at " + client);
		} catch (IOException e) {
			System.out.println("failed to open client server socket " + client);
		}
	}

	@Override
	protected void finalize() throws Throwable {
		// close client sockets
		closeAll();

		// stop tasks
		executor.shutdown();

		// close server sockets
		if (device_server_socket != null) {
			try {
				device_server_socket.close();
			} catch (Exception e) {
				System.out.println("server could not close device_server_socket");
			}
		}

		if (client_server_sockets != null && !client_server_sockets.isEmpty()) {
			for (Iterator<PeerAddress> it = client_server_sockets.keySet().iterator(); it.hasNext();) {
				PeerAddress key = it.next();
				ServerSocket server = client_server_sockets.get(key);
				if (server == null || server.isClosed()) {
					continue;
				}
				try {
					server.close();
				} catch (IOException e) {
					System.out.println("server could not close client_server_sockets (" + key + ")");
				}
			}
			client_server_sockets.clear();
		}

		super.finalize();
	}

	private void closeAll() {
		if (clients != null && !clients.isEmpty()) {
			for (Iterator<Integer> it = clients.keySet().iterator(); it.hasNext();) {
				Integer key = it.next();
				SocketTriplet client = clients.get(key);
				if (client == null || client.isClosed()) {
					continue;
				}
				client.close();
			}
			clients.clear();
		}

		if (device != null) {
			device.close();
		}
	}

}
