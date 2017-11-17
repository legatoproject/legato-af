package io.legato.samples;

import io.legato.Component;
import io.legato.Level;

public class javaHelloWorldComponent extends Component {
	public void componentInit() {
		getLogger().log(Level.INFO, "Hello, World (from Java).");
	}
}
