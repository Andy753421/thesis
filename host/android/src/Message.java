package org.pileus.thesis;

import java.util.HashMap;
import java.util.LinkedList;

import org.json.JSONArray; 
import org.json.JSONException; 
import org.json.JSONObject; 

public class Message {
	public String     string;
	public JSONObject object;
	public JSONArray  array;

	/* Helper functions */
	protected void parseObject() throws Err, JSONException {
		if (this.object != null)
			return;
		if (this.string == null)
			throw new Err();
		this.object = new JSONObject(this.string);
	}
	protected void parseArray() throws Err, JSONException {
		if (this.array != null)
			return;
		if (this.string == null)
			throw new Err();
		this.array = new JSONArray(this.string);
	}
	protected Message toMessage(Object o) throws Err {
		if (o instanceof Message)
			return (Message)o;
		if (o instanceof JSONObject) {
			Message m = new Message();
			m.object = (JSONObject)o;
			return m;
		}
		if (o instanceof JSONArray) {
			Message m = new Message();
			m.array = (JSONArray)o;
			return m;
		}
		throw new Err();
	}
	protected Object toObject(Message m) {
		if (m.object != null)
			return (Object)m.object;
		if (m.array != null)
			return (Object)m.array;
		return null;
	}

	/* Hash Index */
	public Message no(String k) throws Err {
		try {
			this.parseObject();
			if (this.object.has(k))
				throw new Err();
		} catch (JSONException e) {
			throw new Err();
		}
		return this;
	}
	public Object obj(String k) throws Err {
		try {
			this.parseObject();
			return this.object.get(k);
		} catch (JSONException e) {
			throw new Err();
		}
	}
	public Message get(String k) throws Err {
		try {
			this.parseObject();
			return this.toMessage(this.object.get(k));
		} catch (JSONException e) {
			throw new Err();
		}
	}
	public String str(String k) throws Err {
		try {
			this.parseObject();
			return this.object.getString(k);
		} catch (JSONException e) {
			throw new Err();
		}
	}
	public double num(String k) throws Err {
		try {
			this.parseObject();
			return this.object.getDouble(k);
		} catch (JSONException e) {
			throw new Err();
		}
	}

	/* List Index */
	public Message no(int i) throws Err {
		try {
			this.parseArray();
			if (this.object.length() > i)
				throw new Err();
		} catch (JSONException e) {
			throw new Err();
		}
		return this;
	}
	public Object obj(int i) throws Err {
		try {
			this.parseArray();
			return this.array.get(i);
		} catch (JSONException e) {
			throw new Err();
		}
	}
	public Message get(int i) throws Err {
		try {
			this.parseArray();
			return this.toMessage(this.array.get(i));
		} catch (JSONException e) {
			throw new Err();
		}
	}
	public String str(int i) throws Err {
		try {
			this.parseArray();
			return this.array.getString(i);
		} catch (JSONException e) {
			throw new Err();
		}
	}
	public double num(int i) throws Err {
		try {
			this.parseArray();
			return this.array.getDouble(i);
		} catch (JSONException e) {
			throw new Err();
		}
	}

	/* Setters */
	public static class Hash extends Message {
		public Hash() {
			this.object = new JSONObject();
		}
		public Hash set(String k, Message v) {
			try {
				this.object.put(k, this.toObject(v));
			} catch (JSONException e) {}
			return this;
		}
		public Hash set(String k, int v) {
			try {
				this.object.put(k, v);
			} catch (JSONException e) {}
			return this;
		}
		public Hash set(String k, double v) {
			try {
				this.object.put(k, v);
			} catch (JSONException e) {}
			return this;
		}
		public Hash set(String k, String v) {
			try {
				this.object.put(k, v);
			} catch (JSONException e) {}
			return this;
		}
	}

	/* Setters */
	public static class List extends Message {
		public List() {
			this.array = new JSONArray();
		}
		public List add(Message v) {
			this.array.put(this.toObject(v));
			return this;
		}
		public List add(int v) {
			this.array.put(v);
			return this;
		}
		public List add(double v) {
			try {
				this.array.put(v);
			} catch (JSONException e) {}
			return this;
		}
		public List add(String v) {
			this.array.put(v);
			return this;
		}
	}

	/* Setters */
	public static class Wrap extends Message {
		public Wrap(String s) {
			this.string = s;
		}
	}

	/* Write functions */
	public String write() {
		if (this.string != null)
			return this.string;
		if (this.object != null)
			return this.string = this.object.toString();
		if (this.array != null)
			return this.string = this.array.toString();
		return this.string = "";
	}

	/* Exceptions */
	public static class Err extends Throwable {
	}
}
