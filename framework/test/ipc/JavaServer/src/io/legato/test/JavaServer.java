/*
 * Copyright (C) Sierra Wireless Inc.
 */

package io.legato.test;

import java.util.logging.Logger;
import java.math.BigInteger;

import io.legato.Ref;
import io.legato.Component;
import io.legato.api.ipcTest;

public class JavaServer extends Component implements ipcTest {
	@Override
	public void EchoSimple(BigInteger in, Ref<BigInteger> out) {
		if (out != null) {
			out.setValue(in);
		}
	}

	@Override
	public void EchoSmallEnum(ipcTest.SmallEnum in, Ref<ipcTest.SmallEnum> out) {
		if (out != null) {
			out.setValue(in);
		}
	}

	@Override
	public void EchoLargeEnum(ipcTest.LargeEnum in, Ref<ipcTest.LargeEnum> out) {
		if (out != null) {
			out.setValue(in);
		}
	}

	@Override
	public void EchoSmallBitMask(ipcTest.SmallBitMask in, Ref<ipcTest.SmallBitMask> out) {
		if (out != null) {
			out.setValue(new ipcTest.SmallBitMask(in.getValue()));
		}
	}

	@Override
	public void EchoLargeBitMask(ipcTest.LargeBitMask in, Ref<ipcTest.LargeBitMask> out) {
		if (out != null) {
			out.setValue(new ipcTest.LargeBitMask(in.getValue()));
		}
	}

	@Override
	public void EchoReference(ipcTest.SimpleRef in, Ref<ipcTest.SimpleRef> out) {
		if (out != null) {
			out.setValue(in);
		}
	}

	@Override
	public void EchoString(String InString, Ref<String> OutString) {
		if (OutString != null) {
			OutString.setValue(InString);
		}
	}

	@Override
	public void ExitServer() {
		System.exit(-1);
	}

	@Override
	public void componentInit() {
	}
}
