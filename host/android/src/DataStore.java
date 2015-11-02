package org.pileus.thesis;

import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;

import android.content.Context;

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
					.set("class",     "read"))
				.add(new Message.Hash()
					.set("direction", "input")
					.set("class",     "write"))
				.add(new Message.Hash()
					.set("direction", "input")
					.set("class",     "info"))
				.add(new Message.Hash()
					.set("direction", "input")
					.set("class",     "list"))));
	}

	public @interface Test {
		public boolean enabled() default true;
	}

	@Receive({"read.name:str", "!read.data"})
	public void onRead(String name) {
		try {
			Main.debug("DataStore: onRead - " + name);

			int n;
			FileInputStream fd   = this.main.openFileInput(name);
			StringBuffer    sbuf = new StringBuffer("");
			byte[]          bbuf = new byte[1024];
			while ((n = fd.read(bbuf)) != -1)
				sbuf.append(new String(bbuf, 0, n));
			fd.close();
			String  data = sbuf.toString();
			this.broadcast(new Message.Hash()
				.set("read", new Message.Hash()
					.set("name", name)
					.set("data", data)));
		} catch (IOException e) {
			this.broadcast(new Message.Hash()
				.set("read", new Message.Hash()
					.set("name",  name)
					.set("error", "fail")));
		}
	}

	@Receive({"write.name:str", "write.data:str"})
	public void onWrite(String name, String data) {
		try {
			FileOutputStream fd = this.main.openFileOutput(
					name, Context.MODE_PRIVATE);
			fd.write(data.getBytes());
			fd.close();
			this.broadcast(new Message.Hash()
				.set("write", new Message.Hash()
					.set("name", name)));
		} catch (IOException e) {
			this.broadcast(new Message.Hash()
				.set("write", new Message.Hash()
					.set("name",  name)
					.set("error", "fail")));
		}
	}
}
