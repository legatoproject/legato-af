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

import java.util.Iterator;
import java.util.concurrent.ConcurrentHashMap;

import com.sw.readyagent.tcpremote.TCPRemoteServer;
import com.sw.readyagent.tcpremote.utils.SocketTriplet;

/**
 * @author Gilles
 * 
 */
public class DeviceServerBridge implements Runnable {

	public DeviceServerBridge(SocketTriplet device, ConcurrentHashMap<Integer, SocketTriplet> clients) {
		super();
		this.device = device;
		this.clients = clients;
	}

	private SocketTriplet device;
	private ConcurrentHashMap<Integer, SocketTriplet> clients;

	@Override
	public void run() {
		System.out.println("device server bride established");

		int read = 0;
		char cmdId = 0;
		int size = 0;
		byte[] buf = new byte[TCPRemoteServer.BUF_LENGTH];
		boolean fatalError;

		while (true) {
			size = 4;
			fatalError = false;
			while (size > 0) {
				try {
					read = device.getInput().read(buf, 0, (size > TCPRemoteServer.BUF_LENGTH) ? TCPRemoteServer.BUF_LENGTH : size);
					System.out.println("-----\n->read [header]:" + read + "(size=" + size + ")");
				} catch (Exception e) {
					// System.out.println("device reader failed to read from device");
					fatalError = true;
					break;
				}
				if (read == -1) {
					// System.out.println("device reader failed to read from device");
					fatalError = true;
					break;
				}
				size = size - read;
			}
			if (fatalError) {
				break;
			}

			size = ((0xFF & (int) buf[0]) << 8) + (0xFF & (int) buf[1]);
			cmdId = (char) (0xFF & (int) buf[2]);
			Integer clientId = new Integer((int) (0xFF & (int) buf[3]));
			System.out.println("device->client: size=" + size + " cmdId=" + (int) cmdId + " clientId=" + clientId);

			SocketTriplet client = clients.get(clientId);
			if (client == null || client.isClosed()) {
				if (cmdId != TCPRemoteServer.CLOSE) {
					device.sendCmd(clientId, TCPRemoteServer.CLOSE);
					if (client != null && client.isClosed()) {
						skipOver(size);
					}
				}		
				continue;
			}

			if (cmdId == TCPRemoteServer.DATA) {
				if (size < 0) {
					// System.out.println("device reader received bad frame");
					break;
				}

				fatalError = false;
				while (size > 0) {
					try {
						read = device.getInput().read(buf, 0, (size > TCPRemoteServer.BUF_LENGTH) ? TCPRemoteServer.BUF_LENGTH : size);
						System.out.println(" ->read [payload]:" + read + "(size=" + size + ")");
					} catch (Exception e) {
						// System.out.println("device reader failed to read from device");
						fatalError = true;
						break;
					}
					if (read == -1) {
						// System.out.println("device reader failed to read from device");
						fatalError = true;
						break;
					}
					size = size - read;
					try {
						client.getOutput().write(buf, 0, read);
						client.getOutput().flush();
					} catch (Exception e) {
						System.out.println("device reader failed to write to client clientId=" + clientId);
						client.sendCmd(clientId, TCPRemoteServer.CLOSE);
						//client.close();
					}
				}

				if (fatalError) {
					break;
				}

			} else if (cmdId == TCPRemoteServer.CLOSE) {
				// System.out.println("device reader received (2)...closing client for clientId="+ clientId);
				client.close();

			} else {
				System.out.println("--->unknown cmdId cmdId="+ (int)cmdId);
				skipOver(size);
			}
		}
		closeAll();
		System.out.println("device server bridge closed");
	}

	private void skipOver(long size) {
		try {
			long skipped = 0;
			int available = 0;
			do {
				available = device.getInput().available();
				skipped = device.getInput().skip(available);
				System.out.println("skipped:" + skipped);
				size = size - skipped;
			} while (size > 0 && available > 0);
		} catch (Exception e) {
			// not a big deal
		}
	}

	private void closeAll() {
		if (clients != null && !clients.isEmpty()) {
			for (Iterator<Integer> it = clients.keySet().iterator(); it.hasNext();) {
				Integer key = it.next();
				SocketTriplet client = clients.get(key);
				if (client == null) {
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
