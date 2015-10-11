package org.pileus.thesis;

import java.util.HashMap;
import java.util.LinkedList;

public abstract class Message {
	public abstract String write();

	public Message get(String k) throws Err { throw new Err(); }
	public String  str(String k) throws Err { throw new Err(); }
	public int     num(String k) throws Err { throw new Err(); }

	public Message get(int    i) throws Err { throw new Err(); }
	public String  str(int    i) throws Err { throw new Err(); }
	public int     num(int    i) throws Err { throw new Err(); }

	public static class Err extends Throwable {
	}

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

		/* Setters */
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

		/* Getters */
		public Message get(String k) throws Err {
			if (data.containsKey(k)) {
				return this.get(k);
			} else {
				throw new Err();
			}
		}
		public String str(String k) throws Err {
			Message m = this.get(k);
			if (m instanceof Str)
				return ((Str)m).str();
			else
				throw new Err(); 
		}
		public int num(String i) throws Err {
			Message m = this.get(k);
			if (m instanceof Num)
				return ((Int)m).num();
			else
				throw new Err(); 
		}

		/* Write */
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

		/* Setters */
		public List add(Message m) {
			this.data.add(m);
			return this;
		}

		/* Getters */
		public Message get(String k) throws Err {
			if (data.containsKey(k)) {
				return this.get(k);
			} else {
				throw new Err();
			}
		}

		/* Write */
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
