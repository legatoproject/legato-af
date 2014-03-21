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

import java.util.concurrent.ConcurrentHashMap;

import com.sw.readyagent.tcpremote.utils.SocketTriplet;

/**
 * @author Gilles
 * 
 */
public class ClientReader implements Runnable {

	public ClientReader(Integer clientId, ConcurrentHashMap<Integer, SocketTriplet> clients, SocketTriplet device) {
		super();
		this.clients = clients;
		this.device = device;
		this.clientId = clientId;
	}

	private Integer clientId;
	private ConcurrentHashMap<Integer, SocketTriplet> clients;
	private SocketTriplet device;

	@Override
	public void run() {
		System.out.println("client reader begin clientId=" + clientId);

		char cmdId = 0;
		int size = 0;
		byte[] buf = new byte[TCPRemoteServer.BUF_LENGTH];

		SocketTriplet client = clients.get(clientId);
		if (client == null || client.isClosed()) {
			// System.out.println("client reader end clientId=" + clientId);
			return;
		}
		while (true) {
			try {
				size = client.getInput().read(buf, 4, TCPRemoteServer.BUF_LENGTH - 4);
			} catch (Exception e) {
				// System.out.println("failed to read from client clientId=" +
				// clientId + " send (2)");
				device.sendCmd(clientId, TCPRemoteServer.CLOSE);
				break;
			}
			if (size > 0) {
				cmdId = TCPRemoteServer.DATA;
				buf[0] = (byte) (((char) size & 0xFF00) >> 8);
				buf[1] = (byte) ((char) size & 0xFF);
				buf[2] = (byte) (cmdId & 0xFF);
				buf[3] = (byte) (clientId.intValue() & 0xFF);

				// System.out.println("client->device: size=" + size + " cmdId="
				// + (int)cmdId + " clientId=" + clientId);

				try {
					synchronized (device.getOutput()) {
						device.getOutput().write(buf, 0, size + 4);
						device.getOutput().flush();
					}
				} catch (Exception e) {
					// System.out.println("client failed to write to device");
					break;
				}
			} else if (size == -1) {
				// System.out.println("failed to read from client clientId=" +
				// clientId + " send (2)");
				device.sendCmd(clientId, TCPRemoteServer.CLOSE);
				break;
			}
		}
		//client.close();
		System.out.println("client reader end clientId=" + clientId);
	}
}
