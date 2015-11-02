package org.pileus.thesis;

import java.util.UUID;
import java.util.Vector;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

abstract class Driver {
	public Main   main;
	public String uuid;

	// Annotations
	@Retention(RetentionPolicy.RUNTIME)
	@Target(ElementType.METHOD)
	protected static @interface Receive {
		String[] value();
	}

	protected Object[] filter(Message msg, String[] filt) {
		Vector<Object> objs = new Vector<Object>();

		for (String str : filt) {
			boolean neg   = false;
			boolean opt   = false;
			String  typ   = null;
			Object  obj   = null;

			// Check optional and negation flag
			if (str.startsWith("!")) {
				neg = true;
				str = str.substring(1);
			}
			if (str.startsWith("?")) {
				opt = true;
				str = str.substring(1);
			}

			// Check type flag
			String[] tmp = str.split(":");
			if (tmp.length > 1) {
				str = tmp[0];
				typ = tmp[1];
			}

			// Get message path
			String[] parts = str.split("\\.");
			if (parts.length == 0) {
				Main.debug("Filter format error: " + str + " " + parts.length);
				return null;
			}
			String tail = parts[parts.length-1];

			// Lookup value
			try {
				Message cur = msg;
				for (int i = 0; i < parts.length-1; i++)
					cur = cur.get(parts[i]);
				if      (typ == "msg") obj = cur.get(tail);
				else if (typ == "str") obj = cur.str(tail);
				else if (typ == "num") obj = cur.num(tail);
				else                   obj = cur.obj(tail);
			} catch (Message.Err e) {
				obj = null;
			}
			
			// Validate
			if (neg && obj!=null)
				return null;
			if (!neg && !opt && obj==null)
				return null;

			// Push object
			if (!neg)
				objs.add(obj);
		}

		return objs.toArray();
	}

	// Constructors
	public Driver() {
		this.uuid = UUID.randomUUID().toString();    
	}

	public Driver(Main main) {
		super();
		this.main = main;                         
	}

	// Messaging
	public void broadcast(Message msg) {
		this.main.broadcast(this, msg);
	}

	public void receive(Message msg) {
		for (Method method : this.getClass().getDeclaredMethods()) {
			if (!method.isAnnotationPresent(Receive.class))
				continue;

			Receive recv = (Receive)method.getAnnotation(Receive.class);
			try {
				Object[] args = this.filter(msg, recv.value());
				if (args == null) {
					Main.debug("Filter fail: ");
				} else {
					Main.debug("Filter pass: ");
					method.invoke(this, args);
				}
			} catch (IllegalAccessException ex) {
				Main.debug("Error invoking Receive method", ex);
			} catch (InvocationTargetException ex) {
				Main.debug("Error invoking Receive method", ex);
			}
		}
	}

	public abstract void discover();
};
