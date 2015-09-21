package org.pileus.thesis;

import java.util.UUID;

abstract class Driver {
	public Main   main;
	public String uuid;

	public void init(Main main) {
		this.uuid = UUID.randomUUID().toString();    
		this.main = main;                         

	}

	public void broadcast(Message m) {
		this.main.broadcast(m.write());
	}

	public abstract void receive(Message m);

	public abstract void discover();
};
