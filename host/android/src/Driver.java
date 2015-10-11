package org.pileus.thesis;

import java.util.UUID;

abstract class Driver {
	public Main   main;
	public String uuid;

	public Driver() {
		this.uuid = UUID.randomUUID().toString();    
	}

	public Driver(Main main) {
		super();
		this.main = main;                         
	}

	public void broadcast(Message m) {
		this.main.broadcast(this, m);
	}

	public abstract void receive(Message m);

	public abstract void discover();
};
