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
package com.sw.readyagent.tcpremote.utils;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;
import java.nio.charset.Charset;

/**
 * @author Gilles
 * 
 */
public class SocketTriplet {
	private Socket socket;
	private InputStream input;
	private OutputStream output;
	private boolean closed;

	public SocketTriplet(Socket socket, InputStream input, OutputStream output) {
		this.socket = socket;
		this.input = input;
		this.output = output;
		this.closed = false;
	}

	public void close() {
		try {
			output.close();
		} catch (Exception e) {
			// System.out.println("failed to close output");
		}
		try {
			input.close();
		} catch (Exception e) {
			// System.out.println("failed to close input");
		}
		try {
			socket.close();
		} catch (Exception e) {
			// System.out.println("failed to close socket");
		}
		closed = true;
	}

	public boolean isClosed() {
		return closed;
	}

	public Socket getSocket() {
		return socket;
	}

	public InputStream getInput() {
		return input;
	}

	public OutputStream getOutput() {
		return output;
	}

	public boolean sendCmd(Integer clientId, char targetCmd) {
		byte[] buf = new byte[4];
		buf[0] = 0x00;
		buf[1] = 0x00;
		buf[2] = (byte) (targetCmd & 0xFF);
		buf[3] = (byte) (clientId.intValue() & 0xFF);
		try {
			synchronized (output) {
				output.write(buf, 0, 4);
				output.flush();
			}
			System.out.println("(" + (int) targetCmd + ") sent to device clientId=" + clientId);
		} catch (IOException e) {
			// System.out.println("failed to send (" + (int)targetCmd +
			// ") clientId="
			// + clientId);

			return false;
		}

		return true;
	}
	
	public boolean sendCmd(Integer clientId, char targetCmd, String address, int port) {
		byte[] encodedAddr = address.getBytes(Charset.defaultCharset());
		int addrLength = encodedAddr.length;
		int payload = 2 + addrLength + 2;
		byte[] buf = new byte[4 + payload];
		buf[0] = (byte) ((payload >> 8) & 0xFF);
		buf[1] = (byte) (payload & 0xFF);
		buf[2] = (byte) (targetCmd & 0xFF);
		buf[3] = (byte) (clientId.intValue() & 0xFF);
		
		buf[4] = (byte) ((addrLength >> 8) & 0xFF);
		buf[5] = (byte) (addrLength & 0xFF);
		for (int i=0; i < addrLength; i++){
			buf[6 + i] = encodedAddr[i];
		}
		buf[6 + addrLength] = (byte) ((port >> 8) & 0xFF);
		buf[6 + addrLength + 1] = (byte) (port & 0xFF);

		try {
			synchronized (output) {
				output.write(buf, 0, addrLength + 8);
				output.flush();
			}
			System.out.println("(" + (int) targetCmd + "," + address + "," + port + "," + addrLength +") sent to device clientId=" + clientId);
		} catch (IOException e) {
			// System.out.println("failed to send (" + (int)targetCmd +
			// ") clientId="
			// + clientId);

			return false;
		}

		return true;
	}

	public boolean waitForCmd(char targetCmd) {
		byte[] buf = new byte[4];
		try {
			input.read(buf, 0, 4);
		} catch (IOException e) {
			// System.out.println("read failed");
		}

		int size = ((0xFF & (int) buf[0]) << 8) + (0xFF & (int) buf[1]);
		if (size != 0) {
			// System.out.println("Received data, but expected command (size:" +
			// size + ")");
			return false;
		}

		char cmdId = (char) (0xFF & (int) buf[2]);
		if (cmdId != targetCmd) {
			// System.out.println("received command, but expected (" + targetCmd
			// + ")");
			return false;
		}

		int clientId = (int) (0xFF & (int) buf[3]);
		if (clientId != 0) {
			// System.out.println("received command, but expected clientId=0 ("
			// + clientId + ")");
			return false;
		}
		// System.out.println("(" + targetCmd + ") received from device");
		return true;
	}
}