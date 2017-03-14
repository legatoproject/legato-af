package io.legato.samples;

import io.legato.Component;
import io.legato.Level;

import java.util.logging.Logger;

public class javaHelloWorldComponent implements Component {
    private Logger logger;

    public void setLogger(Logger logger) {
        this.logger = logger;
    }

    public void componentInit() {
        logger.log (Level.INFO, "Hello, World (from Java).");
    }
}
