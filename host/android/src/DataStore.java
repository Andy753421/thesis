package org.pileus.thesis;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;

import android.content.Context;

class DataStore extends Driver {

	private String client = "";
	private String device = "";
	private String prefix = ".";

	public DataStore(Main main) {
		super(main);
	}

	public DataStore(Main main, String preifx) {
		super(main);
		this.prefix = prefix;
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

	@Receive({"list.name:str", "!list.data"})
	public void onList(String name) {
		//try {
			Main.debug("DataStore: onList - " + name);
			Message.List lst = new Message.List();
			File dir = new File(this.prefix+'/'+name);
			for (File ent : dir.listFiles()) {
				if (ent.isDirectory())
					lst.add(name + ent.getName() + "/");
				else
					lst.add(name + ent.getName());
			}
			this.broadcast(new Message.Hash()
				.set("list", new Message.Hash()
					.set("name", name)
					.set("data", lst)));
		//} catch (IOException e) {
		//	this.broadcast(new Message.Hash()
		//		.set("list", new Message.Hash()
		//			.set("name",  name)
		//			.set("error", "fail")));
		//}
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
