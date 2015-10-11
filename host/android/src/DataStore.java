package org.pileus.thesis;

class DataStore extends Driver {

	private String client = "";
	private String device = "";

	public DataStore(Main main) {
		super(main);
	}

	public void discover() {
		this.broadcast(new Message.Hash()
			.set("uuid",         this.uuid)
			.set("capabilities", new Message.List()
				.add(new Message.Hash()
					.set("direction", "input")
					.set("class",     "webapp")
					.set("client",    this.client)
					.set("device",    this.device))
				.add(new Message.Hash()
					.set("direction", "input")
					.set("class",     "webapp")
					.set("client",    this.client)
					.set("device",    this.device))));
	}

	public void receive(Message msg) {
	}

}
