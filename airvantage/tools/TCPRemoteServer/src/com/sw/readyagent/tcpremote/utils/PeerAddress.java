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
package com.sw.readyagent.tcpremote.utils;

import java.net.InetAddress;
import java.net.UnknownHostException;

public class PeerAddress {
	
	private String hostname;	
	private InetAddress hostinet;
	private int portnumber;
	private boolean resolve;

	public String getHostname() {
		return hostname;
	}

	public InetAddress getHostinet() {
		return hostinet;
	}

	public int getPortnumber() {
		return portnumber;
	}

	public static boolean check(String entry) {
		if (entry != null && entry.length() > (entry.lastIndexOf(":") + 1))
			return true;
		return false;
	}

	public PeerAddress(String entry, boolean resolve) throws UnknownHostException {
		super();
		this.resolve = resolve;
		if (check(entry)) {
			this.hostname = entry.substring(0, entry.lastIndexOf(":"));
			if (resolve) 
				this.hostinet = InetAddress.getByName(hostname);
			this.portnumber = Integer.valueOf(entry.substring(entry.lastIndexOf(":") + 1));
		} else {
			throw new UnknownHostException(entry);
		}
	}
	
	@Override
	public String toString() {
		if (resolve)
			return hostinet + ":" + portnumber;
		return hostname + ":" + portnumber;
	}
}