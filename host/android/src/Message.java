package org.pileus.thesis;

import java.util.HashMap;
import java.util.LinkedList;

public abstract class Message {
	public abstract String write();

	public static class Wrap extends Message {
		private String string;
		public Wrap init(String string) {
			this.string = string;
			return this;
		}
		public String write() {
			return string;
		}
	}

	public static class Hash extends Message {
		private HashMap<String,Message> data
			= new HashMap<String,Message>();
		public Hash set(String k, Message v) {
			this.data.put(k, v);
			return this;
		}
		public Hash set(String k, int v) {
			return this.set(k, new Num(v));
		}
		public Hash set(String k, double v) {
			return this.set(k, new Num(v));
		}
		public Hash set(String k, String v) {
			return this.set(k, new Str(v));
		}
		public String write() {
			String str = null;
			for (String k : this.data.keySet()) {
				String v = this.data.get(k).write();
				String s = "\"" + k + "\"" + ":" + v;
				str = (str==null) ? s : str+','+s;
			}
			return "{" + str + "}";
		}
	}

	public static class List extends Message {
		private LinkedList<Message> data =
			new LinkedList<Message>();
		public List add(Message m) {
			this.data.add(m);
			return this;
		}
		public String write() {
			String str = null;
			for (Message m : this.data) {
				String s = m.write();
				str = (str==null) ? s : str+','+s;
			}
			return "[" + str + "]";
		}
	}

	private static class Num extends Message {
		private double data = 0;
		public Num(int n) {
			this.data = n;
		}
		public Num(double n) {
			this.data = n;
		}
		public String write() {
			return Double.toString(this.data);
		}
	}

	private static class Str extends Message {
		private String data = "";
		public Str(String s) {
			this.data = s;
		}
		public String write() {
			return "\"" + this.data + "\"";
		}
	}
}
